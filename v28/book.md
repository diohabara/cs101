`v28` では、VMCS (Virtual Machine Control Structure) を設定してゲストとホストの CPU 状態を定義します。
v27 で VMX モードに入った後、ゲスト VM を実行するための制御構造を構築します。

## 概要

VMCS は VMX のコアデータ構造です。4KB のメモリ領域に、ゲストの CPU 状態、ホストの CPU 状態、VM 実行制御パラメータがすべて格納されます。vmlaunch 命令でゲストを起動する前に、VMCS のフィールドを正しく設定する必要があります。

## VMCS の構造

VMCS のフィールドは 4 つのカテゴリに分類されます。

| カテゴリ | 内容 | 例 |
|----------|------|-----|
| ゲスト状態 | ゲスト VM の CPU 状態 | RIP, RSP, CR0, CR3, CR4, セグメントレジスタ |
| ホスト状態 | VM exit 時に復元する CPU 状態 | RIP (exit ハンドラ), RSP, CR0, CR3, CR4 |
| VM 実行制御 | どの操作で VM exit を発生させるか | HLT exit, I/O exit, CPUID exit |
| VM exit 情報 | exit の理由と詳細 | exit reason, qualification, 命令長 |

```
VMCS 領域 (4096 バイト):
  +0x000: VMCS リビジョン ID (4バイト)
  +0x004: VM abort indicator (4バイト)
  +0x008: VMCS データ (4088 バイト)
           ├── ゲスト状態フィールド
           ├── ホスト状態フィールド
           ├── VM 実行制御フィールド
           └── VM exit 情報フィールド
```

フィールドは直接メモリアクセスではなく、vmread/vmwrite 命令でアクセスします。各フィールドはエンコーディング番号で識別されます。

## VMCS の初期化手順

| ステップ | 命令 | 意味 |
|----------|------|------|
| 1 | vmclear | VMCS をクリアして launch 状態を "clear" にする |
| 2 | vmptrld | この VMCS を「現在の VMCS」として CPU に登録 |
| 3 | vmwrite (ゲスト状態) | ゲストの RIP, RSP, CR0 等を設定 |
| 4 | vmwrite (ホスト状態) | VM exit 時の復帰先を設定 |
| 5 | vmwrite (制御) | VM exit の発生条件を設定 |

### vmclear と vmptrld

```
vmclear: VMCS 領域をクリア
  ├── VMCS 内部データを初期化
  └── launch 状態を "clear" にする（vmlaunch の前提条件）

vmptrld: VMCS をアクティブにする
  ├── CPU の内部ポインタを設定
  └── 以後の vmread/vmwrite はこの VMCS に対して操作
```

### ゲスト状態

ゲストが実行を開始する時点の CPU 状態を設定します。

| フィールド | エンコーディング | 設定値 | 意味 |
|-----------|----------------|--------|------|
| Guest RIP | 0x681E | ゲストコードのアドレス | ゲストの実行開始位置 |
| Guest RSP | 0x681C | ゲストスタックの先頭 | ゲストのスタックポインタ |
| Guest CR0 | 0x6800 | ホストと同じ | プロテクトモード等 |
| Guest CR4 | 0x6804 | ホストと同じ | VMX 有効ビット含む |
| Guest RFLAGS | 0x6820 | 0x02 | ビット 1 は常に 1（予約） |
| Guest CS | 0x0802 | 0x08 | カーネルコードセレクタ |

### ホスト状態

VM exit が発生したときに CPU が復元する状態です。

| フィールド | エンコーディング | 設定値 | 意味 |
|-----------|----------------|--------|------|
| Host RIP | 0x6C16 | exit ハンドラのアドレス | VM exit 時のジャンプ先 |
| Host RSP | 0x6C14 | ホストスタック | ホストのスタックポインタ |
| Host CR0 | 0x6C00 | 現在の CR0 | ホストの制御レジスタ |
| Host CS | 0x0C02 | 0x08 | カーネルコードセレクタ |

### VM 実行制御

どの操作で VM exit を発生させるかを制御するビットフィールドです。

| 制御 | MSR | 主要ビット |
|------|-----|-----------|
| Pin-based | IA32_VMX_PINBASED_CTLS (0x481) | 外部割り込み exit |
| Proc-based | IA32_VMX_PROCBASED_CTLS (0x482) | HLT exit (bit 7), I/O exit |
| Exit controls | IA32_VMX_EXIT_CTLS (0x483) | 64bit ホスト (bit 9) |
| Entry controls | IA32_VMX_ENTRY_CTLS (0x484) | 64bit ゲスト (bit 9) |

制御ビットには「must be 1」と「must be 0」の制約があり、MSR から許可範囲を取得して設定します。

## 実装

### vmcs.h --- VMCS 定義

{{code:src/vmcs.h}}

### vmcs.c --- VMCS 初期化

{{code:src/vmcs.c}}

### boot.c --- カーネルエントリポイント（v28: VMCS 追加）

{{code:src/boot.c}}

![VMCS 構造](/images/v28/vmcs-structure.drawio.svg)

## VMCS のライフサイクル

```
vmclear ──→ "clear" 状態
  │
  └─ vmptrld ──→ "current" 状態
       │
       ├─ vmwrite ──→ フィールド設定
       │
       ├─ vmlaunch ──→ "launched" 状態（v29 で実装）
       │    │
       │    └─ VM exit ──→ ホスト実行
       │         │
       │         └─ vmresume ──→ ゲスト再開（v30 で実装）
       │
       └─ vmclear ──→ "clear" に戻す
```

v28 では vmclear → vmptrld → vmwrite の部分を実装しました。v29 で vmlaunch によるゲスト実行に進みます。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3C Ch.24 "Virtual Machine Control Structures"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VMCS の構造、フィールドエンコーディング
- [Intel SDM Vol.3C Ch.24.4 "Guest-State Area"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- ゲスト状態フィールドの仕様
- [Intel SDM Vol.3C Ch.24.5 "Host-State Area"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- ホスト状態フィールドの仕様
- [Intel SDM Vol.3C Ch.24.6 "VM-Execution Control Fields"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VM 実行制御の設定
- [Intel SDM Vol.3D Appendix B "Field Encoding in VMCS"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VMCS フィールドのエンコーディング番号一覧
