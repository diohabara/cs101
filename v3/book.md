CPU はレジスタだけでは動きません。
`v3` では、CPU がメモリ上の値を読み書きするとはどういうことかを学びます。

## 概要

`v3` は CPU がメモリへ値を退避し、必要になったら再び読み出す流れを扱います。
ここで初めて「CPU の外にある状態」が明示的に登場します。

x86-64 の実アセンブリ（NASM Intel 構文）で `.data` セクションに変数を定義し、`mov [addr], reg`（store）と `mov reg, [addr]`（load）を使ってレジスタとメモリ間のデータ移動を体験します。配列アクセスではベースアドレス + インデックス * 要素サイズのアドレッシングモードも使います。

## この段階で押さえること

- レジスタは速い作業台、メモリは大きな保管場所
- load/store は CPU とメモリの橋渡し
- アドレスを間違えると「どこを見ているか」が壊れる

## 理論

CPU の内部レジスタは数が少なく、長く値を置いておくには向きません。
そこで、より大きな保存場所としてメモリを使います。CPU はアドレスを指定してメモリセルを読み出したり、逆にレジスタの値を書き込んだりします。

![レジスタとメモリの関係](/images/v3/register-memory.drawio.svg)

## メモリ — .data セクションと .bss セクション

x86-64 のプログラムは、メモリ上にいくつかのセクションを持ちます。

- **`.data` セクション** — 初期値ありの変数を置く場所。`dq 0` のように定義する
- **`.bss` セクション** — 初期値なし（ゼロ初期化される）の変数を置く場所。`resq 1` のように領域だけ確保する

`dq` は "define quad-word" の略で、8 バイト（64 ビット）の領域を確保します。

| 擬似命令 | サイズ | 説明 |
|---------|-------|------|
| `db`    | 1 バイト | define byte |
| `dw`    | 2 バイト | define word |
| `dd`    | 4 バイト | define double-word |
| `dq`    | 8 バイト | define quad-word |

## リトルエンディアン

x86 アーキテクチャはリトルエンディアン方式でデータを格納します。
これは、複数バイトの値を「下位バイトから先に」メモリに並べるという意味です。

たとえば `0x0000002A`（10 進で 42）を 4 バイトで格納すると：

| アドレス | +0   | +1   | +2   | +3   |
|---------|------|------|------|------|
| 値      | `2A` | `00` | `00` | `00` |

人間が読む「左が上位」とは逆順になります。GDB でメモリを表示するときは、この点に注意してください。

## store — `mov [addr], reg`

レジスタの値をメモリに書き込む操作を **store** と呼びます。

`[addr]` の `addr` は `.data` セクションで定義したラベル名です。以下の例では `value` というラベルを使います。

```nasm
mov rax, 42        ; レジスタに値をセット
mov [value], rax   ; メモリ番地 value に rax の値を書き込む
```

`[value]` のように角括弧で囲むと「そのラベルが指すメモリアドレスの中身」を意味します。

### 状態遷移表（store）

| ステップ | 命令               | RAX | メモリ[value] |
|---------|--------------------|-----|---------------|
| 0       | （初期状態）        | 0   | 0             |
| 1       | `mov rax, 42`      | 42  | 0             |
| 2       | `mov [value], rax` | 42  | 42            |

## load — `mov reg, [addr]`

メモリの値をレジスタに読み込む操作を **load** と呼びます。

同じく `addr` にはラベル名が入ります。

```nasm
mov rbx, [value]   ; メモリ番地 value の値を rbx に読み込む
```

### 状態遷移表（load）

| ステップ | 命令               | RBX | メモリ[value] |
|---------|--------------------|-----|---------------|
| 0       | （store 完了後）    | 0   | 42            |
| 1       | `mov rbx, 0`      | 0   | 42            |
| 2       | `mov rbx, [value]` | 42  | 42            |

{{code:asm/store_load.asm}}

## 配列のアクセス

配列の要素にアクセスするには、ベースアドレス + インデックス * 要素サイズ を計算します。

```nasm
lea rsi, [array]       ; rsi = 配列の先頭アドレス
add rax, [rsi + rcx*8] ; rax += array[rcx]（8 バイト単位）
```

x86-64 のアドレッシングモードは `[base + index*scale + displacement]` 形式で、scale は 1, 2, 4, 8 のいずれかです。`dq` で定義した配列は要素サイズが 8 バイトなので `rcx*8` を使います。

ループ内で使っている `inc rcx` は、`rcx` の値を 1 だけ増やす命令です。`add rcx, 1` と同じ意味ですが、`inc` は「1 を足す」専用の命令なのでより短く書けます。

### 状態遷移表（array_sum）

| ステップ | RCX (index) | RAX (sum) | アクセスアドレス        | 読み出し値 |
|---------|-------------|-----------|----------------------|-----------|
| 0       | 0           | 0         | —                    | —         |
| 1       | 0           | 10        | `array + 0*8`        | 10        |
| 2       | 1           | 30        | `array + 1*8`        | 20        |
| 3       | 2           | 60        | `array + 2*8`        | 30        |
| 4       | 3           | 100       | `array + 3*8`        | 40        |

{{code:asm/array_sum.asm}}

## GDB でメモリを確認する — `x/1xg`

GDB の `x` コマンドでメモリの中身を直接確認できます。

```
x/1xg &value
```

- `1` — 表示する単位数
- `x` — 16 進表示
- `g` — giant word（8 バイト単位）

store の直後に実行すれば、メモリに正しく書き込まれたことを目視で確認できます。
`gdb/memory_trace.gdb` スクリプトで `stepi` しながらレジスタとメモリの両方を追えます。

## メモリの値を変更する

load → 計算 → store の 3 ステップで、メモリ上の値を更新できます。

### 状態遷移表（memory_modify）

| ステップ | 命令                  | RAX | メモリ[counter] |
|---------|-----------------------|-----|-----------------|
| 0       | （初期状態）           | 0   | 5               |
| 1       | `mov rax, [counter]`  | 5   | 5               |
| 2       | `add rax, 3`          | 8   | 5               |
| 3       | `mov [counter], rax`  | 8   | 8               |

{{code:asm/memory_modify.asm}}

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.1 §4.1 "Fundamental Data Types"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — Quadword = 64 bits (8 bytes)
- [AMD64 APM Vol.1 §2.2.1 "Byte Ordering"](https://developer.amd.com/resources/developer-guides-manuals/) — x86 はリトルエンディアン
- [Intel SDM Vol.2 §2.1.5 "Addressing-Mode Encoding"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — SIB バイトの scale は 1/2/4/8 の 4 値
- [NASM Manual §3.2 "Pseudo-Instructions"](https://www.nasm.us/doc/nasmdoc3.html) — DB/DW/DD/DQ の定義
- [ELF Specification (System V ABI)](https://refspecs.linuxfoundation.org/LSB_1.1.0/gLSB/specialsections.html) — `.bss` は SHT_NOBITS、ゼロ初期化が保証される
