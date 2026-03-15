コンピュータの心臓部である CPU を、実際の x86-64 アセンブリプログラムを書いて動かしながら理解します。
`v1` では CPU の基本概念を丁寧に押さえ、GDB で 1 命令ずつ実行しながら状態の変化を観察します。

## 概要

`v1` は CPU の基本概念を実際の x86-64 アセンブリで体験する最初の段階です。レジスタ、RIP、システムコールの仕組みを `exit42.asm` と `regs.asm` で学び、`swap.asm` で mov のコピーセマンティクス、`arithmetic.asm` で add/sub、`flags.asm` で RFLAGS と条件ジャンプの導入を行います。

## この段階で押さえること

- CPU とは何か — 命令を順に処理する装置
- レジスタ — CPU 内部の超高速な記憶場所
- RIP（命令ポインタ）— 次に実行する命令のアドレス
- システムコール — プログラムが OS に頼む仕組み
- GDB で 1 命令ずつ実行し、状態遷移を観察する方法

## CPU とは何か

CPU（Central Processing Unit）は「命令を順に読んで実行する装置」です。

日常的なアナロジーで考えてみましょう：

- **レシピ（プログラム）**: やるべき手順が書かれた紙
- **料理人（CPU）**: レシピを 1 行ずつ読んで実行する人
- **メモ帳（レジスタ）**: 今使っている数値をすぐ書き留められる小さな紙
- **棚（メモリ）**: たくさんの材料を保管できるが、取りに行くのに時間がかかる場所

CPU は「メモ帳に数を書く」「メモ帳の数を足す」「棚から取ってくる」といった単純な操作を、1 秒間に数十億回繰り返しています。

## RISC と CISC

CPU の命令セットは大きく **RISC** と **CISC** という考え方で語られることがあります。

- **RISC (Reduced Instruction Set Computer)** — 命令の種類を絞り、単純な命令を高速に回す設計思想
- **CISC (Complex Instruction Set Computer)** — 1 命令でできることを増やし、豊かな命令を持つ設計思想

x86-64 は典型的には CISC 系に分類されます。`mov [addr], reg` のように、メモリアクセスやサイズ違いのオペランド、複雑なアドレッシングを 1 命令で表現できるからです。

ただし今の CPU は内部で x86 命令をより単純な内部命令へ分解して実行するため、「CISC = 遅い」「RISC = 速い」と単純には言えません。ここで大事なのは、**本教材で触る x86-64 は表現力が高く、その分だけ命令の読み方を丁寧に覚える必要がある** という点です。

## レジスタ

レジスタは CPU の内部にある超高速な記憶場所です。メモリ（RAM）と比べると：

- **速度**: レジスタへのアクセスは 1 サイクル。メモリは数百サイクルかかることもある
- **容量**: レジスタは数十個。メモリは数 GB〜数十 GB
- **名前**: レジスタには名前がついている（`rax`, `rbx` など）。メモリはアドレス（番号）で参照する

x86-64 の汎用レジスタ（64 ビット）：

| レジスタ | 用途の慣例 |
|----------|-----------|
| `rax` | 戻り値、syscall 番号 |
| `rbx` | 汎用（callee-saved） |
| `rcx` | カウンタ、第4引数 |
| `rdx` | データ、第3引数 |
| `rsi` | ソース、第2引数 |
| `rdi` | デスティネーション、第1引数 |
| `rsp` | スタックポインタ |
| `rbp` | ベースポインタ |
| `r8`〜`r15` | 汎用 |

なぜレジスタが存在するのでしょうか？ メモリは CPU から物理的に離れた場所にあり、アクセスに時間がかかります。レジスタは CPU チップの内部にあるため、即座にアクセスできます。「計算中の値はレジスタに置き、必要なときだけメモリに読み書きする」というのが基本的な考え方です。

## メモリ

メモリ（RAM）はバイト列の大きな配列です。各バイトにはアドレス（0, 1, 2, ...）が割り振られています。

プログラムのコード（命令列）もデータもメモリに置かれます。CPU はメモリから命令を読み取り（フェッチ）、実行します。

## RIP（命令ポインタ）

`RIP`（Register Instruction Pointer）は特別なレジスタで、**次に実行する命令のメモリアドレス** を保持しています。

