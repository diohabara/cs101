/*
 * pmm.c --- 物理メモリマネージャ（実装）
 *
 * ビットマップ方式で物理ページを管理する。
 * 管理対象のメモリ領域の先頭にビットマップを配置し、
 * 残りをアロケーション可能な領域として使う。
 *
 * 簡略化のため、連続した物理メモリ領域を 1 つだけ管理する。
 */

#include "pmm.h"
#include "uart.h"

/* ビットマップの最大サイズ（4096 バイト = 32768 ページ = 128MB 管理可能） */
#define BITMAP_MAX_BYTES 4096

static uint8_t bitmap[BITMAP_MAX_BYTES];
static uint64_t base_addr;
static uint64_t total_pages;

/* ビットマップ操作ヘルパー */
static inline void bitmap_set(uint64_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline int bitmap_test(uint64_t page) {
    return (bitmap[page / 8] >> (page % 8)) & 1;
}

void pmm_init(uint64_t mem_start, uint64_t mem_size) {
    base_addr = (mem_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);  /* ページ境界に切り上げ */
    total_pages = mem_size / PAGE_SIZE;
    if (total_pages > BITMAP_MAX_BYTES * 8) {
        total_pages = BITMAP_MAX_BYTES * 8;
    }

    /* 全ページを空きに初期化 */
    for (uint64_t i = 0; i < BITMAP_MAX_BYTES; i++) {
        bitmap[i] = 0;
    }
}

uint64_t pmm_alloc(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return base_addr + i * PAGE_SIZE;
        }
    }
    return 0;  /* メモリ不足 */
}

void pmm_free(uint64_t addr) {
    if (addr < base_addr) return;
    uint64_t page = (addr - base_addr) / PAGE_SIZE;
    if (page < total_pages) {
        bitmap_clear(page);
    }
}

uint64_t pmm_free_count(void) {
    uint64_t count = 0;
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) count++;
    }
    return count;
}
