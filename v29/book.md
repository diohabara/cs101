`v29` では、vmlaunch 命令でゲスト VM を実行します。
v28 で設定した VMCS を使い、最小限のゲストコード（`mov rax, 0x42; hlt`）を実行して VM exit をハンドリングします。

## 概要

vmlaunch はゲスト VM の実行を開始する命令です。CPU はゲスト状態（VMCS に設定した RIP, RSP, CR0 等）に切り替わり、ゲストコードを実行します。ゲストが HLT などの特権命令を実行すると VM exit が発生し、ホスト（ハイパーバイザ）に制御が返ります。

## ゲストコード

v29 のゲストは最小限のマシンコードです。

```
マシンコード       アセンブリ         意味
48 C7 C0 42 00 00 00  mov rax, 0x42    RAX レジスタに 0x42 を設定
F4                    hlt              CPU を停止 → VM exit 発生
```

ゲストコードは通常のメモリ上のバイト列として配置します。VMCS の Guest RIP にこのアドレスを設定すると、vmlaunch 後に CPU がここから実行を開始します。

## vmlaunch の動作

```
vmlaunch 実行前:
  CPU 状態 = ホスト（ハイパーバイザ）

vmlaunch 成功:
  CPU 状態 = VMCS のゲスト状態にロード
  RIP = Guest RIP (ゲストコードの先頭)
  RSP = Guest RSP (ゲストスタック)
  ゲストコードが実行される

ゲストが HLT を実行:
  VM exit 発生
  CPU 状態 = VMCS のホスト状態に復元
  RIP = Host RIP (vmexit ハンドラ)
  VM exit reason = 12 (HLT)
```

### vmlaunch vs vmresume

| 命令 | 用途 | VMCS launch 状態 |
|------|------|------------------|
| vmlaunch | 初回のゲスト起動 | "clear" → "launched" に遷移 |
| vmresume | 2 回目以降のゲスト再開 | "launched" のまま |

vmlaunch を 2 回実行すると失敗します（launch 状態が "launched" のため）。VM exit 後にゲストを再開する場合は vmresume を使います（v30 で実装）。

### vmlaunch の失敗

vmlaunch が失敗すると、制御は vmlaunch の次の命令に進みます。

| フラグ | 意味 |
|--------|------|
| CF=1 | VMfailInvalid: 現在の VMCS が無効 |
| ZF=1 | VMfailValid: VMCS の設定に問題あり。VM-instruction error field にエラーコードが設定される |

## 実装

### guest.h --- ゲスト VM 定義

{{code:src/guest.h}}

### guest.c --- ゲスト VM 実装

{{code:src/guest.c}}

### boot.c --- カーネルエントリポイント（v29: ゲスト実行追加）

{{code:src/boot.c}}

## VM exit のフロー

```
ホスト (boot.c)           ゲスト (guest_code)
  │                         │
  ├─ vmcs_init()            │
  │                         │
  ├─ vmlaunch ─────────────→│
  │                         ├─ mov rax, 0x42
  │                         │
  │                         ├─ hlt
  │                         │  (VM exit: reason=12)
  │←────────────────────────┤
  │                         │
  ├─ vmexit_stub()          │
  │  (VM exit reason を確認)│
  │                         │
  └─ hlt ループで停止       │
```

ゲストが HLT を実行した時点で CPU が自動的にホストに制御を返します。ホストはどこにもジャンプする必要はありません。CPU が VMCS のホスト RIP を読み取って自動的にジャンプします。

## コンテナ環境での動作

VMX が利用できない環境では、シミュレーション出力が表示されます。

```
VT-x 非対応環境:
  "Guest: skipped (no VT-x)"
  "[simulated] Guest code: mov rax, 0x42; hlt"
  "[simulated] vmlaunch: guest started"
  "[simulated] VM exit: reason=HLT(12), guest RAX=0x42"
```

## PlayStation との関連

PS4 でゲームを起動するとき、ハイパーバイザはゲーム用の VMCS を設定し、vmlaunch でゲームプロセスを実行します。ゲームが I/O アクセスやシステムコールを行うと VM exit が発生し、ハイパーバイザがそれを処理してから vmresume でゲームに復帰します。v29 で実装した vmlaunch によるゲスト実行は、この仕組みの最も基本的な部分です。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3C Ch.26 "VM Entries"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- vmlaunch/vmresume の仕様、VM entry のチェック項目
- [Intel SDM Vol.3C Ch.26.2 "Checks on VMX Controls and Host-State Area"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VM entry 時のバリデーション
- [Intel SDM Vol.3C Ch.30 "VMX Instruction Reference"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- vmlaunch, vmresume の命令仕様
