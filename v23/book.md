`v23` では、x86-64 の 4 レベルページテーブルを構築してページングを有効化します。
仮想アドレスから物理アドレスへの変換の仕組みを学び、アイデンティティマップでカーネルが動作し続けることを確認します。

## 概要

`v23` は v22（物理メモリアロケータ）の上にページングを追加します。

ページングは現代 OS の最重要機能の一つです:
- **仮想アドレス空間**: 各プロセスが独立したアドレス空間を持つ
- **メモリ保護**: プロセス間のメモリ分離
- **デマンドページング**: 物理メモリの効率的な利用

この段階では最初の一歩として、アイデンティティマップ（仮想アドレス = 物理アドレス）を設定します。

## 仮想アドレス変換

### 4 レベルページテーブル

x86-64 は $48$ ビットの仮想アドレスを 4 レベルのテーブルで変換します。

```
仮想アドレス (48ビット):
  [47:39]   [38:30]   [29:21]   [20:12]   [11:0]
  PML4 idx  PDPT idx  PD idx    PT idx    オフセット
  9ビット   9ビット   9ビット   9ビット   12ビット

変換プロセス:
  CR3 → PML4[idx] → PDPT[idx] → PD[idx] → PT[idx] → 物理ページ + オフセット
```

![4レベルページテーブルウォーク](/images/v23/4level-pagetable.drawio.svg)

### 変換の具体例

仮想アドレス `0x0000000000123456` の変換:

```
ビット分解:
  [47:39] = 0x000 >> 39 & 0x1FF = 0   → PML4[0]
  [38:30] = 0x000 >> 30 & 0x1FF = 0   → PDPT[0]
  [29:21] = 0x123 >> 21 & 0x1FF = 0   → PD[0]    (0x123456 >> 21 = 0, 下位21ビットは 0x123456 & 0x1FFFFF)
  [20:12] = 0x123 >> 12 & 0x1FF = 0x123 → PT[0x123]
  [11:0]  = 0x456                      → ページ内オフセット

  物理アドレス = PT[0x123] の上位ビット + 0x456
```

### テーブルエントリの構造

各エントリは $8$ バイト（$64$ ビット）:

```
ビット  63..12              11..3    2     1      0
        物理アドレス上位52ビット  予約  User  Write  Present
```

| フラグ | ビット | 意味 |
|--------|--------|------|
| Present | 0 | エントリが有効。0 なら Page Fault 発生 |
| Write | 1 | 書き込み可能。0 なら読み取り専用 |
| User | 2 | Ring 3 からアクセス可能。0 ならカーネルのみ |

## アイデンティティマップ

アイデンティティマップ（identity mapping）とは、仮想アドレス = 物理アドレスになるマッピングです。

```
仮想アドレス → 物理アドレス
0x000000     → 0x000000
0x001000     → 0x001000
0x100000     → 0x100000  (カーネル)
0x200000     → 0x200000  (PMM 管理対象)
...
0x3FF000     → 0x3FF000
```

アイデンティティマップが必要な理由:
1. ページング有効化前後で命令ポインタ（RIP）のアドレスが変わらない
2. UART の I/O ポートアクセスが引き続き動作する
3. ページテーブル自体のアドレスもそのまま有効

## 実装

### paging.h --- インターフェース定義

{{code:src/paging.h}}

### paging.c --- 4レベルページテーブル構築

{{code:src/paging.c}}

### boot.c --- エントリポイント

全サブシステムを順に初期化し、ページング有効化後に UART が動作することを確認します。

{{code:src/boot.c}}

## ページテーブルのメモリ消費

$4\,\text{MB}$ のアイデンティティマップに必要なテーブル数:

```
PML4:  1 テーブル (4KB)  --- ルート
PDPT:  1 テーブル (4KB)  --- PML4[0] が指す
PD:    1 テーブル (4KB)  --- PDPT[0] が指す
PT:    2 テーブル (8KB)  --- PD[0], PD[1] が指す（各 2MB 分）

合計: 5 テーブル = 20KB
```

$1024$ ページ（$4\,\text{MB}$）のマッピングに $5$ ページ（$20\,\text{KB}$）のオーバーヘッド。効率は約 $99.5\%$。

## CR3 レジスタ

CR3（Control Register 3）はページテーブルのルートアドレスを保持する制御レジスタです。

```
CR3 の構造:
  ビット 63..12: PML4 テーブルの物理アドレス（4KB アラインメント）
  ビット 11..0:  フラグ（通常 0）

CR3 への書き込み:
  mov cr3, rax  ; PML4 アドレスを CR3 に設定
                ; → CPU が TLB をフラッシュ
                ; → 以降のメモリアクセスは新しいページテーブルで変換
```

## 実行結果

```
Hello, bare metal!
GDT loaded
IDT loaded
Timer started: 100Hz
PMM: initialized 512 pages (0x200000 - 0x400000)
Paging: setting up 4-level page tables
Paging: identity mapped 0x0 - 0x400000 (4MB)
Paging enabled
UART works after paging
```

"UART works after paging" が出力されれば、ページング有効化後もアイデンティティマップにより全てのアドレスが正しく変換されていることが確認できます。

## 参考文献

- [Intel SDM Vol.3 Ch.4 "Paging"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- x86-64 ページングの公式仕様
- [AMD64 APM Vol.2 Ch.5 "Page Translation"](https://www.amd.com/en/support/tech-docs/amd64-architecture-programmers-manual-volumes-1-5) --- AMD の仮想アドレス変換仕様
- [OSDev Wiki: Paging](https://wiki.osdev.org/Paging) --- ページングの実装ガイド
- [OSDev Wiki: Setting Up Paging](https://wiki.osdev.org/Setting_Up_Paging) --- ページテーブル構築の手順
