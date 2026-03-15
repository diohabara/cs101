`v31` では、EPT (Extended Page Tables) によるゲスト間のメモリ隔離を実装し、Phase 3（ハイパーバイザ編）を完成させます。
v27-v30 で構築した VMX/VMCS/ゲスト実行/VM exit ハンドリングの上に、複数ゲスト間のセキュリティ隔離を追加します。

## 概要

EPT は Intel VT-x のメモリ仮想化機能です。通常のページテーブルがバーチャルアドレス→物理アドレスの変換を行うのに対し、EPT はゲスト物理アドレス (GPA) → ホスト物理アドレス (HPA) の変換を追加します。

EPT により、各ゲスト VM が独立した物理アドレス空間を持ちます。ゲスト A の GPA 0x1000 とゲスト B の GPA 0x1000 は異なるホスト物理メモリを指すため、ゲスト間のメモリアクセスは完全に隔離されます。

## アドレス変換の 2 段階

EPT が有効な場合、メモリアクセスは 2 段階の変換を経ます。

```
ゲスト内のアドレス変換:
  GVA (Guest Virtual Address)
    → ゲストページテーブル
    → GPA (Guest Physical Address)

EPT による変換:
  GPA (Guest Physical Address)
    → EPT テーブル（ハイパーバイザが管理）
    → HPA (Host Physical Address)

合計:
  GVA → GPA → HPA
```

EPT なしの場合、ハイパーバイザはゲストのページテーブル変更をすべてトラップして「シャドウページテーブル」を維持する必要がありました。EPT はこれをハードウェアで処理するため、大幅にオーバーヘッドが削減されます。

## EPT テーブルの構造

EPT テーブルは通常のページテーブルと同じ 4 レベル構造です。

```
EPT PML4 (Level 4)
  └─ EPT PDPT (Level 3)
       └─ EPT PD (Level 2)
            └─ EPT PT (Level 1)
                 └─ 4KB ページ (HPA)
```

GPA からインデックスを抽出する方法:

```
GPA の 48 ビット:
  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐
  │ PML4    │ PDPT    │ PD      │ PT      │ Offset      │
  │ [47:39] │ [38:30] │ [29:21] │ [20:12] │ [11:0]      │
  │ 9 bits  │ 9 bits  │ 9 bits  │ 9 bits  │ 12 bits     │
  └─────────┴─────────┴─────────┴─────────┴─────────────┘

例: GPA = 0x1000
  PML4 index = 0, PDPT index = 0, PD index = 0, PT index = 1
  → PT[1] から HPA を取得
```

### EPT エントリのビット構成

| ビット | 名前 | 意味 |
|--------|------|------|
| 0 | Read | ゲストがこのページを読み取り可能 |
| 1 | Write | ゲストがこのページに書き込み可能 |
| 2 | Execute | ゲストがこのページのコードを実行可能 |
| 3-5 | Memory Type | キャッシュタイプ (0=UC, 6=WB) |
| 12-51 | Physical Address | 次レベルテーブルまたはページの物理アドレス |

## ゲスト間のメモリ隔離

```
                  ゲスト A                          ゲスト B
                  ┌──────────┐                     ┌──────────┐
                  │ GPA 0x0  │                     │ GPA 0x0  │
                  │ GPA 0x1000│                    │ GPA 0x1000│
                  └────┬─────┘                     └────┬─────┘
                       │                                 │
                  EPT A│                            EPT B│
                       ▼                                 ▼
          ┌────────────────────────────────────────────────────┐
          │ Host Physical Memory                               │
          │                                                    │
          │  HPA 0x300000  ← Guest A, GPA 0x0                 │
          │  HPA 0x301000  ← Guest A, GPA 0x1000              │
          │  ...                                               │
          │  HPA 0x400000  ← Guest B, GPA 0x0                 │
          │  HPA 0x401000  ← Guest B, GPA 0x1000              │
          └────────────────────────────────────────────────────┘
```

