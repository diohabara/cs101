/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v31: ハイパーバイザ統合）
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. ハイパーバイザ初期化 --- VMX, VMCS, Guest, VM exit, EPT を統合実行
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"

/* hypervisor.c で定義 */
void hypervisor_init(void);

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded\n");

    idt_init();
    uart_puts("IDT loaded\n");

    timer_init(100);
    uart_puts("Timer started: 100Hz\n");

    /* --- Phase 3: Hypervisor --- */
    uart_puts("--- Phase 3: Hypervisor ---\n");

    hypervisor_init();

    /* 停止 */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
