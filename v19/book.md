`v19` では、GDT（Global Descriptor Table）を初期化してメモリセグメントと特権レベルを設定します。
v18 で作ったベアメタルカーネルに、カーネルモード（Ring 0）とユーザーモード（Ring 3）を区別するための基盤を追加します。

## 概要

GDT は x86 CPU がメモリアクセスの権限を管理するためのテーブルです。すべてのメモリアクセスはセグメントセレクタを経由し、GDT のエントリが「このメモリアクセスは許可されるか」を決定します。

x86-64 のロングモードではセグメンテーションはほぼ無効化されていますが、特権レベル（Ring 0 = カーネル、Ring 3 = ユーザー）の切り替えには GDT が必須です。OS を作る上で最初に設定すべきハードウェア機構の一つです。

## GDT とは

GDT（Global Descriptor Table）はメモリ上に配置された配列で、各エントリ（8 バイト）が一つのセグメントの属性を定義します。CPU は `lgdt` 命令で GDT の場所とサイズを記憶し、以後すべてのメモリアクセスでセグメントセレクタを参照します。

```
メモリ上の GDT:
  Index 0 (0x00): ヌルディスクリプタ  --- CPU が要求（使用不可）
  Index 1 (0x08): カーネルコード      --- Ring 0, 実行可能
  Index 2 (0x10): カーネルデータ      --- Ring 0, 読み書き可
  Index 3 (0x18): ユーザーコード      --- Ring 3, 実行可能
  Index 4 (0x20): ユーザーデータ      --- Ring 3, 読み書き可
```

セレクタ値はインデックス * 8 で計算します。例えばインデックス 1 のセレクタは `0x08`、インデックス 2 は `0x10` です。

## GDT エントリの構造

各エントリは 8 バイトで、以下のフィールドを持ちます。

```
バイト位置:  7      6        5       4       3       2       1       0
           base_hi gran     access  base_mid         base_low       limit_low
           [31:24] [flags   [P|DPL  [23:16]          [15:0]         [15:0]
                   |lim_hi]  |S|Type]
```

### access バイト（ビット 47..40）

| ビット | 名前 | 意味 |
|--------|------|------|
| 7 | P (Present) | 1 = セグメントがメモリに存在する |
| 5-6 | DPL | 特権レベル: 0 = カーネル, 3 = ユーザー |
| 4 | S | 1 = コード/データセグメント, 0 = システムセグメント |
| 3 | E (Executable) | 1 = コードセグメント, 0 = データセグメント |
| 1 | RW | コード: 読み取り可, データ: 書き込み可 |

v19 で使う access 値:

| 値 | 2進数 | 意味 |
|----|-------|------|
| `0x9A` | `1001 1010` | P=1, DPL=0, S=1, Code+Readable |
| `0x92` | `1001 0010` | P=1, DPL=0, S=1, Data+Writable |
| `0xFA` | `1111 1010` | P=1, DPL=3, S=1, Code+Readable |
| `0xF2` | `1111 0010` | P=1, DPL=3, S=1, Data+Writable |

### granularity バイト（ビット 55..52）

上位 4 ビットがフラグ、下位 4 ビットがリミットの上位部分です。

| ビット | 名前 | 意味 |
|--------|------|------|
| 7 | G (Granularity) | 1 = リミットを 4K 単位で解釈（0xFFFFF * 4K = 4GB） |
| 6 | D/B | 1 = 32ビットセグメント |
| 5 | L (Long) | 1 = 64ビットコードセグメント（D=0 と併用） |

v19 で使う granularity 値:

| 値 | 2進数 | 意味 |
|----|-------|------|
| `0xA0` | `1010 0000` | G=1, D=0, L=1 --- 64ビットコードセグメント |
| `0xC0` | `1100 0000` | G=1, D=1, L=0 --- 32ビットデータセグメント |

![GDT エントリとセレクタ値](/images/v19/gdt-selectors.drawio.svg)

## 特権レベル（Ring 0 / Ring 3）

x86 CPU には 4 段階の特権レベル（Ring 0 ~ Ring 3）がありますが、実用上は Ring 0（カーネル）と Ring 3（ユーザー）の 2 段階を使います。

