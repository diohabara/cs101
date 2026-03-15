/*
 * freelist_alloc.c --- フリーリストアロケータ（コアレッシング付き）
 *
 * malloc/free の基本的な仕組みを実装する。mmap で取得したプールを
 * ヘッダ付きチャンクに分割し、フリーリスト（空きブロックの連結リスト）で
 * 管理する。
 *
 * チャンクのメモリレイアウト:
 *   +-------------------+
 *   | BlockHeader       |   ← サイズと次ブロックへのポインタ
 *   |   size (size_t)   |
 *   |   next (pointer)  |
 *   +-------------------+
 *   | ユーザデータ領域   |   ← alloc() が返すアドレス
 *   |   ...             |
 *   +-------------------+
 *
 * これは実際の malloc（glibc の ptmalloc、jemalloc 等）と同じ原理。
 * malloc が返すポインタの直前には常にメタデータ（ヘッダ）がある。
 */

#include "alloc.h"
#include <string.h>

/* 各ブロックの先頭に置くヘッダ */
typedef struct BlockHeader {
    size_t               size;  /* ユーザデータ領域のサイズ（ヘッダ含まず） */
    struct BlockHeader  *next;  /* フリーリストの次のブロック（使用中は NULL） */
} BlockHeader;

#define HEADER_SIZE (align8(sizeof(BlockHeader)))

/* フリーリストアロケータの状態 */
typedef struct {
    char        *pool;      /* mmap で取得したメモリプール */
    size_t       pool_size; /* プール全体のサイズ */
    BlockHeader *free_list; /* フリーリストの先頭 */
} FreeListAllocator;

/*
 * アロケータを初期化する。
 * プール全体を 1 つの巨大な空きブロックとしてフリーリストに登録する。
 *
 * 初期状態:
 *   free_list → [header | .............. 空き領域 .............. ]
 *               ^                                                ^
 *               pool                                pool + pool_size
 */
static void fl_init(FreeListAllocator *a, size_t size) {
    a->pool      = (char *)pool_create(size);
    a->pool_size = size;

    /* プール全体を 1 つの空きブロックにする */
    a->free_list       = (BlockHeader *)a->pool;
    a->free_list->size = size - HEADER_SIZE;
    a->free_list->next = NULL;
}

/*
 * メモリを割り当てる（first-fit アルゴリズム）。
 *
 * フリーリストを先頭から探索し、要求サイズ以上の最初のブロックを返す。
 * ブロックが十分大きければ分割（split）する。
 *
 * split 前:
 *   [header|    大きな空きブロック    ]
 *
 * split 後:
 *   [header| 割当済み ][header| 残り空き ]
 */
static void *fl_alloc(FreeListAllocator *a, size_t size) {
    size = align8(size);

    BlockHeader **prev = &a->free_list;
    BlockHeader  *curr = a->free_list;

    while (curr != NULL) {
        if (curr->size >= size) {
            /* このブロックを使う */

            /* 分割できるか？（残りにヘッダ + 最小 8 バイトが必要） */
            if (curr->size >= size + HEADER_SIZE + 8) {
                /* ブロックを分割する */
                BlockHeader *new_block =
                    (BlockHeader *)((char *)curr + HEADER_SIZE + size);
                new_block->size = curr->size - size - HEADER_SIZE;
                new_block->next = curr->next;

                curr->size = size;
                *prev = new_block;
            } else {
                /* ブロック全体を割り当てる（分割の余地なし） */
                *prev = curr->next;
            }

            curr->next = NULL;  /* 使用中マーク */
            /* ヘッダの直後がユーザデータ領域 */
            return (char *)curr + HEADER_SIZE;
        }
        prev = &curr->next;
        curr = curr->next;
    }

    return NULL;  /* 空きブロックなし */
}

/*
 * メモリを解放してフリーリストに返す。
 *
 * 返却後、アドレス順にリストを巡回して隣接ブロックを結合（coalesce）する。
 * これにより、小さな空きブロックが断片化するのを防ぐ。
 *
 * coalesce 前:
 *   [free A][free B]   ← 隣接する 2 つの空きブロック
 *
 * coalesce 後:
 *   [free A+B      ]   ← 1 つの大きな空きブロックに結合
 */
static void fl_free(FreeListAllocator *a, void *ptr) {
    if (ptr == NULL) return;

    BlockHeader *block = (BlockHeader *)((char *)ptr - HEADER_SIZE);

    /* アドレス順にフリーリストへ挿入する */
    BlockHeader **prev = &a->free_list;
    BlockHeader  *curr = a->free_list;

    while (curr != NULL && curr < block) {
        prev = &curr->next;
        curr = curr->next;
    }

    block->next = curr;
    *prev = block;

    /* 後方のブロックと結合を試みる */
    if (curr != NULL &&
        (char *)block + HEADER_SIZE + block->size == (char *)curr) {
        block->size += HEADER_SIZE + curr->size;
        block->next = curr->next;
    }

    /* 前方のブロックと結合を試みる */
    if (*prev != block) {
        /* prev は block の 1 つ前を指している */
        BlockHeader *prev_block = (BlockHeader *)((char *)prev -
            offsetof(BlockHeader, next));
        /* ただし free_list 自体が prev の場合はスキップ */
        if ((char **)prev != (char **)&a->free_list &&
            (char *)prev_block + HEADER_SIZE + prev_block->size == (char *)block) {
            prev_block->size += HEADER_SIZE + block->size;
            prev_block->next = block->next;
        }
    }
}

/* プール全体を解放する */
static void fl_destroy(FreeListAllocator *a) {
    pool_destroy(a->pool, a->pool_size);
}

int main(void) {
    FreeListAllocator alloc;
    fl_init(&alloc, POOL_SIZE);

    printf("=== free-list allocator ===\n");

    /* ステップ 1: A と B を割り当てる */
    void *a = fl_alloc(&alloc, 64);
    void *b = fl_alloc(&alloc, 64);
    printf("alloc A (64 bytes) = %p\n", a);
    printf("alloc B (64 bytes) = %p\n", b);

    /* A のアドレスを記録しておく */
    void *a_addr = a;

    /* ステップ 2: A を解放する */
    fl_free(&alloc, a);
    printf("free A\n");

    /* ステップ 3: C を割り当てる（A と同じサイズ → A の跡地を再利用するはず） */
    void *c = fl_alloc(&alloc, 64);
    printf("alloc C (64 bytes) = %p\n", c);

    if (c == a_addr) {
        printf("reused: yes\n");
    } else {
        printf("reused: no\n");
    }

    /* ステップ 4: B, C を解放してコアレッシングをテスト */
    fl_free(&alloc, b);
    fl_free(&alloc, c);
    printf("free B, free C (coalescing)\n");

    /*
     * ステップ 5: 大きなブロックを割り当てる
     * B と C が結合されていれば、128 + ヘッダ分以上の領域が確保できる
     */
    void *d = fl_alloc(&alloc, 200);
    if (d != NULL) {
        printf("alloc D (200 bytes) = %p -> coalesced: yes\n", d);
    } else {
        printf("alloc D (200 bytes) = NULL -> coalesced: no\n");
    }

    fl_destroy(&alloc);

    /* 正常終了 */
    return 0;
}
