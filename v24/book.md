`v24` では、syscall/sysret 命令を設定し、ユーザーモード遷移の基盤を構築します。
v23 までに構築した GDT、IDT、タイマー、PMM、ページングの上に、システムコール機構を追加します。

## 概要

syscall/sysret は x86-64 で最も高速なカーネル呼び出し方法です。ユーザーモード (Ring 3) のプログラムが OS のサービス（ファイル読み書き、メモリ確保など）を利用するとき、syscall 命令でカーネルに制御を移し、処理が終わったら sysret でユーザーモードに戻ります。

v24 では MSR（Model Specific Register）を設定して syscall 命令のエントリポイントを登録し、Ring 0 から syscall を呼び出して動作を確認します。

![syscall/sysret フロー](/images/v24/syscall-flow.drawio.svg)

## syscall/sysret の仕組み

### syscall 命令の動作

syscall 命令を実行すると、CPU は以下の処理を自動的に行います。

```
syscall 実行時の CPU 動作:
  1. RCX ← RIP        次の命令のアドレスを保存
  2. R11 ← RFLAGS     現在のフラグを保存
  3. RFLAGS &= ~SFMASK  指定ビットをクリア（IF=0 で割り込み無効化）
  4. CS  ← STAR[47:32]  カーネルコードセレクタ (0x08)
  5. SS  ← STAR[47:32] + 8  カーネルデータセレクタ (0x10)
  6. RIP ← LSTAR       ハンドラアドレスにジャンプ
```

### sysret 命令の動作

sysret 命令は syscall の逆操作です。

```
sysret 実行時の CPU 動作:
  1. RIP    ← RCX      保存した戻り先アドレスを復元
  2. RFLAGS ← R11      保存したフラグを復元
  3. CS     ← STAR[63:48] + 16  ユーザーコードセレクタ
  4. SS     ← STAR[63:48] + 8   ユーザーデータセレクタ
```

### MSR の設定

syscall/sysret を使うには 3 つの MSR を設定する必要があります。

| MSR | アドレス | 役割 |
|-----|----------|------|
| STAR | 0xC0000081 | CS/SS セレクタの指定 |
| LSTAR | 0xC0000082 | syscall エントリポイントのアドレス |
| SFMASK | 0xC0000084 | syscall 時に RFLAGS からクリアするビット |

STAR の設定値:

```
STAR レジスタ (64ビット):
  ビット 63:48 = SYSRET ベースセレクタ (0x0010)
    → CS = 0x0010 + 16 = 0x0020 (ユーザーコード)
    → SS = 0x0010 + 8  = 0x0018 (ユーザーデータ)
  ビット 47:32 = SYSCALL セレクタ (0x0008)
    → CS = 0x0008 (カーネルコード)
    → SS = 0x0008 + 8 = 0x0010 (カーネルデータ)
```

## TSS (Task State Segment)

TSS はユーザーモードからカーネルモードに遷移するとき（syscall、割り込み）に CPU がカーネルスタックのアドレスを知るために必要な構造体です。

```
TSS の主要フィールド:
  RSP0: Ring 0 用スタックポインタ
    → ユーザーモードから割り込みが発生したとき、
       CPU はこのアドレスをスタックポインタにセットする
  IST[1-7]: Interrupt Stack Table
    → 特定の割り込み用の専用スタック（オプション）
  IOPB: I/O Permission Bitmap のオフセット
    → ユーザーモードで許可する I/O ポートを制御
```

v24 では TSS を初期化しますが、GDT への TSS ディスクリプタ登録と LTR 命令は v26 の統合で行います。

## syscall レジスタ規約

Linux の syscall ABI に準拠したレジスタ規約を使います。

| レジスタ | 用途 |
|----------|------|
| RAX | システムコール番号 |
| RDI | 第 1 引数 |
| RSI | 第 2 引数 |
| RDX | 第 3 引数 |
| RCX | 戻り先 RIP（CPU が自動保存） |
| R11 | RFLAGS（CPU が自動保存） |

サポートするシステムコール:

| 番号 | 名前 | 引数 | 動作 |
|------|------|------|------|
| 1 | sys_write | buf, unused, len | UART にバッファの内容を出力 |
| 60 | sys_exit | code | 終了コードを表示して停止 |

## 実装

### usermode.h --- TSS 構造体と関数宣言

{{code:src/usermode.h}}

### usermode.c --- MSR 設定と TSS 初期化

{{code:src/usermode.c}}

### syscall_handler.c --- syscall エントリポイントとディスパッチャ

{{code:src/syscall_handler.c}}

### boot.c --- カーネルエントリポイント（v24）

{{code:src/boot.c}}

### gdt.h / gdt.c --- GDT（v19 から継続）

{{code:src/gdt.h}}

{{code:src/gdt.c}}

### idt.h / idt.c --- IDT（v20 から継続）

{{code:src/idt.h}}

{{code:src/idt.c}}

### timer.h / timer.c --- PIT タイマー（v21 から継続）

{{code:src/timer.h}}

{{code:src/timer.c}}

### pmm.h / pmm.c --- 物理メモリマネージャ（v22 から継続）

{{code:src/pmm.h}}

{{code:src/pmm.c}}

### paging.h / paging.c --- ページング（v23 から継続）

{{code:src/paging.h}}

{{code:src/paging.c}}

### linker.ld --- リンカスクリプト

{{code:src/linker.ld}}

## syscall エントリの naked 属性

`syscall_entry` 関数には `__attribute__((naked))` を使っています。通常のC関数ではコンパイラがプロローグ（スタックフレーム設定）とエピローグ（スタック復元 + ret）を自動生成しますが、syscall ハンドラは sysretq で戻るため、通常の ret は使えません。

```
通常のC関数:
  push %rbp           ← コンパイラ生成（プロローグ）
  mov %rsp, %rbp
  ...関数本体...
  pop %rbp            ← コンパイラ生成（エピローグ）
  ret                 ← コンパイラ生成

naked 関数:
  ...手書きのアセンブリのみ...
  sysretq             ← 手動で戻る
```

## PlayStation との関連

PS4/PS5 の Orbis OS カーネルも syscall/sysret を使ってユーザーモードのゲームプロセスとカーネルを切り替えます。ゲームが OS のサービス（メモリ確保、ファイル I/O、GPU コマンド送信）を利用するたびに syscall が呼ばれ、カーネルが処理を行って sysret で戻ります。v24 で構築した仕組みは、PS4/PS5 のシステムコールパスと本質的に同じ機構です。

## 参考文献

- [AMD64 Architecture Programmer's Manual Vol.2 Ch.6 "System-Call Extension"](https://www.amd.com/en/search/documentation/hub.html) --- syscall/sysret 命令の仕様、MSR の設定
- [Intel SDM Vol.3 Ch.5 "SYSCALL/SYSRET"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- Intel 版の syscall 仕様
- [OSDev Wiki: SYSENTER](https://wiki.osdev.org/SYSENTER) --- syscall/sysenter の比較と設定例
- [OSDev Wiki: TSS](https://wiki.osdev.org/TSS) --- TSS の構造と用途
