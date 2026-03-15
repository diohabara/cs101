`v21` では、PIT（Programmable Interval Timer）を使ってタイマー割り込みを実装します。
PIC の初期化と IRQ リマッピング、IDT の設定、ISR（Interrupt Service Routine）の実装を通じて、ハードウェア割り込みの基礎を学びます。

## 概要

`v21` は v19（GDT）の上に割り込み処理基盤を追加します。具体的には:

1. **IDT（Interrupt Descriptor Table）** --- 割り込みベクタとハンドラの対応表
2. **PIC（Programmable Interrupt Controller）** --- ハードウェア IRQ のリマッピング
3. **PIT（Programmable Interval Timer）** --- 定期的なタイマー割り込み

タイマー割り込みは OS の心臓部です。プリエンプティブマルチタスク、スケジューリング、時間管理 --- すべてタイマー割り込みが起点になります。

## 割り込みの仕組み

### 割り込みとは

CPU が現在の処理を一時中断し、特定のハンドラに制御を移す仕組みです。

```
通常の実行:
  命令A → 命令B → 命令C → 命令D → ...

割り込み発生時:
  命令A → 命令B → [割り込み!] → ISR 実行 → 命令C → 命令D → ...
                      ↑                    ↑
                  RIP/RFLAGS を         iretq で
                  スタックに保存         復帰
```

### 割り込みの種類

| 種類 | ベクタ | 例 |
|------|--------|-----|
| CPU 例外 | 0-31 | ゼロ除算 (0), ページフォールト (14), 一般保護例外 (13) |
| ハードウェア IRQ | 32-47 | タイマー (32), キーボード (33), COM1 (36) |
| ソフトウェア割り込み | 任意 | `int 0x80` (Linux syscall), `int3` (ブレークポイント) |

## IDT (Interrupt Descriptor Table)

IDT は 256 個の割り込みベクタとハンドラアドレスの対応表です。CPU が割り込みを受けると、IDT からハンドラアドレスを取得してジャンプします。

### IDT エントリ構造（16 バイト）

```
ビット 127..96  95..64       63..48    47..40     39..32  31..16   15..0
      reserved  offset_high  offset_mid type_attr  ist    selector offset_low
```

- `offset_low/mid/high`: ハンドラの 64 ビットアドレスを 3 分割
- `selector`: GDT のカーネルコードセレクタ（0x08）
- `type_attr`: 0x8E = Present + DPL=0 + Interrupt Gate

### idt.h --- IDT 構造体定義

{{code:src/idt.h}}

### idt.c --- IDT の初期化

256 エントリをデフォルトハンドラで初期化し、`lidt` でロードします。

{{code:src/idt.c}}

## PIC (8259A Programmable Interrupt Controller)

### IRQ リマッピングが必要な理由

デフォルトの PIC 設定では IRQ 0-7 がベクタ 8-15 にマッピングされています。しかしベクタ 0-31 は CPU 例外に予約されているため、IRQ が CPU 例外と衝突します。

```
デフォルト（衝突あり）:
  ベクタ  8: Double Fault (CPU例外) ← IRQ0 タイマー（衝突!）
  ベクタ  9: Coprocessor (CPU例外) ← IRQ1 キーボード（衝突!）

リマップ後（衝突なし）:
  ベクタ 32: IRQ0 タイマー
  ベクタ 33: IRQ1 キーボード
  ...
  ベクタ 39: IRQ7
```

### PIC 初期化シーケンス

ICW (Initialization Command Word) を順に送信してリマッピングを行います。

| ステップ | ポート | 値 | 意味 |
|----------|--------|------|------|
| ICW1 | 0x20, 0xA0 | 0x11 | 初期化開始、ICW4 必要 |
| ICW2 | 0x21 | 0x20 | マスター: IRQ 0-7 → ベクタ 32-39 |
| ICW2 | 0xA1 | 0x28 | スレーブ: IRQ 8-15 → ベクタ 40-47 |
| ICW3 | 0x21 | 0x04 | マスター: IRQ2 にスレーブ接続 |
| ICW3 | 0xA1 | 0x02 | スレーブ: カスケード ID = 2 |
| ICW4 | 0x21, 0xA1 | 0x01 | 8086 モード |
| マスク | 0x21 | 0xFE | IRQ0 のみ有効 |
| マスク | 0xA1 | 0xFF | スレーブ全マスク |

