`v5` ではシステムコールの仕組みを深く理解します。
v1 で使った `sys_exit` に加え、`sys_write` でデータを出力し、カーネルとの対話を体験します。

## 概要

`v5` はシステムコールの仕組みを深く理解する段階です。sys_write による stdout/stderr への出力を通じて、syscall 呼び出し規約（レジスタの割り当てと戻り値）、ユーザーモードとカーネルモードの境界を学びます。

## この段階で押さえること

- syscall 呼び出し規約（レジスタの役割）
- ユーザーモードとカーネルモードの境界
- なぜ syscall が必要なのか
- syscall の戻り値が rax を上書きすること

## UNIX・POSIX・BSD・Linux

この教材では「x86-64 Linux」を対象にしていますが、syscall やプロセスの資料を読むと UNIX、POSIX、BSD、Linux といった名前が頻繁に登場します。混乱しやすいので、ここで関係を整理しておきます。

### UNIX

1969 年に AT&T ベル研究所で Ken Thompson と Dennis Ritchie が開発した OS です。「ファイルディスクリプタ」「プロセス」「fork/exec」「パイプ」など、この教材で扱う概念の多くは UNIX が発明しました。ただし UNIX そのものは AT&T の商標であり、ソースコードも当初は自由に使えませんでした。

### BSD

1970 年代後半にカリフォルニア大学バークレー校が UNIX のソースコードに独自の改良を加えて配布したのが **BSD（Berkeley Software Distribution）** です。TCP/IP ネットワークスタック、`wait3` / `wait4` syscall、ソケット API など、現代のネットワーキングの基盤となる機能の多くは BSD が導入しました。現在も FreeBSD、OpenBSD、macOS（Darwin カーネル）などに系譜が続いています。

### POSIX

UNIX が広まるにつれ、各ベンダーが独自に拡張した結果「同じ UNIX なのに動かない」という互換性問題が起きました。これを解決するために IEEE が策定した **標準仕様** が **POSIX（Portable Operating System Interface for uniX）** です。末尾の **X** は UNIX の X で、「UNIX 向けの移植可能な OS インターフェース」という意味になります。名付けたのは Richard Stallman（GNU プロジェクトの創始者）です。

POSIX は OS そのものではなく、「`fork()` はこう動くべき」「`write()` のインターフェースはこう」といった **API の約束事** です。POSIX に準拠していれば、異なる OS 間でプログラムを移植しやすくなります。

### Linux

1991 年に Linus Torvalds が開発した **カーネル** です。UNIX のソースコードは含まれていませんが、POSIX に（おおむね）準拠する形で設計されたため、UNIX 系のプログラムがそのまま動きます。

重要な区別：
- **Linux カーネル** — syscall を処理し、プロセスやメモリを管理するソフトウェア本体
- **Linux ディストリビューション**（Ubuntu、Debian、Arch など） — Linux カーネル + シェル + libc + ユーティリティ群をまとめたパッケージ

この教材で `syscall` 命令を使って呼び出しているのは **Linux カーネル** の機能です。syscall 番号（`sys_write=1`、`sys_exit=60` など）は Linux 固有のもので、同じ POSIX 準拠でも FreeBSD では番号が異なります。

### 本教材での位置づけ

| 用語 | 本教材との関係 |
|------|--------------|
| UNIX | 概念の源流。fd、プロセス、syscall の設計思想はここから |
| BSD | `wait3`/`wait4` など一部の syscall の出自。macOS の基盤 |
| POSIX | 資料で `POSIX.1-2017` と引用しているのはこの標準仕様 |
| Linux | 本教材の実行環境。syscall 番号やカーネル動作はすべて Linux 前提 |

## syscall と libc 関数の違い

UNIX 系の資料を読むと `write(2)` や `printf(3)` のような C ライブラリ関数と、アセンブリの `syscall` が混ざって見えることがあります。ここは層を分けて理解すると迷いません。

- **syscall** — CPU 命令。ユーザーモードからカーネルモードへ入る入口そのもの
- **`write(2)`** — libc から呼べる関数インターフェース。内部で `syscall` を使って `sys_write` へつなぐ
- **`printf(3)`** — 書式展開まで含むさらに高水準な関数。最終的には `write` 系へ落ちる

この章で学ぶのは一番下の層です。`mov rax, 1`、`mov rdi, 1`、`syscall` と自分で並べることで、普段 C や Rust が隠してくれている「カーネルへの入り口」が見えるようになります。

