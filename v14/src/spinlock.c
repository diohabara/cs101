/*
 * spinlock.c — データ競合の体験と spinlock による排他制御
 *
 * 2 つのスレッドが共有変数を 100000 回ずつインクリメントする。
 *   - ロックなし: 結果が 200000 にならない（データ競合）
 *   - spinlock あり: 結果が必ず 200000 になる
 */

#include <pthread.h>
#include <stdio.h>
#include "sync.h"

#define ITERATIONS 100000

/* ロックなしの共有データ */
static int counter_nolock = 0;

/* spinlock 付きの共有データ */
static int counter_locked = 0;
static spinlock_t lock = SPINLOCK_INIT;

/*
 * ロックなしインクリメント
 *
 * counter_nolock++ は以下の 3 命令に分解される:
 *   1. メモリから値を読む (load)
 *   2. レジスタで +1 する  (add)
 *   3. メモリに書き戻す   (store)
 *
 * 2 スレッドが同時に実行すると、load-add-store の間に
 * 他スレッドの書き込みが割り込み、更新が失われる。
 */
static void *increment_nolock(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        counter_nolock++;
    }
    return NULL;
}

/*
 * spinlock 付きインクリメント
 *
 * spin_lock で排他区間に入り、spin_unlock で出る。
 * 同時に 1 スレッドしかクリティカルセクションに入れないため、
 * load-add-store が不可分になる。
 */
static void *increment_locked(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        spin_lock(&lock);
        counter_locked++;
        spin_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    /* --- ロックなし: データ競合デモ --- */
    pthread_create(&t1, NULL, increment_nolock, NULL);
    pthread_create(&t2, NULL, increment_nolock, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("without lock: %d\n", counter_nolock);

    /* --- spinlock あり: 正しい排他制御 --- */
    pthread_create(&t1, NULL, increment_locked, NULL);
    pthread_create(&t2, NULL, increment_locked, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("with lock: %d\n", counter_locked);

    return 0;
}
