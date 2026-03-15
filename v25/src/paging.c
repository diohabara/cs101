/*
 * paging.c --- ページング（実装）
 *
 * x86-64 の 4 レベルページテーブルを構築し、CR3 にロードする。
 *
 * QEMU が -kernel で起動した時点で既にロングモード（ページング有効）だが、
 * 自前のページテーブルを構築して CR3 を書き換えることで、
 * メモリマッピングを完全に制御する。
 *
 * 簡略化のため、最初の 4MB をアイデンティティマッピング（仮想=物理）する。
 * カーネルが 0x100000 (1MB) にロードされるため、この範囲で十分。
 */

#include "paging.h"
#include "uart.h"

/*
 * ページテーブル用の静的メモリ
 * 各テーブルは 512 エントリ × 8 バイト = 4096 バイト（1 ページ）
 * aligned(4096) でページ境界に配置する。
 */
static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt[512] __attribute__((aligned(4096)));
static uint64_t pd[512]   __attribute__((aligned(4096)));
static uint64_t pt0[512]  __attribute__((aligned(4096)));
static uint64_t pt1[512]  __attribute__((aligned(4096)));

void paging_init(void) {
    /* テーブルをゼロクリア */
    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i]   = 0;
        pt0[i]  = 0;
        pt1[i]  = 0;
    }

    /* PML4[0] → PDPT */
    pml4[0] = ((uint64_t)&pdpt) | PTE_PRESENT | PTE_WRITABLE;

    /* PDPT[0] → PD */
    pdpt[0] = ((uint64_t)&pd) | PTE_PRESENT | PTE_WRITABLE;

    /* PD[0] → PT0 (最初の 2MB) */
    pd[0] = ((uint64_t)&pt0) | PTE_PRESENT | PTE_WRITABLE;

    /* PD[1] → PT1 (次の 2MB: 2MB-4MB) */
    pd[1] = ((uint64_t)&pt1) | PTE_PRESENT | PTE_WRITABLE;

    /* PT0: 0x000000 - 0x1FFFFF をアイデンティティマッピング */
    for (int i = 0; i < 512; i++) {
        pt0[i] = (uint64_t)(i * 4096) | PTE_PRESENT | PTE_WRITABLE;
    }

    /* PT1: 0x200000 - 0x3FFFFF をアイデンティティマッピング */
    for (int i = 0; i < 512; i++) {
        pt1[i] = (uint64_t)(0x200000 + i * 4096) | PTE_PRESENT | PTE_WRITABLE;
    }

    /* CR3 にページテーブルのアドレスをロード */
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint64_t)&pml4) : "memory");
}