## Syscall 呼び出し規約

x86-64 Linux では、`syscall` 命令を使ってカーネルの機能を呼び出します。
レジスタの割り当ては以下の通りです：

| レジスタ | 役割 |
|----------|------|
| `rax` | システムコール番号 |
| `rdi` | 第 1 引数 |
| `rsi` | 第 2 引数 |
| `rdx` | 第 3 引数 |
| `r10` | 第 4 引数 |
| `r8` | 第 5 引数 |
| `r9` | 第 6 引数 |
| **戻り値** | **`rax` に格納される** |

### syscall で破壊されるレジスタ

`syscall` 命令は以下のレジスタを **CPU が自動的に上書き** します：

- `rcx` ← 戻りアドレス（syscall 直後の RIP）
- `r11` ← RFLAGS の値

これらは syscall の「副作用」であり、プログラマが明示的にセットするものではありません。

## ユーザーモード vs カーネルモード

CPU には少なくとも 2 つの動作モードがあります：

![ユーザーモード⇔カーネルモード遷移](/images/v5/user-kernel-mode.drawio.svg)

- **ユーザーモード**: 通常のプログラムが実行されるモード。ハードウェアに直接アクセスできない
- **カーネルモード**: OS カーネルが実行されるモード。すべてのハードウェアにアクセス可能

> **Ring とは？** — x86 CPU は特権レベルを Ring 0〜Ring 3 の 4 段階で管理します。数字が小さいほど権限が高く、Ring 0 が最も強い特権（カーネル）、Ring 3 が最も弱い特権（ユーザープログラム）です。Ring 1・Ring 2 はドライバ用に設計されましたが、Linux を含む現代の OS はほぼ使わず、Ring 0 と Ring 3 の 2 段階だけで運用しています。

### モード・ランド・空間 — 同じ境界の 3 つの呼び方

OS の文脈では似た用語がいくつも出てきます。指している境界は同じですが、視点が違います。

| 用語 | 視点 | 意味 |
|------|------|------|
| ユーザー**モード** / カーネル**モード** | CPU | CPU が今 Ring 3 で動いているか Ring 0 で動いているか。ハードウェアの状態 |
| ユーザー**ランド** / カーネル**ランド** | ソフトウェア | そのモードで動く **プログラム群の総称**。ユーザーランド = シェル、ls、ブラウザなど。カーネルランド = カーネル本体、デバイスドライバなど |
| ユーザー**空間** / カーネル**空間** | メモリ | 仮想アドレス空間のどの領域か。Linux x86-64 では `0x0000000000000000`〜`0x00007FFFFFFFFFFF` がユーザー空間、`0xFFFF800000000000` 以降がカーネル空間 |

つまり「ユーザーランドのプログラムが、ユーザーモードで、ユーザー空間のメモリを使って動いている」——全部同じ側の話です。`syscall` を実行すると、CPU のモードが切り替わり、カーネルランドのコードが、カーネル空間のメモリを使って動き始めます。

`syscall` 命令を実行すると、CPU は以下を行います：
1. 現在の RIP を `rcx` に保存
2. 現在の RFLAGS を `r11` に保存
3. LSTAR MSR レジスタに登録されたカーネルのエントリポイントに RIP をセット
4. 特権レベルを Ring 0 に変更

### なぜスタックではなくレジスタに保存するのか

普通の `call` 命令は戻りアドレスをスタックに push します。しかし `syscall` はそうしません。理由は **ユーザーのスタックを信用できない** からです。

ユーザープログラムの `rsp` が不正なアドレスを指していたり、スタック領域が溢れていたりする可能性があります。もしそこに戻りアドレスを push しようとすれば、ページフォルトが発生し、カーネルに入る前に CPU が例外処理に飛ばされてしまいます。

そこで `syscall` はスタックを一切使わず、レジスタだけで状態を退避します：

- **`rcx` ← RIP**：カーネルから戻るとき `sysret` がこの値を RIP に復元する
- **`r11` ← RFLAGS**：同じく `sysret` がフラグを復元する

`rcx` と `r11` が選ばれた理由は、x86-64 の呼び出し規約でどちらも **caller-saved**（呼び出し側が退避する責任を持つ）レジスタだからです。syscall で壊れても、規約上はプログラム側が「壊れてもよい」と了承しているレジスタなので、既存の C コードとの互換性を保てます。