ゲスト A が GPA 0x1000 にアクセスすると HPA 0x301000 を参照します。ゲスト B が同じ GPA 0x1000 にアクセスすると HPA 0x401000 を参照します。ゲスト A はゲスト B のメモリ（HPA 0x400000 付近）にアクセスする方法がありません。

## EPT violation

ゲストがマッピングされていない GPA にアクセスすると、EPT violation（VM exit reason 48）が発生します。

```
ゲスト A の EPT マッピング:
  GPA 0x0000 → HPA 0x300000  (mapped)
  GPA 0x1000 → HPA 0x301000  (mapped)
  GPA 0x2000 → ???           (unmapped)

ゲスト A が GPA 0x2000 にアクセス:
  → EPT violation (VM exit reason 48)
  → ハイパーバイザの選択肢:
     a) 新しいページを割り当ててマッピングする（デマンドページング）
     b) ゲストに #PF を注入する（不正アクセス）
     c) ゲストを終了する（セキュリティ違反）
```

## 実装

### ept.h --- EPT 定義

{{code:src/ept.h}}

### ept.c --- EPT 実装

{{code:src/ept.c}}

### hypervisor.c --- ハイパーバイザ統合

{{code:src/hypervisor.c}}

### boot.c --- カーネルエントリポイント（v31: ハイパーバイザ統合）

{{code:src/boot.c}}

## Phase 3 の全体像

v27-v31 で構築したハイパーバイザの全体構造です。

```
ハイパーバイザ (Phase 3)
  │
  ├─ v27: VMX 検出・有効化
  │    ├─ CPUID で VT-x 検出
  │    ├─ IA32_FEATURE_CONTROL MSR 確認
  │    ├─ CR4.VMXE 設定
  │    └─ VMXON / VMXOFF
  │
  ├─ v28: VMCS 設定
  │    ├─ vmclear → vmptrld
  │    ├─ ゲスト状態 (RIP, RSP, CR0, segments)
  │    ├─ ホスト状態 (exit handler RIP, RSP)
  │    └─ VM 実行制御 (HLT/IO exit)
  │
  ├─ v29: ゲスト実行
  │    ├─ ゲストコード配置 (mov rax, 0x42; hlt)
  │    └─ vmlaunch でゲスト起動
  │
  ├─ v30: VM exit ハンドリング
  │    ├─ exit reason 読み取り
  │    ├─ CPUID exit → フィルタ結果返却
  │    ├─ HLT exit → 停止判断
  │    ├─ I/O exit → 仮想デバイスエミュレーション
  │    └─ vmresume でゲスト再開
  │
  └─ v31: EPT メモリ隔離
       ├─ 4 レベル EPT テーブル構築
       ├─ GPA → HPA マッピング
       ├─ ゲスト間メモリ隔離
       └─ EPT violation ハンドリング
```

## PlayStation との関連

PS4 のハイパーバイザは EPT（AMD では NPT: Nested Page Tables）を使って、ゲーム環境のメモリを完全に隔離しています。各ゲームプロセスは独立した物理アドレス空間を持ち、他のゲームやシステムのメモリにアクセスできません。これにより、チート対策とシステムの安定性を同時に実現しています。v31 で実装した EPT テーブルの構築とゲスト間隔離は、PS4 のセキュリティアーキテクチャの核心的な仕組みです。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3C Ch.28 "VMX Support for Address Translation"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- EPT の仕様、GPA→HPA 変換
- [Intel SDM Vol.3C Ch.28.2 "EPT Translation Mechanism"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- EPT テーブルの構造、エントリフォーマット
- [Intel SDM Vol.3C Ch.28.2.3 "EPT Violations"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- EPT violation の発生条件と処理
- [Intel SDM Vol.3D Appendix C Table C-1](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VM exit reason 48 (EPT violation)
- [OSDev Wiki: Extended Page Tables](https://wiki.osdev.org/Extended_Page_Tables) --- EPT の実装例
