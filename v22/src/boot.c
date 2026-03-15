/*
 * boot.c --- ベアメタルカーネルのエントリポイント
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. PMM 初期化 --- 物理メモリアロケータ（バディアロケータ）
 *   6. ページ割り当てテスト --- 4 ページ割り当てて検証
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"

/*
 * uart_put_hex --- 16進数を UART に出力
 */
static void uart_put_hex(uint64_t n) {
    uart_puts("0x");
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (n >> i) & 0xF;
        if (nibble != 0) started = 1;
        if (started || i == 0) {
            uart_putc(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
        }
    }
}

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded\n");

    idt_init();
    uart_puts("IDT loaded\n");

    timer_init(100);
    uart_puts("Timer started: 100Hz\n");

    /* 割り込み有効化 */
    __asm__ volatile ("sti");

    /*
     * PMM 初期化: 0x200000 - 0x400000 (2MB) を管理対象にする
     *
     * カーネルは 0x100000 にロードされるので、0x200000 以降は空き。
     * 2MB = 512 ページ (4KB/ページ)
     */
    pmm_init(0x200000, 0x200000);

    /* 4 ページ (order 0 = 4KB) を割り当て */
    void *pages[4];
    int unique = 1;

    for (int i = 0; i < 4; i++) {
        pages[i] = pmm_alloc(0);
        uart_puts("PMM: allocated page at ");
        uart_put_hex((uint64_t)pages[i]);
        uart_puts("\n");
    }

    /* 全てのアドレスが異なることを確認 */
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (pages[i] == pages[j]) {
                unique = 0;
            }
        }
    }

    if (unique) {
        uart_puts("4 unique pages\n");
    } else {
        uart_puts("ERROR: duplicate pages\n");
    }

    /* ページを解放 */
    for (int i = 0; i < 4; i++) {
        pmm_free(pages[i], 0);
    }
    uart_puts("PMM: all pages freed\n");

    /* 停止: 割り込み無効化 + hlt ループ */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
