/*
 * bump_alloc.c --- バンプアロケータ
 *
 * 最もシンプルなメモリアロケータ。mmap で取得したプールの先頭から
 * 順にポインタを進める（bump する）だけで割り当てを行う。
 *
 * 特徴:
 *   - alloc は O(1)（ポインタを進めるだけ）
 *   - free は個別にできない（プール全体を一括解放）
 *   - ゲームの 1 フレーム内で大量に小さなオブジェクトを割り当てて、
 *     フレーム終了時にプール全体をリセットする用途に最適
 *
 * メモリレイアウト:
 *   pool                          pool + POOL_SIZE
 *   |                                            |
 *   v                                            v
 *   [  alloc 1  ][  alloc 2  ][  alloc 3  ][ ... 未使用 ... ]
 *   ^            ^            ^            ^
 *   pool         next(+s1)    next(+s2)    next(+s3)
 */

#include "alloc.h"

/* バンプアロケータの状態 */
typedef struct {
    char  *pool;      /* mmap で取得したメモリ領域の先頭 */
    size_t offset;    /* 次の割り当て位置（プール先頭からのオフセット） */
    size_t capacity;  /* プール全体のサイズ */
} BumpAllocator;

/*
 * バンプアロケータを初期化する。
 * mmap でメモリプールを取得し、オフセットを 0 に設定する。
 */
static void bump_init(BumpAllocator *a, size_t pool_size) {
    a->pool     = (char *)pool_create(pool_size);
    a->offset   = 0;
    a->capacity = pool_size;
}

/*
 * メモリを割り当てる。
 *
 * 現在のオフセットを返し、オフセットを size 分だけ進める。
 * 空きが足りなければ NULL を返す。
 *
 * アセンブリ相当:
 *   mov rax, [bump_offset]   ; 現在位置を取得
 *   add [bump_offset], rsi   ; ポインタを進める
 *   ; rax が返却アドレス
 */
static void *bump_alloc(BumpAllocator *a, size_t size) {
    size = align8(size);  /* 8 バイトアライメント */

    if (a->offset + size > a->capacity) {
        return NULL;  /* プール枯渇 */
    }

    void *ptr = a->pool + a->offset;
    a->offset += size;
    return ptr;
}

/* プール全体を解放する（個別 free はできない） */
static void bump_destroy(BumpAllocator *a) {
    pool_destroy(a->pool, a->capacity);
}

int main(void) {
    BumpAllocator alloc;
    bump_init(&alloc, POOL_SIZE);

    /* 3 つのブロックを割り当てる */
    void *a = bump_alloc(&alloc, 64);
    void *b = bump_alloc(&alloc, 128);
    void *c = bump_alloc(&alloc, 256);

    printf("bump_alloc(64)  = %p\n", a);
    printf("bump_alloc(128) = %p\n", b);
    printf("bump_alloc(256) = %p\n", c);

    /* 3 つとも異なるアドレスであることを検証 */
    int unique = 0;
    if (a != NULL) unique++;
    if (b != NULL && b != a) unique++;
    if (c != NULL && c != a && c != b) unique++;

    printf("unique addresses: %d\n", unique);

    /* アドレスが連続して増加していることを確認 */
    if ((uintptr_t)b > (uintptr_t)a &&
        (uintptr_t)c > (uintptr_t)b) {
        printf("addresses are sequential: yes\n");
    } else {
        printf("addresses are sequential: no\n");
    }

    bump_destroy(&alloc);

    /* exit code = ユニークなアドレスの数（期待値: 3） */
    return unique;
}
