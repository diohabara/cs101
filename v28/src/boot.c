/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v28: VMCS 設定）
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. VMX 検出 --- CPUID で VT-x サポートを確認
 *   6. VMCS 初期化 --- ゲスト/ホスト状態と VM 実行制御を設定
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "vmx.h"
#include "vmcs.h"

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

    /* VMCS 初期化（内部で VMX 検出・有効化も行う） */
    if (vmcs_init() == 0) {
        uart_puts("VMCS setup complete\n");
        vmcs_dump();
        vmx_disable();
        uart_puts("VMX disabled\n");
    } else {
        uart_puts("VMCS setup skipped or failed\n");
    }

    /* 停止 */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
