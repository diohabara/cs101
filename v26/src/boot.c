/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v26）
 *
 * Phase 2 最終段階: ミニ OS 統合
 *
 * v18-v25 で構築した全コンポーネントを統合し、
 * 協調的マルチタスクカーネルを起動する。
 *
 * 起動シーケンス:
 *   1. UART 初期化 (v18)
 *   2. GDT 初期化 (v19)
 *   3. IDT 初期化 + PIC 再マッピング (v20)
 *   4. PIT タイマー初期化 (v21)
 *   5. PMM 初期化 (v22)
 *   6. ページング初期化 (v23)
 *   7. syscall 機構初期化 (v24)
 *   8. タスク登録 + カーネルメインループ (v26)
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "usermode.h"
#include "dma_hw.h"
#include "kernel.h"

/* task.c で定義されたタスク関数 */
extern void task_alpha(void);
extern void task_beta(void);

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    /* === Phase 2 初期化シーケンス === */

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

    uart_puts("=== Phase 2 init complete ===\n");

    /* === タスク登録 === */

    /*
     * 2 つのタスクを登録し、それぞれ 3 回ずつ実行する。
     * ラウンドロビンで交互に実行される。
     *
     * 期待される出力:
     *   [Alpha] iteration 3 remaining
     *     Alpha working
     *   [Beta] iteration 3 remaining
     *     Beta working
     *   [Alpha] iteration 2 remaining
     *     Alpha working
     *   [Beta] iteration 2 remaining
     *     Beta working
     *   [Alpha] iteration 1 remaining
     *     Alpha working
     *   [Alpha] completed
     *   [Beta] iteration 1 remaining
     *     Beta working
     *   [Beta] completed
     *   Kernel: all tasks done
     */
    kernel_add_task("Alpha", task_alpha, 3);
    kernel_add_task("Beta", task_beta, 3);

    /* カーネルメインループ */
    kernel_main();

    /* 停止: 割り込み無効化 + hlt ループ */
    uart_puts("System halted\n");
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
