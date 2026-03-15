/*
 * dma.h --- リングバッファとDMAディスクリプタの定義
 *
 * リングバッファ: head/tail ポインタで管理する固定長の循環バッファ。
 * NVMe の Submission Queue / Completion Queue と同じ構造。
 *
 * DMA ディスクリプタ: 転送元・転送先アドレスと長さ、状態を持つ。
 * CPU がディスクリプタを書き込み、DMA エンジン（デバイス）が
 * データ転送を実行する。CPU はポーリングで完了を検知する。
 */

#ifndef DMA_H
#define DMA_H

#include <stddef.h>
#include <stdint.h>

/* ---- リングバッファ ---- */

/*
 * RING_SIZE はバッファのスロット数。
 * 小さくしておくと wrap-around の動作を確認しやすい。
 *
 *   head: 次に dequeue する位置
 *   tail: 次に enqueue する位置
 *   count: 現在の要素数
 *
 *   配列インデックスは (head % RING_SIZE) のように剰余で循環する。
 *
 *   例（RING_SIZE = 4）:
 *     enqueue 1,2,3,4 → [1,2,3,4], head=0, tail=4, count=4
 *     dequeue 2個      → [_,_,3,4], head=2, tail=4, count=2
 *     enqueue 5,6      → [5,6,3,4], tail=6 (6%4=2), count=4
 *                         ↑ wrap-around: tail が配列の先頭に戻る
 */
#define RING_SIZE 4

typedef struct {
    int buf[RING_SIZE];
    unsigned int head;  /* dequeue 位置（消費側） */
    unsigned int tail;  /* enqueue 位置（生産側） */
    unsigned int count; /* 現在の要素数 */
} RingBuf;

/* ---- DMA ディスクリプタ ---- */

/*
 * DMA 転送の状態:
 *   IDLE    — ディスクリプタ未使用
 *   PENDING — CPU がディスクリプタを書き込み済み、転送待ち
 *   COMPLETE — DMA エンジンが転送を完了
 */
typedef enum {
    DMA_IDLE     = 0,
    DMA_PENDING  = 1,
    DMA_COMPLETE = 2,
} DmaStatus;

/*
 * DMA ディスクリプタ:
 *   src    — 転送元アドレス（物理アドレスに相当）
 *   dst    — 転送先アドレス
 *   length — 転送バイト数
 *   status — 転送状態
 *
 * 実際の DMA コントローラでは、このディスクリプタがリングバッファ上に
 * 並び、デバイスが順番に処理する。NVMe では 64 バイトの SQE
 * (Submission Queue Entry) がこれに相当する。
 */
typedef struct {
    const void *src;
    void       *dst;
    size_t      length;
    DmaStatus   status;
} DmaDescriptor;

#endif /* DMA_H */
