`v27` では、Intel VT-x (VMX) のハードウェアサポートを CPUID で検出し、VMXON 命令で VMX モードに入ります。
Phase 3（ハイパーバイザ編）の最初のステップとして、仮想化支援機能の基盤を構築します。

## 概要

Intel VT-x (Virtual Machine Extensions) は、CPU がハードウェアレベルで仮想化を支援する機能です。VT-x がなかった時代、仮想化ソフトウェア（VMware, QEMU など）は特権命令をソフトウェアでエミュレートする必要があり、パフォーマンスと実装の両面で困難がありました。

VT-x は CPU に「ホストモード」と「ゲストモード」の 2 つの動作モードを追加します。ゲストが特権命令を実行すると、CPU が自動的にホスト（ハイパーバイザ）に制御を返します。これを「VM exit」と呼びます。

## VT-x の検出

VT-x の対応状況は CPUID 命令で確認できます。

```
CPUID leaf=1 の結果:
  ECX[5] = 1 → VT-x 対応
  ECX[5] = 0 → VT-x 非対応

例: ECX = 0x00000020 (ビット 5 = 1) → VT-x 対応
    ECX = 0x00000000 (ビット 5 = 0) → VT-x 非対応
```

CPUID はソフトウェアから CPU の機能を問い合わせる標準的な方法です。leaf=1 はプロセッサの基本機能フラグを返します。

## VMX の有効化手順

VT-x が検出されても、すぐに VMX 命令を使えるわけではありません。以下の 5 ステップが必要です。

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | CPUID 確認 | CPU が VMX に対応しているか |
| 2 | IA32_FEATURE_CONTROL MSR | BIOS が VMX を許可しているか |
| 3 | CR4.VMXE 設定 | VMX 命令を有効化 |
| 4 | VMXON 領域準備 | 4KB アラインメント + リビジョン ID |
| 5 | VMXON 実行 | VMX ルート操作モードに入る |

### IA32_FEATURE_CONTROL MSR (0x3A)

BIOS が VMX の有効/無効を制御する MSR（Model Specific Register）です。

```
IA32_FEATURE_CONTROL (MSR 0x3A):
  bit 0: Lock     --- 1 = ロック済み（再起動まで変更不可）
  bit 2: VMXON    --- 1 = VMXON 命令を許可

状態の判定:
  Lock=0          → 未ロック。VMXON と Lock を設定してロックする
  Lock=1, VMXON=1 → VMX 使用可能
  Lock=1, VMXON=0 → BIOS が VMX を禁止。変更不可
```

### CR4.VMXE (ビット 13)

CR4 レジスタのビット 13 を 1 に設定すると、VMXON などの VMX 命令が有効になります。このビットが 0 のまま VMX 命令を実行すると #UD (Invalid Opcode) 例外が発生します。

### VMXON 領域

VMXON 命令には 4KB アラインメントされたメモリ領域が必要です。この領域の先頭 4 バイトに VMCS リビジョン ID を書き込みます。

```
VMXON 領域（4096 バイト）:
  +0x000: VMCS リビジョン ID (4バイト)   ← IA32_VMX_BASIC MSR から取得
  +0x004: データ (4092 バイト)           ← CPU が使用
```

リビジョン ID は IA32_VMX_BASIC MSR (0x480) のビット 30:0 から取得します。

## 実装

### vmx.h --- VMX 定義

{{code:src/vmx.h}}

### vmx_detect.c --- VT-x 検出と有効化

{{code:src/vmx_detect.c}}

### boot.c --- カーネルエントリポイント（v27: VMX 追加）

{{code:src/boot.c}}

![VMX モード遷移](/images/v27/vmx-modes.drawio.svg)

## VMX モードの遷移

```
通常モード
  │
  ├─ CR4.VMXE = 1 ──→ VMX 命令有効化
  │
  ├─ VMXON ──────────→ VMX ルート操作モード（ホスト）
  │                      │
  │                      ├─ VMLAUNCH/VMRESUME ──→ VMX 非ルートモード（ゲスト）
  │                      │                          │
  │                      │                          └─ VM exit ──→ ホストに復帰
  │                      │
  │                      └─ VMXOFF ──→ 通常モードに復帰
  │
  └─ CR4.VMXE = 0 ──→ VMX 命令無効化
```

v27 では VMXON → VMXOFF の部分を実装しました。v28 以降で VMCS の設定とゲスト実行に進みます。

## コンテナ環境での動作

Docker コンテナや VT-x 非対応の環境では、CPUID で VT-x が検出されません。これは正常な動作です。

```
VT-x 対応環境:
  "VT-x detected via CPUID"
  "VMX mode entered"
  "VMXON success"
  "VMX mode exited"
  "VMXOFF success"

VT-x 非対応環境:
  "VT-x not available (expected in container)"
```

テストはどちらの出力も正常としてパスします。

## PlayStation との関連

PS4/PS5 はハイパーバイザ（hypervisor）を使ってゲーム実行環境を分離しています。PS4 の場合、AMD-V（AMD の VT-x 相当）を使って Orbis OS カーネルとゲームプロセスを分離し、セキュリティとパフォーマンスの両方を確保しています。v27 で実装した VMX 検出と有効化は、そのハイパーバイザ初期化の最初のステップに相当します。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3C Ch.23 "Introduction to VMX Operation"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VMX の概要、VMXON/VMXOFF の仕様
- [Intel SDM Vol.3C Ch.23.7 "Enabling and Entering VMX Operation"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VMX 有効化の手順、IA32_FEATURE_CONTROL MSR
- [Intel SDM Vol.3D Appendix A "VMX Capability Reporting Facility"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- IA32_VMX_BASIC MSR、VMCS リビジョン ID
- [OSDev Wiki: VMX](https://wiki.osdev.org/VMX) --- VMX 有効化の実装例
