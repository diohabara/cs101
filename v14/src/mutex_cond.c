/*
 * mutex_cond.c — 生産者-消費者パターン（pthread_mutex + pthread_cond）
 *
 * 有界バッファ（サイズ 4）を介して、
 * 生産者スレッドが値 0-9 を投入し、消費者スレッドが全て読み出す。
 *
 * spinlock との違い:
 *   spinlock: ロック取得までビジーウェイト（CPU を消費し続ける）
 *   mutex + cond: 条件が満たされるまでスレッドをスリープさせる（CPU に優しい）
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE   4
#define NUM_ITEMS  10

/* 有界バッファ（リングバッファ） */
static int buffer[BUF_SIZE];
static int buf_count = 0;   /* バッファ内の要素数 */
static int buf_in    = 0;   /* 次の書き込み位置 */
static int buf_out   = 0;   /* 次の読み出し位置 */

static pthread_mutex_t mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  not_full  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  not_empty = PTHREAD_COND_INITIALIZER;

/*
 * 生産者: 値 0, 1, 2, ..., 9 をバッファに投入
 *
 * バッファが満杯なら not_full を待つ。
 * 投入後、消費者に not_empty を通知する。
 */
static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < NUM_ITEMS; i++) {
        pthread_mutex_lock(&mtx);

        /* バッファが満杯なら待機 */
        while (buf_count == BUF_SIZE) {
            pthread_cond_wait(&not_full, &mtx);
        }

        buffer[buf_in] = i;
        buf_in = (buf_in + 1) % BUF_SIZE;
        buf_count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

/*
 * 消費者: バッファから 10 個の値を読み出す
 *
 * バッファが空なら not_empty を待つ。
 * 読み出し後、生産者に not_full を通知する。
 */
static void *consumer(void *arg) {
    int *received = (int *)arg;
    for (int i = 0; i < NUM_ITEMS; i++) {
        pthread_mutex_lock(&mtx);

        /* バッファが空なら待機 */
        while (buf_count == 0) {
            pthread_cond_wait(&not_empty, &mtx);
        }

        received[i] = buffer[buf_out];
        buf_out = (buf_out + 1) % BUF_SIZE;
        buf_count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

int main(void) {
    pthread_t prod, cons;
    int received[NUM_ITEMS];
    memset(received, -1, sizeof(received));

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, received);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    /* 受信した値を表示 */
    printf("consumed:");
    for (int i = 0; i < NUM_ITEMS; i++) {
        printf(" %d", received[i]);
    }
    printf("\n");

    /* 検証: 0-9 が全て揃っているか */
    int ok = 1;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (received[i] != i) {
            ok = 0;
            break;
        }
    }

    if (ok) {
        printf("all values received\n");
    } else {
        printf("MISMATCH: values are incorrect\n");
        return 1;
    }

    return 0;
}
