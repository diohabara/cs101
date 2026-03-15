`v16` では、DMA (Direct Memory Access) の基本概念を学びます。
リングバッファ（循環バッファ）の仕組みと、CPU の代わりにデバイスがメモリにデータを書き込む DMA 転送をシミュレーションします。

## 概要

コンピュータがディスクやネットワークからデータを読み込むとき、CPU が 1 バイトずつコピーしていたら他の仕事ができません。
DMA は「CPU の代わりにデバイス（DMA エンジン）がメモリを直接読み書きする」仕組みです。

CPU の役割は 2 つだけに限定されます。

1. **開始**: 「ここからここへ N バイトコピーしろ」という指示（ディスクリプタ）をデバイスに渡す
2. **完了確認**: デバイスが転送を終えたかどうかを確認する

この指示のやりとりに使われるのがリングバッファ（循環バッファ）です。

## リングバッファ

リングバッファは、固定長の配列を head/tail ポインタで管理する FIFO キューです。
配列の末尾に達したら先頭に戻る（wrap-around）ことで、メモリの再割り当てなしに繰り返し使えます。

```
配列:  [ slot0 | slot1 | slot2 | slot3 ]

初期状態（空）:
  head=0, tail=0, count=0
  [ ___ | ___ | ___ | ___ ]

enqueue 1,2,3,4（満杯）:
  head=0, tail=4, count=4
  [  1  |  2  |  3  |  4  ]
  ↑head                ↑tail(4%4=0 と同じ位置)

dequeue 1,2:
  head=2, tail=4, count=2
  [ ___ | ___ |  3  |  4  ]
            ↑head

enqueue 5,6（wrap-around）:
  head=2, tail=6, count=4
  [  5  |  6  |  3  |  4  ]
  tail(6%4=2)→↑  ↑head
  ※ tail が配列の先頭に戻って slot0, slot1 に書き込む
```

### なぜ配列サイズの剰余で循環させるのか

`tail % RING_SIZE` とすると、tail が配列サイズを超えても正しいインデックスに変換されます。

```
tail = 4 → 4 % 4 = 0  （slot0 に書き込む）
tail = 5 → 5 % 4 = 1  （slot1 に書き込む）
tail = 6 → 6 % 4 = 2  （slot2 に書き込む ← ここで head に追いつく = 満杯）
```

head も同様に剰余で循環します。これにより、ポインタをリセットする必要がなく、単調増加させるだけで正しく動作します。

![リングバッファ: head/tail ポインタで循環する固定長キュー](/images/v16/ring-buffer.drawio.svg)

### ringbuf --- リングバッファの enqueue / dequeue

| ステップ | 操作 | head | tail | count | 配列の中身 | 説明 |
|----------|------|------|------|-------|------------|------|
| 1 | enqueue 1 | 0 | 1 | 1 | [1,_,_,_] | slot0 に書き込み |
| 2 | enqueue 2 | 0 | 2 | 2 | [1,2,_,_] | slot1 に書き込み |
| 3 | enqueue 3 | 0 | 3 | 3 | [1,2,3,_] | slot2 に書き込み |
| 4 | enqueue 4 | 0 | 4 | 4 | [1,2,3,4] | slot3 に書き込み、満杯 |
| 5 | enqueue 5 | 0 | 4 | 4 | [1,2,3,4] | count=4=RING_SIZE で失敗 |
| 6 | dequeue → 1 | 1 | 4 | 3 | [_,2,3,4] | head=0 から読み出し |
| 7 | dequeue → 2 | 2 | 4 | 2 | [_,_,3,4] | head=1 から読み出し |
| 8 | enqueue 5 | 2 | 5 | 3 | [5,_,3,4] | tail=4%4=0 → slot0 に wrap-around |
| 9 | enqueue 6 | 2 | 6 | 4 | [5,6,3,4] | tail=5%4=1 → slot1 に書き込み |
| 10 | dequeue → 3 | 3 | 6 | 3 | [5,6,_,4] | head=2 から読み出し |
| 11 | dequeue → 4 | 4 | 6 | 2 | [5,6,_,_] | head=3 から読み出し |
| 12 | dequeue → 5 | 5 | 6 | 1 | [_,6,_,_] | head=4%4=0 → head も wrap-around |
| 13 | dequeue → 6 | 6 | 6 | 0 | [_,_,_,_] | head=5%4=1 から読み出し、空になる |

{{code:src/ringbuf.c}}

## DMA の仕組み

### CPU コピー vs DMA

CPU が自分でデータをコピーする場合、転送中は CPU がビジーになります。