### LSTAR MSR とは

MSR（Model Specific Register）は汎用レジスタ（rax, rbx, ...）とは別の、**CPU の設定・制御用レジスタ群** です。`rdmsr` / `wrmsr` 命令でアクセスしますが、これらは **Ring 0 でしか実行できません**。

LSTAR は **Long System Target Address Register** の略で、MSR アドレス `0xC0000082` に割り当てられています。Linux カーネルは起動時にここに syscall ハンドラのアドレス（`entry_SYSCALL_64` 関数）を書き込みます。

```
起動時（カーネル）:   wrmsr(0xC0000082, entry_SYSCALL_64 のアドレス)
syscall 実行時（CPU）: RIP ← LSTAR の値（= entry_SYSCALL_64）
```

この設計の安全性のポイントは、ジャンプ先がハードウェアレベルで固定されていることです。ユーザープログラムは LSTAR を読むことも書くこともできないため、`syscall` のジャンプ先を改ざんすることは不可能です。

## なぜ syscall が存在するか

プログラムがハードウェアに直接アクセスできると：
- 他のプログラムのメモリを読み書きできてしまう
- ディスクの内容を破壊できてしまう
- 他のプロセスの I/O を横取りできてしまう

**カーネルが仲介者** となることで、安全性を保ちます。プログラムは「ファイルに書き込みたい」「メモリを確保したい」とカーネルに依頼し、カーネルが権限を確認してから実行します。

## sys_write — データを出力する

| 引数 | レジスタ | 値 |
|------|---------|-----|
| syscall# | rax | 1 (sys_write) |
| fd | rdi | 1=stdout, 2=stderr |
| buf | rsi | 書き込むデータのアドレス |
| count | rdx | 書き込むバイト数 |
| **戻り値** | **rax** | 書き込まれたバイト数 |

{{code:asm/hello.asm}}

## fd — sys_write の第 1 引数

`sys_write` の `rdi = 1` の「1」は何でしょうか。これは **ファイルディスクリプタ**（fd）という整数番号で、カーネルが管理する I/O リソースを指します。ここでは最低限だけ押さえておきます：

- `rdi = 1` → fd 1 = **stdout**（標準出力）
- `rdi = 2` → fd 2 = **stderr**（標準エラー出力）

`hello.asm` は `rdi = 1` で stdout に、`write_stderr.asm` は `rdi = 2` で stderr に書き込みます。端末上ではどちらも画面に表示されますが、出力先の fd が違います。

fd の仕組み（fd テーブル、"Everything is a file"、リダイレクト）は [v6 — ファイルディスクリプタ](#v6/ファイルディスクリプタ) で詳しく学びます。

{{code:asm/write_stderr.asm}}

## rax が戻り値で上書きされる

`multi_syscall.asm` の重要なポイント：

```nasm
mov rax, 1      ; rax = 1 (sys_write)
; ... (引数セット)
syscall          ; sys_write 実行後、rax = 書き込んだバイト数
; この時点で rax はもう 1 ではない！
mov rax, 60     ; 次の syscall のために rax を再セット
```

syscall は常に rax を戻り値で上書きします。次の syscall を呼ぶ前に、rax にシステムコール番号を再セットする必要があります。

{{code:asm/multi_syscall.asm}}

## プレビュー: カーネルに入る他の方法

syscall は **同期的・自発的** なカーネル遷移です。プログラムが意図的にカーネルを呼び出します。

カーネルに入る方法は他にもあります：
- **割り込み (interrupt)** — 外部デバイスからの非同期通知（キーボード、タイマーなど）
- **例外 (exception)** — 不正なメモリアクセス（SIGSEGV）やゼロ除算など

これらは v9 以降で学びます。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.2 SYSCALL](https://www.felixcloutier.com/x86/syscall) — RCX←RIP, R11←RFLAGS, RIP←IA32_LSTAR の動作
- [Intel SDM Vol.3 Ch.5 "Protection"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — Ring 0〜3 の特権レベル定義
- [`syscall(2)` man page](https://man7.org/linux/man-pages/man2/syscall.2.html) — x86-64 Linux の syscall 呼び出し規約: rax=番号, rdi/rsi/rdx/r10/r8/r9=引数
- [Linux kernel `syscall_64.tbl`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl) — sys_write=1, sys_exit=60
