`v8` では、仮想メモリの仕組みを `mmap`/`mprotect` システムコールを通じて体験します。
ユーザー空間のプログラムがカーネルにメモリの確保や権限変更を依頼する流れを、x86-64 アセンブリで直接書いて確認します。

## 概要

`v8` は仮想メモリの概念を `mmap`/`mprotect` システムコールで体験する段階です。ユーザー空間からカーネルにメモリの確保・権限変更を依頼し、ページ単位のメモリ管理を理解します。

## 仮想メモリとは

プログラムが使うアドレスは「仮想アドレス (virtual address)」です。
CPU がメモリにアクセスするとき、カーネルと MMU (Memory Management Unit) が仮想アドレスを物理アドレスに変換します。

![仮想アドレス→MMU→物理メモリの翻訳](/images/v8/virtual-memory-translation.drawio.svg)

### MMU とは

**MMU（Memory Management Unit）** は CPU の中に組み込まれたハードウェアです。CPU がメモリアクセスを行うたびに、MMU が仮想アドレスを物理アドレスに変換します。この変換はソフトウェアではなく **ハードウェアが行う** ため、毎回の命令実行で遅延がほとんどありません。

MMU は **ページテーブル** という対応表を参照して変換を行います。ページテーブルの中身を作成・更新するのはカーネルの仕事ですが、実際の変換処理は MMU が高速に実行します。つまり役割分担は以下の通りです：

- **カーネル** — ページテーブルを作成・管理する（「仮想アドレス X → 物理アドレス Y」の対応を決める）
- **MMU** — ページテーブルを参照して、CPU が出す仮想アドレスを物理アドレスに変換する