```
CPU コピー（PIO: Programmed I/O）:
  CPU: load byte from device register
  CPU: store byte to memory
  CPU: load byte from device register
  CPU: store byte to memory
  ...（数千回繰り返す）
  → CPU は転送中ずっと占有される

DMA:
  CPU: ディスクリプタを書いて転送開始を指示
  CPU: 別の仕事をする（ゲームの物理演算、AI処理など）
  DMA エンジン: メモリにデータを直接書き込む
  DMA エンジン: 完了を通知
  CPU: 完了を確認してデータを使う
```

### DMA ディスクリプタ

CPU がデバイスに転送を指示するためのデータ構造です。

```c
typedef struct {
    const void *src;    /* 転送元アドレス */
    void       *dst;    /* 転送先アドレス */
    size_t      length; /* 転送バイト数 */
    DmaStatus   status; /* IDLE → PENDING → COMPLETE */
} DmaDescriptor;
```

状態遷移:

```
IDLE ──(CPU がディスクリプタを書き込む)──→ PENDING
PENDING ──(DMA エンジンが転送を実行)──→ COMPLETE
COMPLETE ──(CPU がポーリングで検知)──→ データ使用可能
```

### dma_sim --- DMA 転送シミュレーション

| ステップ | 実行者 | 操作 | 状態 |
|----------|--------|------|------|
| 1 | CPU | 転送元バッファに "Hello, DMA!" を用意 | --- |
| 2 | CPU | ディスクリプタに src, dst, length を書き込む | status = PENDING |
| 3 | DMA エンジン | ディスクリプタを読み、memcpy でデータをコピー | 転送中 |
| 4 | DMA エンジン | status を COMPLETE に更新 | status = COMPLETE |
| 5 | CPU | status をポーリングで確認 | 完了検知 |
| 6 | CPU | dst のデータが src と一致するか検証 | "data verified" |

{{code:src/dma_sim.c}}

このシミュレーションでは memcpy を使っていますが、実際のハードウェアではデバイスが PCIe バス経由でメモリを直接読み書きします。CPU はディスクリプタの設定と完了確認しか行わない点が重要です。

## ディスクリプタリングと NVMe

NVMe SSD は、リングバッファとディスクリプタを組み合わせた仕組みでコマンドを処理します。

```
NVMe のコマンド処理:

  Submission Queue（リングバッファ）:
    CPU がコマンド（= ディスクリプタ）を enqueue
    ↓
  Doorbell Register:
    CPU が tail ポインタを書き込んで「新しいコマンドがあるぞ」と通知
    ↓
  NVMe コントローラ:
    コマンドを dequeue して DMA 転送を実行
    ↓
  Completion Queue（リングバッファ）:
    デバイスが完了エントリを enqueue
    ↓
  CPU:
    完了エントリを dequeue して結果を確認
```

| v16 の概念 | NVMe での実体 |
|------------|---------------|
| RingBuf | Submission Queue / Completion Queue |
| ring_enqueue | SQE を Queue に追加 |
| ring_dequeue | CQE を Queue から取り出し |
| DmaDescriptor | SQE（64 バイトのコマンド構造体） |
| dma_engine_process | NVMe コントローラの DMA エンジン |
| status ポーリング | CQ の Phase Tag 確認 |

## ヘッダファイル

{{code:src/dma.h}}

## PlayStation との関連

- **DMA**: PS4/PS5 の GPU は DMA でメインメモリからテクスチャやバーテックスバッファを VRAM に転送します。CPU がピクセルデータを 1 バイトずつコピーしていたらフレームレートが維持できません
- **リングバッファ**: GPU のコマンドバッファはリングバッファ構造です。CPU がグラフィックスコマンドを enqueue し、GPU の Command Processor が dequeue して描画を実行します。v16 のリングバッファと同じ head/tail の仕組みです
- **NVMe**: PS5 の高速ロードを支える NVMe SSD は、まさにこの章で学んだディスクリプタリングで I/O コマンドを処理しています。ゲームのアセットをメモリに直接転送し、CPU のオーバーヘッドを最小限に抑えます

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [NVM Express Base Specification 2.1](https://nvmexpress.org/specifications/) --- Submission Queue / Completion Queue の構造、Doorbell レジスタ、コマンド処理フローの定義
- [PCI Express Base Specification 6.0](https://pcisig.com/specifications) --- DMA の物理的な転送メカニズム（Bus Master DMA）の仕様
- [Intel 82599 10 GbE Controller Datasheet](https://www.intel.com/content/www/us/en/ethernet-controllers/82599-10-gbe-controller-datasheet.html) --- ネットワークカードにおけるディスクリプタリングの実装例
