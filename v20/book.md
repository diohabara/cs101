`v20` では、CPU の割り込み処理の仕組みを実装します。
IDT（Interrupt Descriptor Table）を構築し、CPU 例外のハンドラを登録します。

## 概要

`v20` は v19 の GDT（セグメント管理）に続いて、CPU の「割り込み」を扱います。割り込みとは、CPU が実行中のプログラムを一時停止して、特定のハンドラ関数にジャンプする仕組みです。

ゼロ除算、無効なメモリアクセス、ブレークポイント — これらが起きたとき CPU は何をすべきか？ IDT がその答えを持っています。

## 割り込みとは

CPU が通常の命令実行を中断して、別のコード（ハンドラ）を実行する仕組みです。

```
通常の実行:
  命令1 → 命令2 → 命令3 → 命令4 → ...

割り込みが起きた場合:
  命令1 → 命令2 → [割り込み発生!]
                       ↓
                  ハンドラを実行
                       ↓
                  (復帰 or 停止)
```

割り込みは 3 種類あります:

| 種類 | 発生元 | 例 |
|------|--------|-----|
| 例外（Exception） | CPU 内部 | ゼロ除算、ページフォルト |
| ハードウェア割り込み（IRQ） | 外部デバイス | キーボード入力、タイマー |
| ソフトウェア割り込み | int 命令 | int3（ブレークポイント）、int 0x80（旧 Linux syscall） |

## 例外の分類

CPU 例外は発生後の振る舞いで 3 つに分類されます:

| 分類 | 動作 | 例 |
|------|------|-----|
| Fault | RIP は例外を起こした命令を指す。修正して再実行できる | #PF（ページフォルト） |
| Trap | RIP は次の命令を指す。実行を継続できる | #BP（ブレークポイント） |
| Abort | 復帰不可能な致命的エラー | #DF（ダブルフォルト） |

#PF（ページフォルト）は Fault なので、OS がページを割り当てれば同じ命令を再実行できます。これが仮想メモリの基盤です。

## IDT の構造

IDT は 256 エントリのテーブルで、各エントリが「ベクタ番号 → ハンドラアドレス」の対応を持ちます。

```
IDT テーブル（メモリ上）:
  ┌──────────────────────────────────────┐
  │ Entry 0:  #DE ハンドラのアドレス      │ ← ゼロ除算
  ├──────────────────────────────────────┤
  │ Entry 1:  #DB ハンドラのアドレス      │ ← デバッグ例外
  ├──────────────────────────────────────┤
  │ Entry 2:  NMI ハンドラのアドレス      │ ← Non-Maskable Interrupt
  ├──────────────────────────────────────┤
  │ Entry 3:  #BP ハンドラのアドレス      │ ← ブレークポイント (int3)
  ├──────────────────────────────────────┤
  │ ...                                  │
  ├──────────────────────────────────────┤
  │ Entry 14: #PF ハンドラのアドレス      │ ← ページフォルト
  ├──────────────────────────────────────┤
  │ ...                                  │
  ├──────────────────────────────────────┤
  │ Entry 255: （未使用）                 │
  └──────────────────────────────────────┘
   各エントリ: 16 バイト（64ビットモード）
   合計: 256 × 16 = 4096 バイト = 4KB
```

### IDT ゲートディスクリプタ

各エントリ（16 バイト）の構造:

| バイト | フィールド | 説明 |
|--------|-----------|------|
| 0-1 | offset_low | ハンドラアドレスの下位 16 ビット |
| 2-3 | selector | コードセグメントセレクタ（0x08 = Kernel Code） |
| 4 | ist | Interrupt Stack Table（通常 0） |
| 5 | type_attr | タイプ + DPL + Present |
| 6-7 | offset_mid | ハンドラアドレスの中位 16 ビット |
| 8-11 | offset_high | ハンドラアドレスの上位 32 ビット |
| 12-15 | reserved | 予約（0） |

ハンドラアドレスが 3 つのフィールドに分割されているのは、32ビット時代の IDT フォーマットとの互換性のためです。

### type_attr バイト

```
  bit 7     bit 6-5    bit 4    bit 3-0
  ─────     ───────    ─────    ───────
  Present   DPL        0        Type
  1=有効    00=Ring0             0xE=割り込みゲート
            11=Ring3             0xF=トラップゲート
```

割り込みゲート（0xE）は割り込み処理中に IF フラグをクリアして、他の割り込みを禁止します。
トラップゲート（0xF）は IF フラグを変更しません。

この実装では `0x8E`（Present=1, DPL=0, 割り込みゲート）を使います。

## 割り込み発生時の CPU 動作

CPU が割り込みを検出すると、ハードウェアが自動的に以下を実行します:

