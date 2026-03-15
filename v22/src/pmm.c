/*
 * pmm.c --- 物理メモリアロケータ（バディアロケータ実装）
 *
 * バディアロケータの動作原理:
 *
 * 割り当て (pmm_alloc):
 *   1. 要求オーダーのフリーリストにブロックがあればそれを返す
 *   2. なければ上位オーダーのブロックを取得して半分に分割（split）
 *   3. 一方を返し、もう一方をフリーリストに追加
 *
 * 解放 (pmm_free):
 *   1. ブロックをフリーリストに返却
 *   2. バディ（隣接ブロック）が空いていれば結合（merge）して上位オーダーに昇格
 *   3. 結合を最大オーダーまで繰り返す
 *
 * バディアドレスの計算:
 *   バディ = ブロックアドレス XOR (ブロックサイズ)
 *   例: アドレス 0x200000, order 0 (4KB)
 *       バディ = 0x200000 ^ 0x1000 = 0x201000
 */

#include "pmm.h"
#include "uart.h"

/*
 * フリーリストノード
 *
 * 空きページの先頭 8 バイトに次のノードへのポインタを格納する。
 * 追加のメモリを消費しない（空きページ自体をリストノードとして使う）。
 */
struct free_block {
    struct free_block *next;
};

/* オーダーごとのフリーリストヘッド */
static struct free_block *free_lists[MAX_ORDER + 1];

/* 管理対象メモリの範囲（バディ計算の範囲チェックに使用） */
static uint64_t mem_region_start;
static uint64_t mem_region_end;

/*
 * uart_put_hex --- 16進数を UART に出力
 *
 * printf がないベアメタル環境で 64 ビットアドレスを表示するためのヘルパー。
 */
static void uart_put_hex(uint64_t n) {
    uart_puts("0x");
    /* 先頭のゼロをスキップするフラグ */
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (n >> i) & 0xF;
        if (nibble != 0) started = 1;
        if (started || i == 0) {
            uart_putc(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
        }
    }
}

/*
 * uart_put_dec --- 10進数を UART に出力
 */
static void uart_put_dec(uint64_t n) {
    if (n >= 10) {
        uart_put_dec(n / 10);
    }
    uart_putc('0' + (char)(n % 10));
}

void pmm_init(uint64_t mem_start, uint64_t mem_size) {
    mem_region_start = mem_start;
    mem_region_end   = mem_start + mem_size;

    /* フリーリストを初期化 */
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_lists[i] = (void *)0;
    }

    /*
     * メモリ領域を最大オーダーのブロックに分割してフリーリストに追加。
     * 各アドレスについて、入る最大のオーダーを見つけてそのフリーリストに追加する。
     */
    uint64_t addr = mem_start;
    while (addr < mem_region_end) {
        /* このアドレスに入る最大のオーダーを見つける */
        int order = MAX_ORDER;
        while (order > 0) {
            uint64_t block_size = (uint64_t)PAGE_SIZE << order;
            /* アラインメントチェックと範囲チェック */
            if ((addr & (block_size - 1)) == 0 && addr + block_size <= mem_region_end) {
                break;
            }
            order--;
        }

        uint64_t block_size = (uint64_t)PAGE_SIZE << order;

        /* フリーリストの先頭に追加 */
        struct free_block *block = (struct free_block *)addr;
        block->next = free_lists[order];
        free_lists[order] = block;

        addr += block_size;
    }

    uart_puts("PMM: initialized ");
    uart_put_dec(mem_size / PAGE_SIZE);
    uart_puts(" pages (");
    uart_put_hex(mem_start);
    uart_puts(" - ");
    uart_put_hex(mem_region_end);
    uart_puts(")\n");
}

void *pmm_alloc(int order) {
    if (order > MAX_ORDER) {
        return (void *)0;
    }

    /* 要求オーダー以上のフリーリストを探す */
    int current = order;
    while (current <= MAX_ORDER && free_lists[current] == (void *)0) {
        current++;
    }

    if (current > MAX_ORDER) {
        uart_puts("PMM: out of memory\n");
        return (void *)0;
    }

    /* フリーリストからブロックを取得 */
    struct free_block *block = free_lists[current];
    free_lists[current] = block->next;

    /*
     * 大きすぎるブロックを分割（split）
     *
     * order 2 のブロックを order 0 で要求した場合:
     *   order 2 (16KB) を分割 → order 1 (8KB) x 2
     *   片方をフリーリストに追加、もう片方をさらに分割
     *   order 1 (8KB) を分割 → order 0 (4KB) x 2
     *   片方をフリーリストに追加、もう片方を返す
     */
    while (current > order) {
        current--;
        uint64_t buddy_addr = (uint64_t)block + ((uint64_t)PAGE_SIZE << current);
        struct free_block *buddy = (struct free_block *)buddy_addr;
        buddy->next = free_lists[current];
        free_lists[current] = buddy;
    }

    return (void *)block;
}

void pmm_free(void *addr, int order) {
    if (addr == (void *)0 || order > MAX_ORDER) {
        return;
    }

    struct free_block *block = (struct free_block *)addr;

    /*
     * バディとのマージを試みる
     *
     * バディアドレス = ブロックアドレス XOR ブロックサイズ
     * バディがフリーリストにあればマージして上位オーダーに昇格。
     * これを最大オーダーまで繰り返す。
     */
    while (order < MAX_ORDER) {
        uint64_t block_size = (uint64_t)PAGE_SIZE << order;
        uint64_t buddy_addr = (uint64_t)block ^ block_size;

        /* バディが管理対象範囲内かチェック */
        if (buddy_addr < mem_region_start || buddy_addr + block_size > mem_region_end) {
            break;
        }

        /* バディがフリーリストにあるか探す */
        struct free_block **prev = &free_lists[order];
        struct free_block *curr = free_lists[order];
        int found = 0;

        while (curr != (void *)0) {
            if ((uint64_t)curr == buddy_addr) {
                /* バディを発見: フリーリストから除去 */
                *prev = curr->next;
                found = 1;
                break;
            }
            prev = &curr->next;
            curr = curr->next;
        }

        if (!found) {
            break;  /* バディが空いていなければマージ終了 */
        }

        /* マージ: 小さい方のアドレスを新しいブロックとする */
        if (buddy_addr < (uint64_t)block) {
            block = (struct free_block *)buddy_addr;
        }
        order++;
    }

    /* フリーリストに追加 */
    block->next = free_lists[order];
    free_lists[order] = block;
}
