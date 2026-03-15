#ifndef SYNC_H
#define SYNC_H

/*
 * sync.h — 自作 spinlock
 *
 * __atomic ビルトインを使って CAS（Compare-And-Swap）相当の操作を行う。
 * pthread_mutex のような OS カーネルの助けを借りない、最もシンプルなロック。
 *
 * locked フィールド:
 *   0 → ロック未取得（空き）
 *   1 → ロック取得済み（他スレッドはビジーウェイト）
 */

typedef struct {
    int locked;
} spinlock_t;

/* spinlock の初期化マクロ */
#define SPINLOCK_INIT { .locked = 0 }

/*
 * spin_lock — ロックを取得する
 *
 * __atomic_exchange_n(&l->locked, 1, __ATOMIC_ACQUIRE):
 *   locked を 1 に書き換え、書き換え前の値を返す。
 *   - 前の値が 0 → 空きだったのでロック取得成功、ループ終了
 *   - 前の値が 1 → 他スレッドが保持中、ビジーウェイト継続
 *
 * __ATOMIC_ACQUIRE: このロード以降のメモリアクセスが、
 *   ロック取得より前に並び替えられないことを保証する。
 */
static inline void spin_lock(spinlock_t *l) {
    while (__atomic_exchange_n(&l->locked, 1, __ATOMIC_ACQUIRE)) {
        /* busy wait */
    }
}

/*
 * spin_unlock — ロックを解放する
 *
 * __atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE):
 *   locked を 0 に戻す。
 *
 * __ATOMIC_RELEASE: このストア以前のメモリアクセスが、
 *   ロック解放より後に並び替えられないことを保証する。
 */
static inline void spin_unlock(spinlock_t *l) {
    __atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE);
}

#endif /* SYNC_H */
