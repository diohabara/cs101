`v7` ではプロセスの概念を学びます。
fork でプロセスを複製し、execve で別のプログラムに変身させる — UNIX のプロセスモデルの根幹を体験します。

## 概要

`v7` はプロセスモデルを扱う段階です。sys_getpid でプロセスのアイデンティティを確認し、sys_fork でプロセスの複製、sys_execve でプロセスイメージの置き換えを体験します。fork + exec + wait パターンは UNIX のプログラム実行の基本形です。

## この段階で押さえること

- プロセスとは何か — 実行中のプログラムのインスタンス
- PID — プロセスのアイデンティティ
- fork() — プロセスの完全コピーを作る
- execve() — プロセスイメージを新プログラムに置き換える
- wait4() — 親が子の終了を待つ

## プロセスとは

プロセスは「実行中のプログラムのインスタンス」です。各プロセスは以下を持ちます：

- **PID** — プロセスを一意に識別する番号
- **メモリ空間** — コード、データ、スタック、ヒープ
- **fd テーブル** — 開いているファイルディスクリプタの一覧（[v6](#v6/ファイルディスクリプタ) で学んだ）
- **レジスタ状態** — rax, rbx, ..., rip, rsp, RFLAGS

![プロセスの構成要素](/images/v7/process-components.drawio.svg)

## sys_getpid — プロセス ID の取得

| 引数 | レジスタ | 値 |
|------|---------|-----|
| syscall# | rax | 39 (sys_getpid) |
| **戻り値** | **rax** | 現在のプロセスの PID |

引数なしの最もシンプルな syscall です。戻り値の PID は常に正の整数です。

exit code は 0-255 の範囲しか表現できないため、PID の下位 8 ビットのみが exit code に反映されます。

{{code:asm/getpid.asm}}

## fork() — プロセスの複製

`fork()` はプロセスの **完全なコピー** を作ります。

![fork() の動作](/images/v7/fork-operation.drawio.svg)

| | 親プロセス | 子プロセス |
|---|-----------|-----------|
| fork() 戻り値 | 子の PID (> 0) | 0 |
| メモリ | コピー | コピー |
| fd テーブル | そのまま | コピー |
| PID | 変わらない | 新しい PID |

fork の戻り値で親と子を区別します：
- `rax > 0` → 親プロセス（rax = 子の PID）
- `rax = 0` → 子プロセス

```nasm
mov rax, 57    ; sys_fork
syscall
cmp rax, 0
jz .child      ; rax=0 なら子プロセス
; ここは親プロセス
```

{{code:asm/fork_basic.asm}}

## wait4() — 子の終了を待つ

親プロセスが子の終了を待たないと、子プロセスは **ゾンビプロセス** になります。

| 引数 | レジスタ | 値 |
|------|---------|-----|
| syscall# | rax | 61 (sys_wait4) |
| pid | rdi | 待つ子の PID |
| status | rsi | 終了ステータスを書き込むアドレス（NULL可） |
| options | rdx | オプション（0 = ブロッキング） |
| rusage | r10 | リソース使用情報（NULL可） |

### なぜ「wait4」なのか

`wait4` の「4」はバージョン番号ではなく **引数の数** です。wait 系 syscall は歴史的に引数が増えていきました：

| syscall | 引数 | 由来 |
|---------|------|------|
| `wait()` | 1個（status） | 初期 UNIX。任意の子を待つ |
| `wait3()` | 3個（status, options, rusage） | 4.3BSD で追加。リソース使用情報を取得可能に |
| `waitpid()` | 3個（pid, status, options） | POSIX で追加。特定の子を指定可能に |
| `wait4()` | 4個（pid, status, options, rusage） | 4.3BSD。waitpid + wait3 の合体版 |

`wait1` や `wait2` は存在しません。Linux の syscall テーブルでは `wait4` (61) が使われており、この章ではこれを直接呼んでいます。

## execve() — プロセスイメージの置き換え

`execve()` は現在のプロセスを **別のプログラムに置き換えます**。

| 引数 | レジスタ | 値 |
|------|---------|-----|
| syscall# | rax | 59 (sys_execve) |
| pathname | rdi | 実行ファイルのパス |
| argv | rsi | 引数配列（NULL終端） |
| envp | rdx | 環境変数配列（NULL終端） |

重要なポイント：
- **PID は変わらない**: 同じプロセスが別のプログラムになる
- **成功すると戻らない**: execve が成功すると、呼び出し元のコードは消え去る
- **メモリは置き換わる**: コード、データ、スタックがすべて新プログラムのものになる
- **fd テーブルは引き継がれる**: open 済みの fd はそのまま使える

![fork+execve シーケンス図](/images/v7/fork-execve-sequence.drawio.svg)

## fork + exec パターン

UNIX でプログラムを実行する基本パターン：

1. `fork()` で子プロセスを作る
2. 子プロセスで `execve()` を呼び、別のプログラムに変身する
3. 親プロセスは `wait4()` で子の終了を待つ

シェルが `ls` や `cat` を実行するとき、内部的にこのパターンを使っています。

{{code:asm/execve_echo.asm}}

## なぜ `fork` と `execve` を分けるのか

初見では「子プロセスを作って、その子をすぐ別プログラムにするなら、1 回でできそう」と感じるはずです。UNIX が `fork` と `execve` を分けている理由は、**親が複製と差し替えを別々に制御できる** からです。

- `fork` だけ使えば、親子でほぼ同じ状態を持つ 2 本の実行流れを作れる
- `execve` だけ使えば、現在の PID を保ったまま別プログラムへ置き換えられる
- 両方を組み合わせると、シェルのように「親は待つ・子は差し替える」という役割分担ができる

この分離はパイプ、リダイレクト、ジョブ制御のような UNIX らしい機能の土台です。つまり `fork + execve + wait4` は 3 つの独立した syscall ではなく、**プロセス実行モデルを構成する 1 セット** として読むのが大事です。

## シグナルの概念（プレビュー）

[v8](#v8/sigsegv) で学ぶ SIGSEGV は **シグナル** の一種です。

シグナルは **カーネルからプロセスへの非同期通知** です：
- **SIGSEGV (11)** — 不正なメモリアクセス
- **SIGKILL (9)** — 強制終了（ブロック不可）
- **SIGCHLD (17)** — 子プロセスの状態変化

シグナルの詳細は OS トラック（v9 以降）で扱います。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Linux kernel `syscall_64.tbl`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl) — sys_getpid=39, sys_fork=57, sys_execve=59, sys_wait4=61
- [POSIX.1-2017 `fork()`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html) — 親は子の PID、子は 0 を受け取る
- [POSIX.1-2017 `exec`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/exec.html) — 成功時は戻らない（呼び出し元のプロセスイメージが置き換わる）
- [`signal(7)` man page](https://man7.org/linux/man-pages/man7/signal.7.html) — x86-64 Linux でのシグナル番号: SIGKILL=9, SIGSEGV=11, SIGCHLD=17
