/*
 * ringbuf.c --- リングバッファの enqueue / dequeue
 *
 * head/tail ポインタが配列の末尾で先頭に戻る（wrap-around）動作を確認する。
 * NVMe の Submission Queue / Completion Queue は、このリングバッファ構造で
 * CPU とデバイスがコマンドのやりとりをする。
 *
 * 実行フロー:
 *   1. enqueue 1,2,3,4  → バッファが満杯になる
 *   2. enqueue 5         → "ring full" で失敗
 *   3. dequeue 2個       → 1,2 を取り出す
 *   4. enqueue 5,6       → tail が配列先頭に wrap-around
 *   5. dequeue 4個       → 3,4,5,6 を取り出す（head も wrap-around）
 */

#include <stdio.h>
#include <stdlib.h>
#include "dma.h"

/* リングバッファを初期化する */
static void ring_init(RingBuf *r) {
    r->head  = 0;
    r->tail  = 0;
    r->count = 0;
}

/*
 * リングバッファに値を追加する。
 *
 *   tail の位置に値を書き込み、tail を 1 進める。
 *   tail が RING_SIZE に達したら 0 に戻る（剰余演算）。
 *
 *   戻り値: 0 = 成功, -1 = バッファ満杯
 */
static int ring_enqueue(RingBuf *r, int value) {
    if (r->count >= RING_SIZE) {
        return -1;  /* 満杯 */
    }
    r->buf[r->tail % RING_SIZE] = value;
    r->tail++;
    r->count++;
    return 0;
}

/*
 * リングバッファから値を取り出す。
 *
 *   head の位置から値を読み出し、head を 1 進める。
 *   head が RING_SIZE に達したら 0 に戻る（剰余演算）。
 *
 *   戻り値: 0 = 成功, -1 = バッファ空
 */
static int ring_dequeue(RingBuf *r, int *out) {
    if (r->count == 0) {
        return -1;  /* 空 */
    }
    *out = r->buf[r->head % RING_SIZE];
    r->head++;
    r->count--;
    return 0;
}

int main(void) {
    RingBuf ring;
    ring_init(&ring);
    int val;

    /* 1. enqueue 1,2,3,4 → 満杯にする */
    for (int i = 1; i <= 4; i++) {
        if (ring_enqueue(&ring, i) == 0) {
            printf("enqueue: %d\n", i);
        }
    }

    /* 2. enqueue 5 → 満杯なので失敗 */
    if (ring_enqueue(&ring, 5) < 0) {
        printf("ring full\n");
    }

    /* 3. dequeue 2個 → 1, 2 を取り出す */
    for (int i = 0; i < 2; i++) {
        if (ring_dequeue(&ring, &val) == 0) {
            printf("dequeue: %d\n", val);
        }
    }

    /* 4. enqueue 5, 6 → tail が wrap-around して配列先頭に書き込む */
    for (int i = 5; i <= 6; i++) {
        if (ring_enqueue(&ring, i) == 0) {
            printf("enqueue: %d\n", i);
        }
    }

    /* 5. dequeue 残り全部 → 3, 4, 5, 6 */
    while (ring_dequeue(&ring, &val) == 0) {
        printf("dequeue: %d\n", val);
    }

    return 0;
}
