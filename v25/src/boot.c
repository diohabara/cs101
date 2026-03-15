/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v25）
 *
 * v24 からの累積:
 *   v18: UART 初期化 + シリアル出力
 *   v19: GDT 初期化 + セグメントセレクタ設定
 *   v20: IDT 初期化 + PIC 再マッピング
 *   v21: PIT タイマー初期化
 *   v22: 物理メモリマネージャ (PMM) 初期化
 *   v23: ページング初期化
 *   v24: syscall/sysret 初期化
 *   v25: DMA コントローラ初期化 + DMA 転送テスト
 *
 * v25 では DMA デスクリプタリングを構築し、メモリ間転送を実行する。
 * PMM で確保したページを転送元/転送先バッファとして使用し、
 * データが正しくコピーされることを確認する。
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "usermode.h"
#include "dma_hw.h"

/* 数値を 16 進で出力するヘルパー */
static void uart_put_hex(uint64_t val) {
    const char hex[] = "0123456789abcdef";
    uart_puts("0x");
    /* 先頭ゼロを省略せずに 16 桁出力 */
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

/* DMA リング（静的確保） */
static struct dma_ring dma_ring;

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded\n");

    idt_init();
    uart_puts("IDT loaded\n");

    timer_init(100);
    uart_puts("Timer started\n");

    pmm_init(0x400000, 0x100000);
    uart_puts("PMM initialized\n");

    paging_init();
    uart_puts("Paging enabled\n");

    usermode_init();
    uart_puts("Syscall handler ready\n");

    /*
     * DMA テスト
     *
     * 1. PMM でソースページとデスティネーションページを確保
     * 2. ソースページにテストパターンを書き込み
     * 3. DMA デスクリプタを投入
     * 4. DMA エンジンを実行
     * 5. デスティネーションのデータを検証
     */
    dma_init(&dma_ring);
    uart_puts("DMA initialized\n");

    /* ソース/デスティネーションのページを確保 */
    uint64_t src_page = pmm_alloc();
    uint64_t dst_page = pmm_alloc();

    if (src_page == 0 || dst_page == 0) {
        uart_puts("Failed to allocate pages for DMA test\n");
        for (;;) __asm__ volatile ("cli\nhlt");
    }

    uart_puts("DMA src page: ");
    uart_put_hex(src_page);
    uart_putc('\n');
    uart_puts("DMA dst page: ");
    uart_put_hex(dst_page);
    uart_putc('\n');

    /* ソースページにテストパターンを書き込み */
    uint8_t *src = (uint8_t *)(uintptr_t)src_page;
    uint8_t *dst = (uint8_t *)(uintptr_t)dst_page;
    for (int i = 0; i < 256; i++) {
        src[i] = (uint8_t)i;
        dst[i] = 0;  /* デスティネーションをゼロクリア */
    }

    /* DMA 転送を投入 */
    if (dma_submit(&dma_ring, src_page, dst_page, 256) == 0) {
        uart_puts("DMA transfer submitted\n");
    } else {
        uart_puts("DMA submit failed\n");
        for (;;) __asm__ volatile ("cli\nhlt");
    }

    /* DMA エンジンを実行 */
    dma_process(&dma_ring);
    uart_puts("DMA transfer complete\n");

    /* データ検証 */
    int ok = 1;
    for (int i = 0; i < 256; i++) {
        if (dst[i] != (uint8_t)i) {
            ok = 0;
            break;
        }
    }

    if (ok) {
        uart_puts("DMA data verified\n");
    } else {
        uart_puts("DMA data mismatch!\n");
    }

    /* 確保したページを解放 */
    pmm_free(src_page);
    pmm_free(dst_page);

    /* 停止: 割り込み無効化 + hlt ループ */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