もし MMU が対応するエントリを見つけられない場合（マッピングがない、または権限がない）、**ページフォルト例外** が発生し、カーネルに制御が移ります。[v8 — SIGSEGV](#v8/sigsegv) で扱う `mprotect` + アクセス違反はまさにこのケースです。

プログラムから見えるアドレス空間は、物理メモリのレイアウトとは無関係です。
カーネルが「仮想アドレス X → 物理アドレス Y」の対応表（ページテーブル）を管理しています。

## ページ

メモリ管理の最小単位を**ページ**と呼びます。
x86-64 では通常 **4096 バイト (4 KiB)** です。

- `mmap` や `mprotect` はページ単位で動作する
- アドレスはページ境界（4096 の倍数）に揃う
- 1 ページ = 0x1000 バイト

```
0x0000_0000_0000  ┌──────────────────┐
                  │  Page 0 (4 KiB)  │
0x0000_0000_1000  ├──────────────────┤
                  │  Page 1 (4 KiB)  │
0x0000_0000_2000  ├──────────────────┤
                  │  Page 2 (4 KiB)  │
                  └──────────────────┘
```

## なぜ仮想メモリが必要か

1. **プロセス間の隔離** — 各プロセスが独立したアドレス空間を持ち、他のプロセスのメモリを直接触れない
2. **物理メモリ以上のアドレス空間** — swap やメモリマップドファイルで、物理 RAM 以上のメモリを使える
3. **メモリ保護** — ページごとに読み/書き/実行の権限を設定できる

## mmap システムコール

カーネルに新しいメモリ領域（ページ）の確保を依頼するシステムコールです。

| 引数   | レジスタ | 意味                          |
|--------|----------|-------------------------------|
| syscall# | rax    | 9 (sys_mmap)                  |
| addr   | rdi      | 希望アドレス（NULL ならカーネル任せ） |
| len    | rsi      | 確保するバイト数              |
| prot   | rdx      | 保護フラグ                    |
| flags  | r10      | マッピングフラグ              |
| fd     | r8       | ファイルディスクリプタ（anonymous なら -1） |
| offset | r9       | オフセット                    |

**保護フラグ (prot):**
- `PROT_NONE` = 0 — アクセス不可
- `PROT_READ` = 1 — 読み取り可
- `PROT_WRITE` = 2 — 書き込み可
- `PROT_READ | PROT_WRITE` = 3 — 読み書き可

**マッピングフラグ (flags):**
- `MAP_PRIVATE` = 0x02 — プライベートマッピング
- `MAP_ANONYMOUS` = 0x20 — ファイルに紐付かない
- `MAP_PRIVATE | MAP_ANONYMOUS` = 0x22 — 最も一般的な組み合わせ

## `mmap` と `malloc` の違い

ユーザー空間で「メモリを確保する」と聞くと、C の `malloc` を思い出すかもしれません。ここでの違いを整理しておきます。

- **`mmap`** — カーネルに新しい仮想メモリ領域を要求する仕組み。ページ単位で確保される
- **`malloc`** — libc 側のメモリアロケータ。小さな要求をまとめたり再利用したりしながら、必要に応じて裏で `mmap` や `brk` を使う

この章で直接扱うのは `mmap` です。つまり「アロケータの一段下」で、仮想アドレス空間にページをどう生やすかを見る段階だと考えると位置づけが分かりやすくなります。

### mmap_write — ページの確保と読み書き

`mmap` でページを確保し、値を書き込んで読み出す最小の例です。

| ステップ | 操作                         | 状態                                 |
|----------|------------------------------|--------------------------------------|
| 1        | mmap(PROT_READ\|PROT_WRITE)  | ページ確保、rax = アドレス           |
| 2        | mov qword [rax], 42          | ページに 42 を書き込み               |
| 3        | mov rbx, [rax]               | ページから 42 を読み出し             |
| 4        | exit(42)                     | 終了コード 42                        |

{{code:asm/mmap_write.asm}}

### GDB の `info proc mappings`

GDB で `info proc mappings` を実行すると、プロセスのメモリマッピングを一覧表示できます。
`mmap` で確保したページがどのアドレスに配置されたか、権限はどうなっているかを確認できます。

```
(gdb) info proc mappings
  Start Addr           End Addr       Size     Offset  Perms  objfile
  0x7ffff7ff0000     0x7ffff7ff1000     0x1000  0x0     rw-p   [anon]
```

## mprotect システムコール

確保済みページのアクセス権を変更するシステムコールです。

| 引数   | レジスタ | 意味                |
|--------|----------|---------------------|
| syscall# | rax    | 10 (sys_mprotect)   |
| addr   | rdi      | 対象アドレス        |
| len    | rsi      | バイト数            |
| prot   | rdx      | 新しい保護フラグ    |

権限を `PROT_NONE` に変更すると、そのページへのすべてのアクセスが禁止されます。

### SIGSEGV

権限のないメモリにアクセスしたとき、CPU が例外を発生させ、カーネルがプロセスに **SIGSEGV (signal 11)** を送ります。
これは [v7 — シグナルの概念](#v7/シグナルの概念（プレビュー）) で学んだシグナルの一種です。
デフォルトの動作はプロセスの強制終了で、終了コードは **139 (= 128 + 11)** になります。

![SIGSEGV 発生のシーケンス図](/images/v8/sigsegv-sequence.drawio.svg)

### mprotect_crash — 権限剥奪と SIGSEGV

`mmap` でページを確保し、書き込みに成功した後、`mprotect` でアクセス権を剥奪します。再度アクセスすると SIGSEGV が発生します。

| ステップ | 操作                         | 状態                                 |
|----------|------------------------------|--------------------------------------|
| 1        | mmap(PROT_READ\|PROT_WRITE)  | ページ確保、r12 = アドレス           |
| 2        | mov qword [r12], 99          | 書き込み成功                         |
| 3        | mprotect(PROT_NONE)          | アクセス権を剥奪                     |
| 4        | mov rbx, [r12]               | SIGSEGV → 終了コード 139             |

{{code:asm/mprotect_crash.asm}}

## 独立したマッピング

`mmap` を複数回呼ぶと、それぞれ独立したメモリ領域が確保されます。
一方のページに書き込んでも、他方のページには影響しません。

![仮想ページ→物理フレームのマッピング](/images/v8/page-frame-mapping.drawio.svg)

### two_mappings — 2 つのページの独立性

| ステップ | 操作                         | 状態                                 |
|----------|------------------------------|--------------------------------------|
| 1        | mmap → r12                   | page1 確保                           |
| 2        | mmap → r13                   | page2 確保（page1 とは独立）         |
| 3        | mov qword [r12], 11          | page1 = 11                           |
| 4        | mov qword [r13], 22          | page2 = 22（page1 に影響しない）     |
| 5        | exit([r12])                  | 終了コード 11                        |

{{code:asm/two_mappings.asm}}

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3 Ch.4 "Paging"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — x86-64 のページサイズ 4096 バイト (4 KiB)
- [Linux kernel `syscall_64.tbl`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl) — sys_mmap=9, sys_mprotect=10
- [`mmap(2)` man page](https://man7.org/linux/man-pages/man2/mmap.2.html) — PROT_NONE=0, PROT_READ=1, PROT_WRITE=2, MAP_PRIVATE=0x02, MAP_ANONYMOUS=0x20
- [Bash Reference Manual "Exit Status"](https://www.gnu.org/software/bash/manual/html_node/Exit-Status.html) — シグナル N で終了したプロセスの exit code = 128+N（Bash の規約）