## PIT (Programmable Interval Timer)

PIT は水晶発振子に基づく 1193182 Hz のベースクロックを持ちます。除数を設定すると `1193182 / 除数` Hz でタイマー割り込みが発生します。

```
設定例:
  基本周波数: 1,193,182 Hz
  除数: 11,932
  結果: 1,193,182 / 11,932 ≈ 100 Hz（10ms ごと）
```

![PIT → PIC → CPU → Timer Handler の流れ](/images/v21/pit-timer.drawio.svg)

### PIT レジスタ

| ポート | 用途 |
|--------|------|
| 0x40 | チャンネル 0 データ（IRQ0 に接続） |
| 0x43 | コマンドレジスタ |

コマンドバイト 0x36 の内訳:
- bit 7-6: `00` = チャンネル 0
- bit 5-4: `11` = 下位→上位の順にアクセス
- bit 3-1: `011` = モード 3（方形波）
- bit 0: `0` = バイナリカウント

## ISR (Interrupt Service Routine)

### naked アセンブリスタブ

x86-64 の割り込みハンドラは特殊な呼び出し規約に従います。CPU が自動的に保存するのは RIP, CS, RFLAGS, RSP, SS だけです。汎用レジスタは ISR が明示的に保存する必要があります。

```
割り込み発生時のスタック（CPU が自動で push）:
  [RSP+32] SS
  [RSP+24] RSP (割り込み前)
  [RSP+16] RFLAGS
  [RSP+8]  CS
  [RSP+0]  RIP (割り込み前の戻り先)

ISR スタブが追加で保存:
  RAX, RCX, RDX, RDI, RSI, R8, R9, R10, R11
  （x86-64 ABI の caller-saved レジスタ）
```

`__attribute__((naked))` を使い、コンパイラがプロローグ/エピローグを生成しないようにします。全てのレジスタ保存と `iretq` を手動で制御します。

### timer.h --- タイマーインターフェース

{{code:src/timer.h}}

### timer.c --- PIC・PIT 初期化とタイマー ISR

{{code:src/timer.c}}

## boot.c --- エントリポイント

起動シーケンス:
1. UART 初期化
2. GDT ロード
3. IDT ロード
4. PIT + PIC 初期化（100Hz）
5. `sti` で割り込み有効化
6. `hlt` ループで 50 ティック待機
7. "50 ticks reached" を出力して停止

{{code:src/boot.c}}

## 実行結果

```
Hello, bare metal!
GDT loaded
IDT loaded
Timer started: 100Hz
10 ticks
20 ticks
30 ticks
40 ticks
50 ticks
50 ticks reached
```

100Hz で 50 ティック = 500ms 後に "50 ticks reached" が出力されます。

## 参考文献

- [Intel 8259A PIC Datasheet](https://pdos.csail.mit.edu/6.828/2005/readings/hardware/8259A.pdf) --- PIC の ICW/OCW シーケンスとリマッピング
- [Intel 8254 PIT Datasheet](https://www.scs.stanford.edu/10wi-cs140/pintos/specs/8254.pdf) --- PIT のチャンネル、モード、除数設定
- [OSDev Wiki: Interrupts](https://wiki.osdev.org/Interrupts) --- x86 割り込みの全体像
- [OSDev Wiki: IDT](https://wiki.osdev.org/IDT) --- IDT エントリの構造と lidt 命令
- [OSDev Wiki: PIC](https://wiki.osdev.org/PIC) --- 8259A PIC の初期化手順
- [OSDev Wiki: PIT](https://wiki.osdev.org/PIT) --- PIT の周波数設定とモード
- [Intel SDM Vol.3 Ch.6 "Interrupt and Exception Handling"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- IDT、割り込みゲート、ISR の仕様
