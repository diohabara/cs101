`v6` ではファイルディスクリプタと I/O を学びます。
[v5](#v5) で学んだ syscall を使って、ファイルの読み書きを行い、UNIX の I/O モデルを体験します。

## 概要

`v6` はファイルディスクリプタと I/O を扱う段階です。sys_read/sys_write/sys_open/sys_close を使ってファイルの読み書きを行い、fd テーブル、.bss セクション、"Everything is a file" の概念を学びます。

## この段階で押さえること

- ファイルディスクリプタ（fd）はカーネルオブジェクトへの参照番号
- stdin=0, stdout=1, stderr=2 は最初から開かれている
- `sys_read` / `sys_write` / `sys_open` / `sys_close` の使い方
- `.bss` セクション: 未初期化データ領域
- "Everything is a file" — UNIX の設計哲学

## ファイルディスクリプタ

ファイルディスクリプタ（fd）は、プロセスがカーネルオブジェクトを参照するための **整数** です。
v5 で `sys_write` の第 1 引数に `rdi = 1`（stdout）や `rdi = 2`（stderr）を渡しましたが、あの番号が fd です。

ファイルディスクリプタとは、カーネルが管理する I/O リソース（ファイル、端末、パイプ、ネットワークソケットなど）を指す **ただの整数番号** です。ユーザーランドのプログラムは、I/O リソースの実体には直接触れません。代わりにカーネルから「この番号で呼んでくれ」と整数を受け取り、以後はその番号を使って read / write します。

![fd とファイルテーブルの構造](/images/v5/fd-file-table.drawio.svg)

なぜ直接触らせないのか？ [v5 — なぜ syscall が存在するか](#v5/なぜ-syscall-が存在するか) で学んだ通り、ユーザーランドのプログラムにハードウェアを直接操作させると危険だからです。カーネルが仲介者として fd を発行し、権限チェックやバッファリングを行います。

### stdin / stdout / stderr

プロセスには最初から 3 つのファイルディスクリプタが開いています：

| fd | 名前 | 用途 |
|----|------|------|
| 0 | stdin | 標準入力 |
| 1 | stdout | 標準出力 |
| 2 | stderr | 標準エラー出力 |

### fd テーブルとカーネルリソース

プロセスの fd テーブルは、各 fd がカーネル内のどのリソースに対応するかを管理しています。

![fd テーブルとカーネルリソースの対応](/images/v6/fd-table-resources.drawio.svg)

新しいファイルを `sys_open` で開くと、空いている最小の fd 番号が割り当てられます。
stdin=0, stdout=1, stderr=2 が既に使われているので、最初に開いたファイルは **fd=3** になります。

{{code:asm/fd_table.asm}}

## file descriptor と `FILE*` の違い

C を触ったことがあると、`FILE*` と fd を同じものだと感じやすいですが、実際には層が異なります。

- **fd** — カーネルが管理する整数ハンドル。`read` / `write` / `open` / `close` の対象
- **`FILE*`** — libc が提供する高水準ストリーム。内部にバッファや書式化の都合を持つ

`stdin`, `stdout`, `stderr` という名前も、最終的には fd 0, 1, 2 に対応します。ただし C の `fopen` や `fprintf` を使うと、裏側で `FILE*` のバッファリングや整形処理が走ります。この章ではそこを 1 段剥がして、**整数 fd と syscall だけで I/O を操作する** 見方に慣れるのが目的です。

## "Everything is a file"

UNIX の設計哲学の根幹です：
- ファイル → fd + read/write
- パイプ → fd + read/write
- ソケット → fd + read/write
- デバイス → fd + read/write

すべてが同じ `read`/`write` インターフェースで操作できます。この統一性が UNIX の強みです。

## syscall 番号表

| 番号 | 名前 | rdi | rsi | rdx | 戻り値 |
|------|------|-----|-----|-----|--------|
| 0 | sys_read | fd | buf | count | 読み取りバイト数 |
| 1 | sys_write | fd | buf | count | 書き込みバイト数 |
| 2 | sys_open | pathname | flags | mode | fd |
| 3 | sys_close | fd | — | — | 0 (成功) |
| 60 | sys_exit | status | — | — | — |

## .bss セクション

`.bss` セクションは **未初期化データ** の領域です。ゼロで初期化されることが保証されています。

```nasm
section .bss
    buf: resb 16       ; 16 バイトを確保（reserve bytes）
```

| 擬似命令 | サイズ | 説明 |
|---------|-------|------|
| `resb N` | N バイト | reserve bytes |
| `resw N` | N×2 バイト | reserve words |
| `resd N` | N×4 バイト | reserve double-words |
| `resq N` | N×8 バイト | reserve quad-words |

`.data` セクションとの違い：
- `.data` — 初期値あり。実行ファイルにデータが含まれる
- `.bss` — 初期値なし。実行ファイルにはサイズ情報のみ（ファイルが小さくなる）

## sys_open のフラグ

ファイルを開くとき、フラグをビット OR で組み合わせます：

| フラグ | 値 | 意味 |
|--------|-----|------|
| O_RDONLY | 0 | 読み取り専用 |
| O_WRONLY | 1 | 書き込み専用 |
| O_CREAT | 64 (0x40) | ファイルが存在しなければ作成 |
| O_TRUNC | 512 (0x200) | 既存の内容を切り詰め |

```
O_WRONLY | O_CREAT | O_TRUNC = 1 | 64 | 512 = 577
```

ビット OR によるフラグの組み合わせは、カーネル API で広く使われるパターンです（v8 の mmap フラグと同じ考え方）。

## open → write → close ライフサイクル

ファイル I/O の基本パターン：

![open→write→close シーケンス図](/images/v6/open-write-close.drawio.svg)

{{code:asm/write_file.asm}}

## エコーバック: read → write

`read_stdin.asm` は stdin から読んだデータをそのまま stdout に書き戻します。

```
echo "hello" | ./read_stdin
```

パイプ（`|`）でつなぐと、左のコマンドの stdout が右のコマンドの stdin に接続されます。
これも fd の仕組み — "Everything is a file" の実例です。

{{code:asm/read_stdin.asm}}

## シェルのリダイレクト — fd の繋ぎ替え

シェルの `>` は、実は `1>` の省略形です。つまり「fd 1（stdout）の出力先をファイルに切り替える」という意味です。fd の番号を変えれば、stderr やその他の fd もリダイレクトできます。

| 構文 | 意味 | fd の操作 |
|------|------|----------|
| `> file` | stdout をファイルへ | fd 1 → file |
| `2> file` | stderr をファイルへ | fd 2 → file |
| `&> file` | stdout + stderr 両方をファイルへ | fd 1, 2 → file |
| `>> file` | stdout を追記 | fd 1 → file (append) |
| `2>> file` | stderr を追記 | fd 2 → file (append) |
| `2>&1` | stderr を stdout と同じ先へ | fd 2 → fd 1 のコピー |
| `< file` | stdin をファイルから | fd 0 ← file |

`2>&1` の `&` は「これは fd 番号だよ」という印です。`2>1` と書くと「`1` という名前のファイル」に書き込んでしまいます。

実用的な例をいくつか示します：

```bash
# stdout を捨てる → 何も表示されない
./hello > /dev/null

# stdout を捨てる → stderr は表示される
./write_stderr > /dev/null

# stderr だけ捨てる → stdout は端末に出る
./hello 2> /dev/null

# stdout と stderr を別ファイルに分離して保存
./my_program > output.log 2> error.log

# パイプに stderr も流す（| は stdout しか渡さない）
./my_program 2>&1 | grep "error"
```

リダイレクトは **左から右に順番に処理** されます。そのため `>/dev/null 2>&1`（stdout を捨ててから stderr を stdout と同じ先へ → 両方消える）と `2>&1 >/dev/null`（stderr を今の stdout＝端末にコピーしてから stdout だけ捨てる → stderr は端末に残る）は結果が違います。

シェルは内部的に、[v7 — fork + exec パターン](#v7/fork-+-exec-パターン) で学ぶ `fork` の後・`execve` の前に fd の繋ぎ替えを行っています。つまりリダイレクトの仕組みは、この章で学んだ fd テーブルと v7 の fork+exec パターンの組み合わせです。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [POSIX.1-2017 `<unistd.h>`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/stdin.html) — STDIN_FILENO=0, STDOUT_FILENO=1, STDERR_FILENO=2
- [Linux kernel `syscall_64.tbl`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl) — sys_read=0, sys_write=1, sys_open=2, sys_close=3
- [glibc `bits/fcntl-linux.h`](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/bits/fcntl-linux.h) — O_WRONLY=1, O_CREAT=0100(8進)=64, O_TRUNC=01000(8進)=512
- [ELF Specification (System V ABI)](https://refspecs.linuxfoundation.org/LSB_1.1.0/gLSB/specialsections.html) — `.bss` セクションのゼロ初期化保証
- [NASM Manual §3.2](https://www.nasm.us/doc/nasmdoc3.html) — `resb`/`resw`/`resd`/`resq` による領域確保
- [Bash Reference Manual §3.6 "Redirections"](https://www.gnu.org/software/bash/manual/html_node/Redirections.html) — シェルのリダイレクト構文と処理順序
