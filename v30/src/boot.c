/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v30: VM exit ハンドリング）
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. VMX 検出 + VMCS 初期化 --- VMCS のフィールドを設定
 *   6. ゲスト実行 --- vmlaunch でゲストコードを実行
 *   7. VM exit ハンドリング --- exit reason に応じた処理
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "vmx.h"
#include "vmcs.h"
#include "guest.h"
#include "vmexit.h"

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
        /* VMX 対応環境 */
        if (vmcs_init() == 0) {
            uart_puts("VMCS ready, launching guest...\n");
            guest_run();
            /*
             * guest_run から戻ってきた場合:
             *   vmlaunch 成功 → vmexit_stub にジャンプ（ここには来ない）
             *   vmlaunch 失敗 → ここに到達
             */
            vmx_disable();
        }
    } else {
        /* VMX 非対応環境: シミュレーション */
        uart_puts("VT-x not available (expected in container)\n");
        guest_run();    /* シミュレーション出力 */
        vmexit_demo();  /* VM exit ハンドリングのデモ */
    }

    /* 停止 */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