CPU は毎サイクル：
1. `RIP` が指すアドレスから命令を読む
2. 命令を実行する
3. `RIP` を次の命令のアドレスに進める

この「読む → 実行する → 進める」のループが **Fetch-Decode-Execute サイクル** です。

![Fetch-Decode-Execute サイクル](/images/v1/fetch-decode-execute.drawio.svg)

## プログラムの書き方

アセンブリソース (`.asm`) から実行可能ファイルができるまでの流れ：

![asm→nasm→.o→ld→実行ファイルのパイプライン](/images/v1/asm-pipeline.drawio.svg)

1. **アセンブラ（nasm）**: 人が読めるニーモニック（`mov`, `add` など）を機械語のバイト列に変換する
2. **オブジェクトファイル（.o）**: 機械語 + メタデータ。まだアドレスが確定していない
3. **リンカ（ld）**: アドレスを確定し、OS が実行できる形式（ELF）にまとめる

```bash
nasm -f elf64 -g -F dwarf hello.asm -o hello.o   # アセンブル（デバッグ情報付き）
ld hello.o -o hello                                # リンク
./hello                                            # 実行
```

`-g -F dwarf` はデバッグ情報を埋め込むオプションです。これにより GDB でソースコードの行番号やラベル名を使ってデバッグできます。

## システムコール

ユーザープログラムは直接ハードウェアを操作できません。画面に文字を出す、ファイルを読む、プログラムを終了する — これらはすべて OS（カーネル）に頼む必要があります。

この「カーネルに頼む」仕組みが **システムコール** です。x86-64 Linux では `syscall` 命令を使います：

| レジスタ | 役割 |
|----------|------|
| `rax` | システムコール番号 |
| `rdi` | 第 1 引数 |
| `rsi` | 第 2 引数 |
| `rdx` | 第 3 引数 |

例えば `sys_exit`（プログラム終了）のシステムコール番号は **60** です。

## プログラム 1: exit42.asm

最初のプログラムは「exit code 42 で終了する」だけの最小プログラムです。

{{code:asm/exit42.asm}}

### 最初の数行は何をしているのか

最初は `mov` や `syscall` よりも、むしろファイル冒頭の
`section .text`、`global _start`、`_start:` が分かりにくいはずです。ここは「おまじない」として流さず、役割を押さえておく方が後で楽になります。

- `section .text`
  命令を置く領域を選ぶ指示です。`.text` は「実行されるコードの場所」で、CPU が読む命令列はここに入ります。今後 `.data` なら初期値付きデータ、`.bss` なら未初期化領域、というように別のセクションも出てきます。
- `global _start`
  `_start` という名前を「このファイルの外から見えるシンボル」として公開する指示です。これがないと、リンカ `ld` は実行開始位置として `_start` を見つけられません。
- `_start:`
  ラベルです。「ここに `_start` という名前をつける」という意味で、実際にはこの行のアドレスが `_start` になります。プログラム起動時、OS はこの位置から命令を実行し始めます。

要するに、冒頭の 3 行は「この先は実行コードであり、その入口は `_start` である」とアセンブラとリンカに伝えています。

### GDB で 1 命令ずつ実行する

GDB（GNU Debugger）を使って、CPU がこのプログラムを実行するとき何が起きているかを見てみましょう。

```bash
gdb ./build/exit42
(gdb) break _start        # プログラムの先頭で停止
(gdb) run                 # 実行開始（_start で停止する）
(gdb) info registers rax rdi rip   # レジスタの値を表示
(gdb) stepi               # 1 命令だけ実行
(gdb) info registers rax rdi rip   # 変化を確認
```

### 状態遷移表

| Step | 命令 | rax | rdi | rip | 説明 |
|------|------|-----|-----|-----|------|
| 0 | (実行前) | 0 | 0 | 0x401000 | 全レジスタがゼロで開始 |
| 1 | `mov rax, 60` | 60 | 0 | 0x401007 | rax にシステムコール番号をセット |
| 2 | `mov rdi, 42` | 60 | 42 | 0x40100e | rdi に終了コードをセット |
| 3 | `syscall` | — | — | — | カーネルに制御を渡し、プロセス終了 |