```
Ring 0 (カーネルモード):
  - すべての命令を実行可能（lgdt, cli, hlt, in/out ...）
  - すべてのメモリにアクセス可能
  - I/O ポートに直接アクセス可能

Ring 3 (ユーザーモード):
  - 特権命令は実行不可（実行すると #GP 例外）
  - カーネルメモリにアクセス不可
  - システムコールでカーネルに処理を依頼
```

GDT のセグメントセレクタに DPL（Descriptor Privilege Level）を設定することで、CPU がどの特権レベルで動作するかを制御します。

## 実装

### gdt.h --- GDT 構造体定義

{{code:src/gdt.h}}

### gdt.c --- GDT 初期化

`gdt_init()` の処理ステップ:

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | `gdt_set_entry(0, ...)` | ヌルディスクリプタを設定（CPU が要求） |
| 2 | `gdt_set_entry(1, ..., 0x9A, 0xA0)` | カーネルコードセグメント（Ring 0） |
| 3 | `gdt_set_entry(2, ..., 0x92, 0xC0)` | カーネルデータセグメント（Ring 0） |
| 4 | `gdt_set_entry(3, ..., 0xFA, 0xA0)` | ユーザーコードセグメント（Ring 3） |
| 5 | `gdt_set_entry(4, ..., 0xF2, 0xC0)` | ユーザーデータセグメント（Ring 3） |
| 6 | `lgdt` | GDT のアドレスとサイズを CPU に登録 |
| 7 | `lretq` | far return で CS をカーネルコードセレクタ (0x08) にリロード |
| 8 | `mov ax, 0x10` | DS/ES/FS/GS/SS をカーネルデータセレクタ (0x10) に設定 |

{{code:src/gdt.c}}

### boot.c --- カーネルエントリポイント（v18 + GDT 追加）

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | `uart_init()` | シリアルポート初期化 |
| 2 | `uart_puts("Hello, bare metal!\n")` | ブートメッセージ |
| 3 | `gdt_init()` | GDT 初期化 + セグメントレジスタ再設定 |
| 4 | `uart_puts("GDT loaded: ...")` | GDT ロード確認メッセージ |
| 5 | `hlt` ループ | CPU 停止 |

{{code:src/boot.c}}

### linker.ld --- リンカスクリプト

v18 と同一。GDT はデータセグメント (.bss) に配置されるため、リンカスクリプトの変更は不要です。

{{code:src/linker.ld}}

## lgdt とセグメントレジスタのリロード

`lgdt` 命令は GDT のアドレスとサイズを CPU の GDTR レジスタに登録するだけです。既存のセグメントレジスタ（CS, DS, SS など）は古い値のまま残ります。

```
lgdt 実行前:
  GDTR = (QEMU が設定した初期 GDT)
  CS = 古いセレクタ
  DS = 古いセレクタ

lgdt 実行後:
  GDTR = 新しい GDT を指す ← 更新された
  CS = 古いセレクタ         ← まだ古いまま!
  DS = 古いセレクタ         ← まだ古いまま!
```

CS は `mov` 命令では書き換えられません。far jump または far return でしか更新できません。v19 では `lretq`（far return）を使います。

```
CS リロード手順:
  1. pushq $0x08         --- 新しい CS 値をスタックに積む
  2. lea 1f(%rip), %rax  --- 戻り先アドレスを計算
  3. pushq %rax          --- 戻り先アドレスをスタックに積む
  4. lretq               --- pop RIP, pop CS → CS=0x08 で 1: に飛ぶ
  1:                      --- ここから CS=0x08 で実行

DS/ES/FS/GS/SS リロード:
  mov $0x10, %ax
  mov %ax, %ds    --- 各データセグメントレジスタに
  mov %ax, %es    --- カーネルデータセレクタ (0x10) を設定
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
```

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3A Ch.3 "Protected-Mode Memory Management"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- GDT の構造、セグメントディスクリプタのフォーマット、lgdt 命令
- [AMD64 Architecture Programmer's Manual Vol.2 Ch.4 "Segmentation"](https://www.amd.com/en/search/documentation/hub.html) --- ロングモードでのセグメンテーションの扱い
- [OSDev Wiki: GDT](https://wiki.osdev.org/GDT) --- GDT エントリの設定例とセグメントセレクタの計算
- [OSDev Wiki: GDT Tutorial](https://wiki.osdev.org/GDT_Tutorial) --- lgdt 後のセグメントレジスタリロード手順
