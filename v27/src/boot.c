/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v27: VMX 検出）
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. VMX 検出 --- CPUID で VT-x サポートを確認
 *   6. VMX 有効化 --- vmxon で VMX モードに入る（VT-x 対応時のみ）
 *   7. VMX 無効化 --- vmxoff で VMX モードから出る
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "vmx.h"

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

    if (vmx_detect()) {
        uart_puts("VT-x detected via CPUID\n");
        if (vmx_enable() == 0) {
            uart_puts("VMXON success\n");
            vmx_disable();
            uart_puts("VMXOFF success\n");
        }
    } else {
        uart_puts("VT-x not available (expected in container)\n");
    }

    /* 停止 */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
