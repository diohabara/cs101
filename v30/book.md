`v30` では、VM exit reason に応じたハンドリングを実装します。
ゲストが HLT, CPUID, I/O 命令を実行した時に発生する VM exit を、ハイパーバイザが適切に処理します。

## 概要

VM exit はゲストからホスト（ハイパーバイザ）への制御遷移です。v29 では VM exit が発生するとそのまま停止していましたが、v30 では exit reason を読み取り、reason ごとに異なる処理を行います。

実際のハイパーバイザ（KVM, Hyper-V, VMware 等）は数十種類の exit reason をハンドリングしています。v30 では最も基本的な 3 種類を実装します。

## VM exit reason

VM exit が発生すると、VMCS の exit reason フィールド (0x4402) に理由が書き込まれます。

| reason | 名前 | 意味 | ハンドリング |
|--------|------|------|-------------|
| 10 | CPUID | ゲストが CPUID 命令を実行 | フィルタした CPU 情報を返す |
| 12 | HLT | ゲストが HLT 命令を実行 | ゲスト停止 or 割り込み注入 |
| 30 | I/O | ゲストが IN/OUT 命令を実行 | 仮想デバイスをエミュレート |
| 48 | EPT violation | 無効な物理アドレスアクセス | v31 で実装 |

### exit qualification

exit reason に加えて、exit qualification フィールド (0x6400) が詳細情報を提供します。

```
I/O exit の exit qualification:
  bit 3:     方向（0=OUT, 1=IN）
  bit 15:0:  ポート番号（bit 31:16 に格納）
  bit 2:0:   アクセスサイズ（0=1バイト, 1=2バイト, 3=4バイト）

例: OUT 0x3F8, AL の場合
  qualification = 0x03F80000
  ポート = 0x3F8 (COM1), 方向 = OUT, サイズ = 1 バイト
```

## CPUID exit のハンドリング

ゲストが CPUID を実行すると VM exit が発生します。ハイパーバイザは「見せたい」CPU 情報をゲストに返します。

```
ゲスト:
  mov eax, 1     ; CPUID leaf=1
  cpuid           ; → VM exit!

ハイパーバイザ:
  1. exit reason = 10 (CPUID) を確認
  2. ゲストの EAX (leaf 番号) を読み取る
  3. フィルタした結果を EAX/EBX/ECX/EDX に設定
     例: ECX の VMX ビット (bit 5) を 0 にする → ゲストは VMX 非対応と認識
  4. ゲスト RIP を CPUID の次の命令に進める
  5. vmresume でゲストに復帰
```

これにより、ゲストに対して CPU 機能をフィルタリングできます。入れ子仮想化（nested virtualization）の制御にも使われます。

## HLT exit のハンドリング

```
ゲスト:
  hlt             ; → VM exit!

ハイパーバイザの選択肢:
  a) ゲストを停止する（v30 の実装）
  b) 割り込みをゲストに注入して再開させる
  c) 他のゲスト（vCPU）にスケジューリングする
```

HLT exit は vCPU スケジューリングのトリガーとしてよく使われます。ゲストが HLT で待機状態になったら、ハイパーバイザは別の vCPU を実行します。

## I/O exit のハンドリング

```
ゲスト:
  mov al, 'A'
  out 0x3F8, al   ; UART に 'A' を出力 → VM exit!

ハイパーバイザ:
  1. exit reason = 30 (I/O) を確認
  2. exit qualification からポート番号 (0x3F8) と方向 (OUT) を取得
  3. 仮想 UART にデータを転送
  4. ゲスト RIP を OUT の次の命令に進める
  5. vmresume でゲストに復帰
```

I/O exit により、ゲストのデバイスアクセスをハイパーバイザが完全に制御できます。仮想ネットワークカード、仮想ディスクなどはすべてこの仕組みで実現されています。

## vmresume

VM exit 後にゲストを再開するには vmresume 命令を使います。

```
vmlaunch (初回実行)
  └─ VM exit
       └─ ハンドリング
            └─ vmresume (再開)
                 └─ VM exit
                      └─ ハンドリング
                           └─ vmresume (再開)
                                └─ ...
```

vmresume は VMCS の launch 状態が "launched" のときに使います。vmlaunch は "clear" → "launched" への遷移で、vmresume は "launched" のまま再実行します。

## 実装

### vmexit.h --- VM exit ハンドリング定義

{{code:src/vmexit.h}}

### vmexit_handler.c --- VM exit ハンドリング実装

{{code:src/vmexit_handler.c}}

### boot.c --- カーネルエントリポイント（v30: VM exit ハンドリング追加）

{{code:src/boot.c}}

## VM exit ハンドリングの全体フロー

```
vmlaunch / vmresume
  ├─ ゲスト実行
  │    ├─ 通常命令: CPU が直接実行（VM exit なし）
  │    └─ 特権命令/制御対象: VM exit 発生
  │
  └─ VM exit
       ├─ CPU が VMCS に exit reason, qualification を書き込む
       ├─ CPU がホスト状態を VMCS から復元
       ├─ ホスト RIP (vmexit ハンドラ) にジャンプ
       │
       └─ vmexit_handler()
            ├─ VMCS から exit reason を読み取る (vmread)
            │
            ├─ reason=10 (CPUID): フィルタ結果を設定、RIP 更新、vmresume
            ├─ reason=12 (HLT):   停止 or 割り込み注入
            ├─ reason=30 (I/O):   仮想デバイスエミュレーション、vmresume
            └─ 未知の reason:      停止
```

## PlayStation との関連

PS4 のハイパーバイザは、ゲームプロセスが生成する VM exit を高速にハンドリングしています。特に I/O exit（GPU, ネットワーク等）は頻繁に発生するため、ハンドリングのオーバーヘッドがゲーム性能に直結します。exit reason ごとに最適化されたハンドラを用意し、vmresume までの時間を最小化しています。v30 で実装した reason ベースのディスパッチは、その基本パターンです。

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Intel SDM Vol.3C Ch.27 "VM Exits"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VM exit の発生条件、exit reason の一覧
- [Intel SDM Vol.3C Ch.27.1 "Architectural State Before a VM Exit"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- VM exit 時の CPU 状態保存
- [Intel SDM Vol.3D Appendix C "VMX Basic Exit Reasons"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- exit reason 番号の完全一覧
- [Intel SDM Vol.3C Ch.27.2 "Recording VM-Exit Information and Updating VM-Entry Control Fields"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- exit qualification の構造
