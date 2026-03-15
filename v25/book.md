`v25` では、DMA（Direct Memory Access）コントローラのデスクリプタリングを実装し、メモリ間のデータ転送を行います。
v24 までに構築したカーネル基盤の上で、CPU を介さないデータ転送の仕組みを学びます。

## 概要

DMA は CPU がメモリ転送の指示だけを出し、実際のデータコピーは DMA コントローラが自律的に行う仕組みです。CPU は転送中に他の処理（計算、割り込み処理など）を実行できるため、I/O 性能が大幅に向上します。

v25 では DMA の核心であるデスクリプタリングを実装します。デスクリプタリングは NIC (e1000)、ストレージ (AHCI, NVMe)、GPU など、現代のほぼすべての高速デバイスが採用するデータ構造です。

## DMA デスクリプタリング

### デスクリプタとは

デスクリプタは 1 つの転送命令を表す構造体です。

```
struct dma_desc {
    uint64_t src_addr;   転送元の物理アドレス
    uint64_t dst_addr;   転送先の物理アドレス
    uint32_t length;     転送バイト数
    uint32_t status;     ステータス (idle/pending/complete/error)
};
```

CPU はデスクリプタに転送パラメータを書き込み、DMA エンジンがそれを読み取って実行します。

### リング構造

複数のデスクリプタをリング（円形バッファ）状に配置し、head/tail ポインタで管理します。

```
デスクリプタリング（4 エントリ）:

  head = 2 (次に投入する位置)
  tail = 0 (次に処理する位置)

  Index:  [0]         [1]         [2]         [3]
  Status: PENDING     PENDING     IDLE        IDLE
          ^tail                   ^head

  投入: descs[head] にパラメータ設定 → head = (head+1) % 4
  処理: descs[tail] を実行 → status = COMPLETE → tail = (tail+1) % 4
  フル判定: (head+1) % 4 == tail
```

head は CPU（プロデューサ）が進め、tail は DMA エンジン（コンシューマ）が進めます。この分離により、CPU と DMA エンジンがロックなしで並行動作できます。

### ステータス遷移

```
IDLE → PENDING → COMPLETE
  ^                  |
  +---(再利用)-------+

IDLE:     デスクリプタが未使用
PENDING:  CPU が転送パラメータを設定済み、DMA エンジンの処理待ち
COMPLETE: DMA エンジンが転送を完了
ERROR:    転送エラー（アドレス不正など）
```

## 転送フロー

```
CPU 側:
  1. dma_submit(ring, src, dst, len)
     → descs[head] にパラメータ設定
     → status = PENDING
     → head を進める

  2. DMA エンジンに処理開始を通知
     （実ハードウェアでは doorbell レジスタに書き込み）

  3. dma_poll_complete(ring)
     → COMPLETE 状態のデスクリプタ数を返す

DMA エンジン側:
  1. tail のデスクリプタを読み取り
  2. src_addr → dst_addr にデータ転送
  3. status = COMPLETE
  4. tail を進める
  5. 割り込みで CPU に完了を通知（オプション）
```

## v25 での実装方針

実際の DMA はハードウェアが転送を実行しますが、QEMU には汎用 DMA デバイスがないため、ソフトウェアで DMA エンジンを模倣します。デスクリプタリングのデータ構造とプロトコルは本物と同じなので、概念の学習には十分です。

v25 のテストフロー:

| ステップ | 操作 | 意味 |
|----------|------|------|
| 1 | `pmm_alloc()` x 2 | ソース/デスティネーションページを確保 |
| 2 | ソースにパターン書き込み | 0x00, 0x01, ..., 0xFF の 256 バイト |
| 3 | `dma_submit()` | デスクリプタリングに転送命令を投入 |
| 4 | `dma_process()` | ソフトウェア DMA エンジンで転送実行 |
| 5 | デスティネーション検証 | パターンが正しくコピーされたか確認 |
| 6 | `pmm_free()` x 2 | ページを解放 |

## 実装

### dma_hw.h --- DMA デスクリプタリング定義

{{code:src/dma_hw.h}}

### dma_hw.c --- DMA エンジン実装

{{code:src/dma_hw.c}}

### boot.c --- カーネルエントリポイント（v25）

{{code:src/boot.c}}

### uart.h / uart.c --- UART ドライバ（v18 から継続）

{{code:src/uart.h}}

{{code:src/uart.c}}

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

### usermode.h / usermode.c / syscall_handler.c --- syscall 機構（v24 から継続）

{{code:src/usermode.h}}

{{code:src/usermode.c}}

{{code:src/syscall_handler.c}}

### linker.ld --- リンカスクリプト

{{code:src/linker.ld}}

## 実デバイスでの DMA

### NIC (e1000)

Intel e1000 NIC はパケット送受信に TX/RX デスクリプタリングを使います。

```
e1000 TX デスクリプタ:
  buffer_addr:  送信データのアドレス
  length:       データ長
  cmd:          コマンドビット（EOP=End of Packet, RS=Report Status）
  status:       完了ステータス（DD=Descriptor Done）
```

OS はパケットデータをバッファに書き、TX デスクリプタリングに投入し、TDT (Tail Descriptor Tail) レジスタを書き換えます。NIC が自律的にパケットを送信し、DD ビットで完了を通知します。

### NVMe

NVMe SSD もコマンドキュー（Submission Queue / Completion Queue）と呼ばれるリング構造を使います。v25 のデスクリプタリングと本質的に同じ仕組みです。

## PlayStation との関連

PS4/PS5 では GPU へのコマンド送信に DMA リングバッファを使います。ゲームが描画コマンド（ドローコール、テクスチャ転送）をリングバッファに投入し、GPU がコマンドを読み取って処理します。v25 で学んだデスクリプタリングは GPU コマンドバッファの基盤技術です。

## 参考文献

- [Intel 82574 GbE Controller Datasheet Ch.3 "Transmit/Receive Descriptor Ring"](https://www.intel.com/content/www/us/en/products/details/ethernet.html) --- NIC DMA デスクリプタリングの実例
- [NVMe Specification 2.0 Ch.4 "Submission and Completion Queues"](https://nvmexpress.org/specifications/) --- NVMe のリングバッファ仕様
- [OSDev Wiki: DMA](https://wiki.osdev.org/DMA) --- ISA DMA の概要
- [OSDev Wiki: PCI](https://wiki.osdev.org/PCI) --- バスマスター DMA の概要