注目ポイント：
- **rip が自動的に進んでいる**: 各命令の実行後、rip は次の命令のアドレスを指す
- **mov は代入**: `mov rax, 60` は「rax に 60 を入れる」。他のレジスタは変わらない
- **syscall でプロセスが消える**: `sys_exit` を呼ぶとプログラムは終了し、GDB に制御が戻る

### GDB セッション出力

```gdb
(gdb) break _start
Breakpoint 1 at 0x401000
(gdb) run
Starting program: /workspace/v1/build/exit42

Breakpoint 1, 0x0000000000401000 in _start ()
(gdb) info registers rax rdi rip
rax            0x0                 0
rdi            0x0                 0
rip            0x401000            0x401000 <_start>
(gdb) stepi
0x0000000000401007 in _start ()
(gdb) info registers rax rdi rip
rax            0x3c                60
rdi            0x0                 0
rip            0x401007            0x401007 <_start+7>
(gdb) stepi
0x000000000040100e in _start ()
(gdb) info registers rax rdi rip
rax            0x3c                60
rdi            0x2a                42
rip            0x40100e            0x40100e <_start+14>
```

`0x3c` は 60 の 16 進表現、`0x2a` は 42 の 16 進表現です。GDB はデフォルトで 16 進数を使いますが、`print /d $rax` で 10 進表示もできます。

## プログラム 2: regs.asm

次のプログラムは複数のレジスタに値をロードし、CPU がそれぞれの「メモ帳」を独立して持っていることを確認します。

{{code:asm/regs.asm}}

### 状態遷移表

| Step | 命令 | rax | rbx | rcx | rdx | rip |
|------|------|-----|-----|-----|-----|-----|
| 0 | (実行前) | 0 | 0 | 0 | 0 | 0x401000 |
| 1 | `mov rax, 1` | 1 | 0 | 0 | 0 | 0x401007 |
| 2 | `mov rbx, 2` | 1 | 2 | 0 | 0 | 0x40100e |
| 3 | `mov rcx, 3` | 1 | 2 | 3 | 0 | 0x401015 |
| 4 | `mov rdx, 4` | 1 | 2 | 3 | 4 | 0x40101c |
| 5 | `mov rax, 60` | 60 | 2 | 3 | 4 | 0x401023 |
| 6 | `mov rdi, 0` | 60 | 2 | 3 | 4 | 0x40102a |
| 7 | `syscall` | — | — | — | — | — |

注目ポイント：
- **各レジスタは独立**: `mov rbx, 2` は rbx だけを変える。rax の値 1 はそのまま
- **rax が上書きされる**: Step 5 で `mov rax, 60` により、rax の値 1 が 60 に変わる。古い値は失われる
- **レジスタの数は限られている**: 終了処理のために rax を使い回す必要がある

## プログラム 3: swap.asm

`mov` がコピー操作であることを、レジスタの値を交換するプログラムで確認します。

{{code:asm/swap.asm}}

### なぜ直接交換できないのか

x86-64 には「2 つのレジスタの値を同時に入れ替える」命令はありません（`xchg` 命令は存在しますが、内部的には同様の処理です）。
`mov` はコピー操作なので、`mov rax, rbx` を実行すると rax の元の値が失われます。
そこで一時レジスタ（rcx）を使って 3 ステップで交換します。

### 状態遷移表

| Step | 命令 | rax | rbx | rcx |
|------|------|-----|-----|-----|
| 0 | (初期状態) | 0 | 0 | 0 |
| 1 | `mov rax, 5` | 5 | 0 | 0 |
| 2 | `mov rbx, 10` | 5 | 10 | 0 |
| 3 | `mov rcx, rax` | 5 | 10 | 5 |
| 4 | `mov rax, rbx` | 10 | 10 | 5 |
| 5 | `mov rbx, rcx` | 10 | 5 | 5 |

注目ポイント：
- **Step 3**: rcx に rax の値をコピー（rax は変わらない）
- **Step 4**: rax を rbx で上書き。このとき rax=5 は失われるが、rcx=5 にバックアップ済み
- **Step 5**: rbx に rcx（元の rax）をコピーして交換完了

## プログラム 4: arithmetic.asm

`add`（加算）と `sub`（減算）を 1 つのプログラムで体験します。

{{code:asm/arithmetic.asm}}