```
1. 現在の SS, RSP をスタックにプッシュ
2. 現在の RFLAGS をプッシュ
3. 現在の CS, RIP をプッシュ
4. エラーコードがある例外なら、エラーコードもプッシュ
5. IDT[ベクタ番号] からハンドラアドレスを取得
6. CS = IDT エントリの selector、RIP = ハンドラアドレス にジャンプ

スタックの状態（ハンドラ開始時）:
  ┌─────────┐
  │ SS      │ ← 元のスタックセグメント
  ├─────────┤
  │ RSP     │ ← 元のスタックポインタ
  ├─────────┤
  │ RFLAGS  │ ← 元のフラグ
  ├─────────┤
  │ CS      │ ← 元のコードセグメント
  ├─────────┤
  │ RIP     │ ← 例外発生位置（Fault）or 次の命令（Trap）
  ├─────────┤
  │ Error   │ ← エラーコード（#PF 等のみ）
  └─────────┘ ← RSP はここを指す
```

## 実装

### idt.h — IDT 構造体定義

{{code:src/idt.h}}

### idt.c — IDT 構築と例外ハンドラ

idt_set_gate() でハンドラアドレスを 3 つのフィールドに分割して格納します:

| フィールド | ビット範囲 | 計算 |
|-----------|-----------|------|
| offset_low | 0-15 | `handler & 0xFFFF` |
| offset_mid | 16-31 | `(handler >> 16) & 0xFFFF` |
| offset_high | 32-63 | `(handler >> 32) & 0xFFFFFFFF` |

登録する例外ハンドラ:

| ベクタ | 名前 | エラーコード | 分類 | 説明 |
|--------|------|------------|------|------|
| 0 | #DE | なし | Fault | ゼロ除算 |
| 3 | #BP | なし | Trap | ブレークポイント（int3） |
| 14 | #PF | あり | Fault | ページフォルト |

`__attribute__((interrupt))` は GCC/Clang の拡張で、コンパイラが自動的に:
- 全レジスタの退避・復元コードを生成
- エラーコードの処理（#PF 等）
- `iretq` 命令での復帰

を行います。通常の関数呼び出し規約とは異なる ABI を使います。

{{code:src/idt.c}}

### boot.c — IDT テスト付き起動シーケンス

int3 命令は 1 バイトのオペコード `0xCC` です。この命令を実行すると CPU は:
1. IDT[3] を参照
2. exc_breakpoint() にジャンプ
3. ハンドラが "Exception: #BP (Breakpoint)" を出力して停止

{{code:src/boot.c}}

### gdt.h / gdt.c — GDT（v19 から継続）

{{code:src/gdt.h}}

{{code:src/gdt.c}}

### uart.h / uart.c — UART ドライバ（v18 から継続）

{{code:src/uart.h}}

{{code:src/uart.c}}

### linker.ld — リンカスクリプト（v18 から継続）

{{code:src/linker.ld}}

## ビルドと実行

```bash
# ビルド（-mgeneral-regs-only は __attribute__((interrupt)) に必要）
make -C v20

# 実行
qemu-system-x86_64 -kernel v20/build/kernel.bin -serial stdio -display none
# 出力:
#   Hello, bare metal!
#   GDT loaded: selectors 0x08(code) 0x10(data)
#   IDT loaded: 256 entries
#   Exception: #BP (Breakpoint)
```

## GDT → IDT の関係

GDT と IDT は協調して動作します:

```
GDT（v19 で設定済み）:
  Entry 1 (selector 0x08): Kernel Code — 64ビット実行可能

IDT（v20 で追加）:
  各エントリの selector = 0x08
  → 割り込みハンドラは GDT の Kernel Code セグメントで実行される
```

IDT エントリの `selector` フィールドが GDT のセレクタを参照することで、割り込みハンドラがカーネルモード（Ring 0）で実行されることを保証します。

## PlayStation との関連

PS4/PS5 のカーネルも起動時に IDT を設定し、ページフォルトハンドラで仮想メモリを管理しています。ゲーム開発で遭遇する「アクセス違反」エラーは、まさにこの #PF 例外が OS に報告されたものです。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3 Ch.6 "Interrupt and Exception Handling"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) — IDT の構造、ゲートディスクリプタ、割り込み処理フロー
- [AMD64 Architecture Programmer's Manual Vol.2 Ch.8 "Exceptions and Interrupts"](https://www.amd.com/en/support/tech-docs/amd64-architecture-programmers-manual-volumes-1-5) — 64ビットモードの IDT フォーマットと例外分類
- [OSDev Wiki: Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table) — IDT の実装ガイド
- [OSDev Wiki: Exceptions](https://wiki.osdev.org/Exceptions) — CPU 例外の一覧とエラーコード
- [GCC Documentation: x86 Function Attributes (interrupt)](https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html) — __attribute__((interrupt)) の仕様
