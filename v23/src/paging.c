/*
 * paging.c --- 4レベルページテーブルの実装
 *
 * x86-64 のアドレス変換:
 *
 *   仮想アドレス 0x00000000_001234AB の変換例:
 *
 *   [47:39] = 0   → PML4[0]
 *   [38:30] = 0   → PDPT[0]
 *   [29:21] = 9   → PD[9]       (0x1234AB >> 21 = 9)
 *   [20:12] = 0x234 → PT[0x234] (ただし12ビットなので 0x234 & 0x1FF = 0x34)
 *   [11:0]  = 0xAB → ページ内オフセット
 *
 * 各テーブルは 4KB ページに格納し、512 個の 8 バイトエントリを持つ。
 * 上位ビットが物理アドレス、下位 12 ビットがフラグ。
 */

#include "paging.h"
#include "pmm.h"
#include "uart.h"

/* PML4 テーブルのアドレス */
static uint64_t *pml4;

/*
 * memset_page --- ページをゼロクリア
 *
 * ページテーブル用に割り当てたページを初期化する。
 * libc の memset がないので自前で実装。
 */
static void memset_page(void *addr) {
    uint64_t *p = (uint64_t *)addr;
    for (int i = 0; i < 512; i++) {
        p[i] = 0;
    }
}

/*
 * alloc_table --- ページテーブル用のゼロクリア済みページを割り当て
 *
 * PMM から 1 ページ (order 0 = 4KB) を取得してゼロクリアする。
 * ページテーブルのエントリが全て「不在」状態で始まるようにする。
 */
static uint64_t *alloc_table(void) {
    void *page = pmm_alloc(0);
    if (page == (void *)0) {
        uart_puts("paging: out of memory for page table\n");
        for (;;) {
            __asm__ volatile ("cli\nhlt");
        }
    }
    memset_page(page);
    return (uint64_t *)page;
}

void paging_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    /*
     * 仮想アドレスからテーブルインデックスを抽出
     *
     * 各レベルのインデックスは 9 ビット (0-511)。
     * ビットシフトと AND マスクで取得する。
     */
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    /* PML4 → PDPT: エントリがなければ新規テーブルを割り当て */
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        uint64_t *pdpt = alloc_table();
        pml4[pml4_idx] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    }
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & ~0xFFFUL);

    /* PDPT → PD */
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        uint64_t *pd = alloc_table();
        pdpt[pdpt_idx] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    }
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & ~0xFFFUL);

    /* PD → PT */
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        uint64_t *pt = alloc_table();
        pd[pd_idx] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE;
    }
    uint64_t *pt = (uint64_t *)(pd[pd_idx] & ~0xFFFUL);

    /* PT エントリに物理アドレスとフラグを設定 */
    pt[pt_idx] = (phys & ~0xFFFUL) | flags;
}

void paging_init(void) {
    uart_puts("Paging: setting up 4-level page tables\n");

    /* PML4 テーブルを割り当て */
    pml4 = alloc_table();

    /*
     * 最初の 4MB をアイデンティティマップ
     *
     * アイデンティティマップ = 仮想アドレス == 物理アドレス
     * これにより、ページング有効化後もカーネルコード、UART、
     * ページテーブル自体がそのままのアドレスでアクセスできる。
     *
     * 4MB = 1024 ページ (4KB/ページ)
     */
    for (uint64_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        paging_map(addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    uart_puts("Paging: identity mapped 0x0 - 0x400000 (4MB)\n");

    /*
     * CR3 に PML4 の物理アドレスを設定
     *
     * CR3 (Control Register 3) = ページテーブルのルートアドレス。
     * CR3 に書き込むと CPU がページテーブルを切り替える。
     * QEMU の -kernel ではページングが既にどう設定されているか不定だが、
     * アイデンティティマップなので安全に切り替えられる。
     */
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint64_t)pml4) : "memory");

    uart_puts("Paging enabled\n");
}
