`v10` では、malloc/free の内部実装を自作し、動的メモリ管理の仕組みを理解します。

## 概要

`v10` は v9 で学んだ C のポインタ操作を活用し、メモリアロケータを 2 種類実装します。
`malloc()` は魔法ではなく、mmap で OS からもらったメモリの塊をヘッダ付きチャンクに切り分けて管理しているだけです。

| アロケータ | alloc | free | 用途 |
|-----------|-------|------|------|
| バンプアロケータ | O(1)・ポインタを進めるだけ | 不可（一括解放のみ） | ゲームの 1 フレーム内の一時割り当て |
| フリーリストアロケータ | O(n)・空きリスト探索 | O(n)・リスト挿入+結合 | 汎用（malloc/free の原型） |

![バンプアロケータとフリーリストアロケータの比較](/images/v10/allocator-comparison.drawio.svg)

## mmap --- OS からメモリをもらう

v9 で `syscall` のラッパーとして `write()` を使いました。
メモリアロケータでは `mmap()` を使って OS からメモリプール（連続した仮想メモリ領域）を取得します。

```
syscall レベル:
  mov rax, 9          ; SYS_mmap
  mov rdi, 0          ; addr = NULL（カーネルに任せる）
  mov rsi, 4096       ; length = 4096 バイト（1 ページ）
  mov rdx, 3          ; PROT_READ | PROT_WRITE
  mov r10, 0x22       ; MAP_PRIVATE | MAP_ANONYMOUS
  mov r8, -1          ; fd = -1（ファイルなし）
  mov r9, 0           ; offset = 0
  syscall              ; rax = 割り当てられたアドレス

C レベル:
  void *pool = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
```

`MAP_ANONYMOUS` を指定すると、ファイルに紐づかないメモリが得られます。
この領域をアロケータのプールとして使います。

## 共通ヘッダ

bump allocator と free-list allocator で共有する定義です。

{{code:src/alloc.h}}

`align8()` は v1-v8 のアセンブリで暗黙に行っていたアライメント調整を明示しています。
x86-64 では多くのデータ型が 8 バイト境界のアクセスを要求し、非アラインアクセスはパフォーマンスペナルティを招きます。

## バンプアロケータ

最もシンプルなアロケータです。ポインタを一方向に進める（bump する）だけで割り当てを行います。

### メモリレイアウト

```
pool                                        pool + 4096
|                                                      |
v                                                      v
[  alloc(64)  ][  alloc(128)  ][  alloc(256)  ][ 未使用 ]
^              ^               ^               ^
offset=0       offset=64       offset=192      offset=448
```

### 動作ステップ

| ステップ | 操作 | offset | 返却アドレス | 残り容量 |
|----------|------|--------|-------------|---------|
| 初期状態 | `bump_init(4096)` | 0 | --- | 4096 |
| 1 | `bump_alloc(64)` | 64 | pool+0 | 4032 |
| 2 | `bump_alloc(128)` | 192 | pool+64 | 3904 |
| 3 | `bump_alloc(256)` | 448 | pool+192 | 3648 |

free は個別にできません。プール全体を `munmap` で一括解放するしかありません。
これはゲームエンジンでフレームアロケータとして実際に使われるパターンです。

### 実装

{{code:src/bump_alloc.c}}

exit code 3（3 つのユニークなアドレス）で正常終了します。

## フリーリストアロケータ

malloc/free の基本原理です。空きブロックを連結リスト（フリーリスト）で管理します。

### チャンクのメモリレイアウト

```
malloc(64) が返すポインタ
          |
          v
+---------+------------------+
| header  | ユーザデータ (64B) |
| size=64 |                  |
| next=NULL|                 |
+---------+------------------+
^                            ^
block                    block + HEADER_SIZE + 64
```

`malloc` が返すポインタの直前には常にヘッダがあります。
`free(ptr)` は `ptr - HEADER_SIZE` でヘッダを復元し、サイズ情報を読み出します。

### アドレス再利用の仕組み

| ステップ | 操作 | フリーリスト | 説明 |
|----------|------|-------------|------|
| 1 | `alloc A (64B)` | `[残り全体]` | プール先頭から切り出す |
| 2 | `alloc B (64B)` | `[残り]` | A の直後から切り出す |
| 3 | `free A` | `[A] → [残り]` | A をフリーリストに返却 |
| 4 | `alloc C (64B)` | `[残り]` | A の跡地を再利用（同じアドレス） |

### コアレッシング（結合）

隣接する空きブロックを結合して、大きなブロックを確保できるようにします。

```
free 直後（断片化状態）:
  [free A (64B)][free B (64B)]   ← 隣接する 2 つの空きブロック

coalesce 後:
  [free A+B (64+64+header = 144B)]  ← 1 つの大きな空きブロック
```

コアレッシングがないと、64 バイトの空きが 2 つあっても 128 バイトの割り当てに失敗します。
これがメモリの断片化（fragmentation）問題であり、malloc の実装が複雑になる主な理由です。

### 実装

{{code:src/freelist_alloc.c}}

free 後のアドレス再利用とコアレッシングを確認し、exit code 0 で正常終了します。

## PlayStation との関連

- **バンプアロケータ**: PS4/PS5 のゲームでは、フレーム単位のバンプアロケータが多用されます。描画コマンドの構築など、1 フレーム（16.6ms）の間に大量の小さなオブジェクトを割り当て、フレーム終了時にプール全体をリセットします。個別の free が不要なため、malloc/free のオーバーヘッドを完全に排除できます
- **カスタムアロケータ**: AAA タイトルでは用途別に複数のアロケータを使い分けます。レベルロード用のスタックアロケータ、パーティクル用のプールアロケータ、汎用のフリーリストアロケータなどを組み合わせて、メモリ断片化を最小化しつつ割り当て性能を最大化します
- **mmap/sbrk の使い分け**: glibc の malloc は小さな割り当て（128KB 未満）に sbrk、大きな割り当てに mmap を使います。ゲームエンジンはこの挙動に依存せず、起動時に大きなプールを mmap で確保して自前で管理するのが一般的です

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [mmap(2) - Linux manual page](https://man7.org/linux/man-pages/man2/mmap.2.html) --- `MAP_ANONYMOUS | MAP_PRIVATE` による匿名メモリマッピングの仕様
- [glibc malloc internals](https://sourceware.org/glibc/wiki/MallocInternals) --- glibc の malloc 実装における chunk 構造、free list 管理、coalescing の詳細
- [Doug Lea's malloc (dlmalloc)](https://gee.cs.oswego.edu/dl/html/malloc.html) --- フリーリストアロケータの設計原理。boundary tag、binning、coalescing の解説
- [ISO/IEC 9899:2011 (C11) 7.22.3](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) --- `malloc`, `free`, `realloc` の標準仕様
