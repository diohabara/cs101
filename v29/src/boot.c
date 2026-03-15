/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v29: ゲスト実行）
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. VMX 検出 + VMCS 初期化 --- VMCS のフィールドを設定
 *   6. ゲスト実行 --- vmlaunch でゲストコードを実行
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "vmx.h"
#include "vmcs.h"
#include "guest.h"

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

    /* VMCS 初期化 + ゲスト実行 */
    if (vmcs_init() == 0) {
        uart_puts("VMCS ready, launching guest...\n");
        guest_run();
        /* guest_run から戻ってきた場合は vmlaunch 失敗 */
        vmx_disable();
    } else {
        /* VMX 非対応: シミュレーション */
        guest_run();
    }

    /* 停止 */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
