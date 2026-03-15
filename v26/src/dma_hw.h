/*
 * dma_hw.h --- DMA コントローラ（ヘッダ）
 *
 * DMA (Direct Memory Access) はデバイスが CPU を介さずに
 * メモリとデータを直接転送する仕組み。CPU はデスクリプタリングに
 * 転送命令を書き込み、DMA エンジンが非同期で実行する。
 *
 * v25 ではメモリ上にデスクリプタリングを構築し、
 * ソフトウェア DMA エンジンで転送を実行する。
 * 実際のハードウェア DMA と同じデータ構造・プロトコルを使うが、
 * 転送処理自体は CPU が行う（QEMU では汎用 DMA デバイスがないため）。
 *
 * デスクリプタリング構成:
 *   +---+     +---+     +---+     +---+
 *   | 0 | --> | 1 | --> | 2 | --> | 3 | --+
 *   +---+     +---+     +---+     +---+   |
 *     ^                                    |
 *     +------------------------------------+
 *                    (リング)
 *
 *   head: 次に投入するデスクリプタのインデックス
 *   tail: 次に完了を確認するデスクリプタのインデックス
 */

#ifndef DMA_HW_H
#define DMA_HW_H

#include <stdint.h>

/* デスクリプタリングのサイズ */
#define DMA_DESC_COUNT 4

/* デスクリプタのステータス */
#define DMA_STATUS_IDLE     0   /* 未使用 */
#define DMA_STATUS_PENDING  1   /* 転送待ち */
#define DMA_STATUS_COMPLETE 2   /* 転送完了 */
#define DMA_STATUS_ERROR    3   /* エラー */

/*
 * DMA デスクリプタ
 *
 * 1 つの転送命令を表す。src_addr から dst_addr へ length バイトをコピーする。
 * 実際のハードウェアでは、デバイスがこの構造体を読み取って転送を実行する。
 */
struct dma_desc {
    uint64_t src_addr;   /* 転送元の物理アドレス */
    uint64_t dst_addr;   /* 転送先の物理アドレス */
    uint32_t length;     /* 転送バイト数 */
    uint32_t status;     /* ステータス (idle/pending/complete/error) */
};

/*
 * DMA デスクリプタリング
 *
 * 複数のデスクリプタをリング状に配置し、
 * head/tail ポインタで投入と完了を管理する。
 */
struct dma_ring {
    struct dma_desc descs[DMA_DESC_COUNT];
    uint32_t head;   /* プロデューサ: 次に書き込むインデックス */
    uint32_t tail;   /* コンシューマ: 次に処理するインデックス */
};

/* DMA リングを初期化する */
void dma_init(struct dma_ring *ring);

/* DMA 転送を投入する。成功時 0、リングフル時 -1 */
int dma_submit(struct dma_ring *ring, uint64_t src, uint64_t dst, uint32_t len);

/* 投入済みの転送を実行する（ソフトウェア DMA エンジン） */
void dma_process(struct dma_ring *ring);

/* 完了した転送の数を返す */
int dma_poll_complete(struct dma_ring *ring);

#endif /* DMA_HW_H */
