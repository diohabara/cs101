`v18` では、OS なしの「ベアメタル」環境でプログラムを動かします。
QEMU 仮想マシン上でカーネルを起動し、UART シリアルポートに直接文字を出力します。

## 概要

`v18` は Phase 2（ベアメタルカーネル）の入口です。v9-v17 では Linux カーネルの上で C プログラムを動かしていましたが、ここからは OS そのものを作り始めます。

最初のステップは「何もない状態から文字を出す」こと。UART 16550A というシリアル通信デバイスの I/O ポートに 1 バイト書くだけで、ホストの端末に文字が表示されます。

## ベアメタルとは

「ベアメタル（bare metal）」は OS なしでハードウェア上で直接プログラムが動作する状態です。

```
v9-v17 の世界:                v18 以降の世界:
┌─────────────┐              ┌─────────────┐
│ ユーザープログラム │        │  カーネル     │ ← 自分で書く
├─────────────┤              ├─────────────┤
│  libc       │              │（なし）      │ ← printf も malloc もない
├─────────────┤              ├─────────────┤
│  Linux カーネル │           │（なし）      │ ← syscall も使えない
├─────────────┤              ├─────────────┤
│  ハードウェア  │            │  ハードウェア  │ ← 直接操作する
└─────────────┘              └─────────────┘
```

使えないもの: `printf`, `malloc`, `write`, `mmap` — これらはすべて OS が提供する機能です。使えるのは CPU 命令とハードウェアレジスタだけです。

## QEMU と起動の仕組み

QEMU の `-kernel` オプションは、フラットバイナリをアドレス `0x100000`（1MB）にロードして CPU をロングモード（64ビットモード）で起動します。

```
QEMU 起動シーケンス:
  1. QEMU が kernel.bin を読み込む
  2. メモリアドレス 0x100000 にバイナリを配置
  3. CPU をロングモード（64ビット）に設定
  4. RIP = 0x100000（_start）から実行開始
```

リンカスクリプトで `.text.boot` セクション（`_start` 関数）がバイナリの先頭に来るように配置します。

## UART 16550A

UART（Universal Asynchronous Receiver/Transmitter）はシリアル通信デバイスです。PC の COM1 ポートは I/O ポートアドレス `0x3F8` に固定されています。

### レジスタマップ

| オフセット | レジスタ | 用途 |
|-----------|---------|------|
| +0 | THR | 送信バッファ（ここに 1 バイト書くとシリアルポートから出力） |
| +1 | IER | 割り込み有効化 |
| +2 | FCR | FIFO 制御 |
| +3 | LCR | ライン制御（データビット数、パリティ、ストップビット） |
| +4 | MCR | モデム制御 |
| +5 | LSR | ラインステータス（送信バッファが空かどうか） |

![CPU → UART → Serial → Terminal の流れ](/images/v18/uart-io.drawio.svg)

### I/O ポートアクセス

x86 にはメモリとは別に「I/O ポート空間」があります。`in` / `out` 命令で 0x0000-0xFFFF の 16ビットアドレス空間にアクセスします。

```
メモリアクセス:         I/O ポートアクセス:
  mov [addr], val        out port, val
  mov val, [addr]        in val, port
```

C からは inline assembly で `in` / `out` 命令を呼びます。

### 送信の仕組み

1. LSR (Line Status Register) の bit 5 (THRE) をチェック — 送信バッファが空か確認
2. 空であれば THR (Transmit Holding Register) に 1 バイト書き込み
3. UART ハードウェアがシリアルラインに 1 ビットずつ送信

v15 の MMIO シミュレーションで学んだ「ステータスポーリング → コマンド書き込み」パターンそのものです。

## 実装

### uart.h — UART レジスタ定義と I/O 関数

{{code:src/uart.h}}

### uart.c — UART ドライバ

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | `outb(IER, 0x00)` | 割り込み無効化 |
| 2 | `outb(LCR, 0x80)` | DLAB=1 でボーレート設定モード |
| 3 | `outb(+0, 0x01)` | ボーレート 115200bps |
| 4 | `outb(LCR, 0x03)` | 8N1 モード設定 |
| 5 | `outb(FCR, 0xC7)` | FIFO 有効化 |
| 6 | `outb(MCR, 0x0B)` | DTR + RTS 有効化 |

{{code:src/uart.c}}

### boot.c — カーネルエントリポイント

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | `uart_init()` | シリアルポート初期化 |
| 2 | `uart_puts("Hello, bare metal!\n")` | 文字列送信 |
| 3 | `hlt` ループ | CPU 停止 |

{{code:src/boot.c}}

### linker.ld — リンカスクリプト

リンカスクリプトはバイナリのメモリレイアウトを定義します。`.text.boot` セクションを先頭に配置し、`_start` がエントリポイントになるようにします。

{{code:src/linker.ld}}

## ビルドと実行

```bash
# ビルド
clang -ffreestanding -nostdlib -target x86_64-unknown-none -c src/boot.c -o build/boot.o
clang -ffreestanding -nostdlib -target x86_64-unknown-none -c src/uart.c -o build/uart.o
ld.lld -T src/linker.ld --nostdlib build/boot.o build/uart.o -o build/kernel.bin

# 実行
qemu-system-x86_64 -kernel build/kernel.bin -serial stdio -display none
# 出力: Hello, bare metal!
```

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [16550 UART Datasheet](https://www.ti.com/lit/ds/symlink/pc16550d.pdf) — UART 16550A のレジスタマップと初期化手順
- [OSDev Wiki: Serial Ports](https://wiki.osdev.org/Serial_Ports) — COM1 の I/O ポートアドレスと初期化シーケンス
- [Intel SDM Vol.1 Ch.19 "Input/Output"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — x86 の I/O ポート空間と in/out 命令
- [QEMU Documentation: Direct Linux Boot](https://www.qemu.org/docs/master/system/linuxboot.html) — -kernel オプションのロードアドレスと動作