### add と sub

| 命令 | 動作 | 例 |
|------|------|----|
| `add dst, src` | dst = dst + src | `add rax, 5` → rax += 5 |
| `sub dst, src` | dst = dst - src | `sub rax, 2` → rax -= 2 |

どちらもフラグ（ZF, SF など）を更新します。`mov` はフラグを変えませんが、`add`/`sub` は計算結果に応じてフラグを設定します。

### 状態遷移表

| Step | 命令 | rax |
|------|------|-----|
| 0 | (初期状態) | 0 |
| 1 | `mov rax, 10` | 10 |
| 2 | `add rax, 5` | 15 |
| 3 | `sub rax, 2` | 13 |

## プログラム 5: flags.asm

CPU は計算結果について「ゼロになったか」「負になったか」をフラグとして記録します。
このフラグを条件ジャンプ命令で読み取ることで、プログラムの実行経路を変えることができます。

{{code:asm/flags.asm}}

### RFLAGS レジスタ

x86-64 の RFLAGS レジスタには、算術演算の結果に応じて自動的に設定されるフラグがあります。

| フラグ | 名前 | セット条件 |
|--------|------|-----------|
| ZF | Zero Flag | 結果が 0 のとき |
| SF | Sign Flag | 結果の最上位ビットが 1 のとき（負の値） |

`sub rax, 7` で rax=7-7=0 になると ZF=1 が設定されます。`jz .zero` は ZF=1 のときにジャンプします。

### 状態遷移表

| Step | 命令 | rax | ZF | RIP の動き |
|------|------|-----|----|-----------|
| 0 | (初期状態) | 0 | 0 | 0x401000 |
| 1 | `mov rax, 7` | 7 | 0 | 順方向 |
| 2 | `sub rax, 7` | 0 | **1** | 順方向 |
| 3 | `jz .zero` | 0 | 1 | **前方ジャンプ**（.zero へ） |
| 4 | `mov rdi, 0` | 0 | 1 | 順方向 |

注目ポイント：
- **ZF は `sub` で自動的にセットされる**: プログラマが明示的にフラグを操作する必要はない
- **`jz` はフラグを読むだけ**: ジャンプするかどうかは直前の演算結果で決まる
- **v2 ではこのパターンを発展させる**: `cmp` + 条件分岐でループや最大値の計算を行う

## まとめ

| 概念 | 一言 |
|------|------|
| CPU | 命令を順に実行する状態機械 |
| レジスタ | CPU 内部の超高速な名前付き記憶場所 |
| RIP | 次に実行する命令を指す特別なレジスタ |
| mov | レジスタに値を書き込む（コピー操作） |
| add / sub | 加算・減算。結果に応じてフラグを更新する |
| RFLAGS | 演算結果の性質（ゼロ、負など）を記録するレジスタ |
| syscall | カーネルに処理を依頼する命令 |
| GDB | CPU の状態を 1 命令ずつ観察するためのツール |

次の [v2](#v2) では、`cmp` 命令と条件分岐（`jz`/`jnz`/`jg`）で制御フローを学びます。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.1 §3.4.1 "General-Purpose Registers"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — 16 本の汎用レジスタ（RAX〜R15）の定義
- [Intel SDM Vol.1 §3.5 "Instruction Pointer"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — RIP が「次に実行する命令のオフセット」を保持すること
- [Intel SDM Vol.2 MOV](https://www.felixcloutier.com/x86/mov) — MOV の動作と Flags Affected: "None"（フラグを変更しない）
- [Intel SDM Vol.2 ADD](https://www.felixcloutier.com/x86/add) — `DEST := DEST + SRC`、OF/SF/ZF/AF/PF/CF を更新
- [Intel SDM Vol.2 SUB](https://www.felixcloutier.com/x86/sub) — `DEST := DEST - SRC`、OF/SF/ZF/AF/PF/CF を更新
- [Intel SDM Vol.1 §3.4.3 "EFLAGS Register"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — ZF: 結果が 0 のときセット、SF: 結果の MSB にセット
- [Intel SDM Vol.2 SYSCALL](https://www.felixcloutier.com/x86/syscall) — 64-bit モードでのシステムコール命令
- [Linux kernel `syscall_64.tbl`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl) — sys_exit = 60
