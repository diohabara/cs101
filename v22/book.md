`v22` では、バディアロケータ（buddy allocator）を実装して物理メモリを管理します。
OS のメモリ管理の基盤となる仕組みで、ページ単位のメモリ割り当てと解放を効率的に行います。

## 概要

`v22` は v21（タイマー割り込み）の上に物理メモリアロケータ（PMM: Physical Memory Manager）を追加します。

ベアメタル環境には `malloc` がありません。メモリを使うには:
1. 使えるメモリの範囲を把握する
2. どのページが使用中/空きかを追跡する
3. 要求に応じてページを割り当て/解放する

これが PMM の役割です。

## バディアロケータとは

バディアロケータは Linux カーネルでも使われている物理メモリ管理手法です。メモリを 2 の累乗サイズのブロックで管理します。

### オーダーとブロックサイズ

| オーダー | ページ数 | サイズ |
|----------|----------|--------|
| 0 | 1 | 4KB |
| 1 | 2 | 8KB |
| 2 | 4 | 16KB |
| 3 | 8 | 32KB |
| ... | ... | ... |
| 10 | 1024 | 4MB |

### フリーリスト

各オーダーに空きブロックの連結リストを持ちます。空きページの先頭 8 バイトに次のノードへのポインタを格納するため、追加のメモリを消費しません。

```
free_lists[0] → [0x201000] → [0x203000] → NULL    (4KB ブロック)
free_lists[1] → [0x204000] → NULL                  (8KB ブロック)
free_lists[2] → NULL                                (16KB ブロック)
...
free_lists[9] → [0x200000] → NULL                   (2MB ブロック)
```

### 割り当て（split）

order 0 (4KB) を要求した場合:

```
1. free_lists[0] が空 → 上位を探す
2. free_lists[9] に 2MB ブロックあり
3. 2MB → 1MB + 1MB (片方を free_lists[8] に追加)
4. 1MB → 512KB + 512KB (片方を free_lists[7] に追加)
5. ... 繰り返し ...
6. 8KB → 4KB + 4KB (片方を free_lists[0] に追加)
7. 残りの 4KB を返す
```

### 解放（merge）

order 0 (4KB) のブロックを解放する場合:

```
1. バディアドレスを計算: buddy = addr XOR 0x1000
2. バディが free_lists[0] にあるか探す
3. あれば: バディを除去し、2つを結合して order 1 のブロックにする
4. order 1 のバディを探す → あれば結合して order 2 に...
5. バディが見つからなくなるまで繰り返す
6. 最終ブロックをフリーリストに追加
```

![バディアロケータの split 可視化](/images/v22/buddy-allocator.drawio.svg)

### バディアドレスの計算

バディアドレスは XOR で計算できます:

```
ブロック  0x200000 (order 0, 4KB): バディ = 0x200000 ^ 0x1000 = 0x201000
ブロック  0x201000 (order 0, 4KB): バディ = 0x201000 ^ 0x1000 = 0x200000
ブロック  0x200000 (order 1, 8KB): バディ = 0x200000 ^ 0x2000 = 0x202000
```

XOR が使える理由: 2 の累乗サイズでアラインされたブロックのペアは、サイズに対応するビットが 0/1 で交互になっています。

## 実装

### pmm.h --- インターフェース定義

{{code:src/pmm.h}}

### pmm.c --- バディアロケータ

{{code:src/pmm.c}}

### boot.c --- テスト用エントリポイント

PMM を初期化し、4 ページを割り当てて全て異なるアドレスであることを検証します。

{{code:src/boot.c}}

## メモリレイアウト

```
0x000000 - 0x0FFFFF : BIOS / レガシーデバイス
0x100000 - 0x1FFFFF : カーネルイメージ
0x200000 - 0x3FFFFF : PMM 管理対象 (2MB = 512 ページ)   ← ここを管理
0x400000 -          : 未使用
```

## 実行結果

```
Hello, bare metal!
GDT loaded
IDT loaded
Timer started: 100Hz
PMM: initialized 512 pages (0x200000 - 0x400000)
PMM: allocated page at 0x200000
PMM: allocated page at 0x201000
PMM: allocated page at 0x202000
PMM: allocated page at 0x203000
4 unique pages
PMM: all pages freed
```

## Linux カーネルとの比較

Linux のバディアロケータ（`mm/page_alloc.c`）も同じ原理で動作します:
- `MAX_ORDER` = 11（Linux 6.x, 4KB〜8MB）
- `__alloc_pages()` → order 以上のフリーリストを探す → split
- `__free_pages()` → バディチェック → merge

Linux の追加機能:
- NUMA ノード対応
- メモリゾーン（DMA, Normal, HighMem）
- per-CPU ページキャッシュ（pcplist）

## 参考文献

- [The Art of Linux Kernel Design, Ch.5](https://www.kernel.org/doc/gorman/html/understand/understand009.html) --- Linux のバディアロケータ
- [OSDev Wiki: Page Frame Allocation](https://wiki.osdev.org/Page_Frame_Allocation) --- 物理ページ管理の概要
- Donald Knuth, *The Art of Computer Programming, Vol.1* --- バディシステムの原論文参照
- [Linux kernel mm/page_alloc.c](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) --- 実際の Linux カーネル実装
