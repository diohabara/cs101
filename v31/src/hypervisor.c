/*
 * hypervisor.c --- ハイパーバイザ統合
 *
 * v27-v31 の全コンポーネントを統合した Phase 3 のメインモジュール。
 *
 * ハイパーバイザの構成要素:
 *   1. VMX 検出・有効化 (vmx_detect.c)
 *   2. VMCS 設定 (vmcs.c)
 *   3. ゲスト実行 (guest.c)
 *   4. VM exit ハンドリング (vmexit_handler.c)
 *   5. EPT によるメモリ隔離 (ept.c)
 *
 * このファイルはハイパーバイザの初期化と実行を一括管理する。
 */

#include "vmx.h"
#include "vmcs.h"
#include "guest.h"
#include "vmexit.h"
#include "ept.h"
#include "uart.h"

/*
 * hypervisor_init --- ハイパーバイザを初期化して実行する
 *
 * Phase 3 のすべてのステージの機能を統合。
 * VMX の有無にかかわらず、教育的な出力を行う。
 */
void hypervisor_init(void) {
    uart_puts("=== Hypervisor Initialization ===\n");

    /* ステージ 1: VMX 検出 (v27) */
    uart_puts("\n[1/5] VMX Detection (v27)\n");
    int vmx_available = vmx_detect();
    if (vmx_available) {
        uart_puts("VT-x detected via CPUID\n");
    } else {
        uart_puts("VT-x not available (expected in container)\n");
    }

    /* ステージ 2: VMCS 設定 (v28) */
    uart_puts("\n[2/5] VMCS Setup (v28)\n");
    int vmcs_ready = 0;
    if (vmx_available) {
        if (vmcs_init() == 0) {
            uart_puts("VMCS configured with guest/host state\n");
            vmcs_ready = 1;
        }
    } else {
        uart_puts("VMCS: simulated setup\n");
        uart_puts("  Guest state: RIP, RSP, CR0, CR4, segments\n");
        uart_puts("  Host state: RIP (exit handler), RSP, CR0, CR4\n");
        uart_puts("  VM execution controls: HLT/IO/CPUID exit enabled\n");
    }

    /* ステージ 3: ゲスト実行 (v29) */
    uart_puts("\n[3/5] Guest Execution (v29)\n");
    if (vmcs_ready) {
        uart_puts("Launching guest with vmlaunch...\n");
        guest_run();
        /* vmlaunch 成功時は戻ってこない（vmexit_stub にジャンプ） */
        uart_puts("vmlaunch returned (failure or no guest code)\n");
    } else {
        guest_run(); /* シミュレーション */
    }

    /* ステージ 4: VM exit ハンドリング (v30) */
    uart_puts("\n[4/5] VM Exit Handling (v30)\n");
    if (!vmx_available) {
        vmexit_demo();
    }

    /* ステージ 5: EPT メモリ隔離 (v31) */
    uart_puts("\n[5/5] EPT Memory Isolation (v31)\n");
    ept_demo();

    /* 統合サマリ */
    uart_puts("\n=== Hypervisor Summary ===\n");
    uart_puts("Phase 3 components:\n");
    uart_puts("  v27: VMX detection and VMXON/VMXOFF\n");
    uart_puts("  v28: VMCS guest/host state configuration\n");
    uart_puts("  v29: Guest execution via vmlaunch\n");
    uart_puts("  v30: VM exit handling (CPUID/HLT/IO)\n");
    uart_puts("  v31: EPT memory isolation between guests\n");
    uart_puts("Hypervisor initialization complete\n");

    /* VMX を使った場合は無効化 */
    if (vmcs_ready) {
        vmx_disable();
        uart_puts("VMX disabled\n");
    }
}
