/*
 * dma_hw.c --- DMA コントローラ（実装）
 *
 * メモリ上のデスクリプタリングを使って DMA 転送を管理する。
 *
 * 転送フロー:
 *   1. CPU が dma_submit() でデスクリプタを投入（status = PENDING）
 *   2. DMA エンジン（dma_process）が PENDING デスクリプタを処理
 *   3. メモリコピー完了後、status = COMPLETE に設定
 *   4. CPU が dma_poll_complete() で完了を確認
 *
 * 実際のハードウェアでは手順 2 を DMA コントローラが自律的に実行するが、
 * v25 ではソフトウェアで模倣する。デスクリプタリングのデータ構造と
 * プロトコルは本物の DMA コントローラ（e1000, AHCI, NVMe など）と同じ。
 */

#include "dma_hw.h"
#include "uart.h"

void dma_init(struct dma_ring *ring) {
    ring->head = 0;
    ring->tail = 0;
    for (int i = 0; i < DMA_DESC_COUNT; i++) {
        ring->descs[i].src_addr = 0;
        ring->descs[i].dst_addr = 0;
        ring->descs[i].length   = 0;
        ring->descs[i].status   = DMA_STATUS_IDLE;
    }
}

int dma_submit(struct dma_ring *ring, uint64_t src, uint64_t dst, uint32_t len) {
    /*
     * リングフル判定:
     * head が tail の 1 つ手前に来たらフル。
     * (head + 1) % COUNT == tail ならリングフル。
     */
    uint32_t next = (ring->head + 1) % DMA_DESC_COUNT;
    if (next == ring->tail) {
        uart_puts("DMA ring full\n");
        return -1;
    }

    /* デスクリプタにパラメータを設定 */
    struct dma_desc *desc = &ring->descs[ring->head];
    desc->src_addr = src;
    desc->dst_addr = dst;
    desc->length   = len;
    desc->status   = DMA_STATUS_PENDING;

    /* head を進める */
    ring->head = next;

    return 0;
}

void dma_process(struct dma_ring *ring) {
    /*
     * tail から head まで、PENDING 状態のデスクリプタを順に処理する。
     * 実際の DMA コントローラではバスマスターがメモリ転送を行うが、
     * ここでは CPU による memcpy で代替する。
     */
    while (ring->tail != ring->head) {
        struct dma_desc *desc = &ring->descs[ring->tail];

        if (desc->status != DMA_STATUS_PENDING) {
            break;
        }

        /* メモリコピー（ソフトウェア DMA）*/
        uint8_t *src = (uint8_t *)(uintptr_t)desc->src_addr;
        uint8_t *dst = (uint8_t *)(uintptr_t)desc->dst_addr;
        for (uint32_t i = 0; i < desc->length; i++) {
            dst[i] = src[i];
        }

        /* ステータスを完了に設定 */
        desc->status = DMA_STATUS_COMPLETE;

        /* tail を進める */
        ring->tail = (ring->tail + 1) % DMA_DESC_COUNT;
    }
}

int dma_poll_complete(struct dma_ring *ring) {
    int count = 0;
    for (int i = 0; i < DMA_DESC_COUNT; i++) {
        if (ring->descs[i].status == DMA_STATUS_COMPLETE) {
            count++;
        }
    }
    return count;
}
