/*
 * guest.c --- ゲスト VM の実装
 *
 * 最小限のゲストコードを定義し、vmlaunch で実行する。
 *
 * ゲストコード（マシンコード）:
 *   48 C7 C0 42 00 00 00    mov rax, 0x42
 *   F4                      hlt
 *
 * ゲストが HLT を実行すると VM exit が発生する（VMCS の HLT exit ビットが有効）。
 * ホスト（ハイパーバイザ）は VM exit reason を読み取って HLT であることを確認する。
 *
 * VMX が利用できない環境では、同等の動作をシミュレーション出力する。
 */

#include "guest.h"
#include "vmx.h"
#include "vmcs.h"
#include "uart.h"

/*
 * ゲストコード（マシンコード直接指定）
 *
 * ゲスト VM はこのバイト列を直接実行する。
 * アセンブラを使わず、マシンコードを配列として定義する。
 *
 *   48 C7 C0 42 00 00 00  = mov rax, 0x42
 *   F4                    = hlt
 *
 * RAX に 0x42 を設定してから HLT で停止する。
 * HLT は VMCS の proc-based control で VM exit を発生させる設定になっている。
 */
static const uint8_t guest_code[] __attribute__((aligned(4096))) = {
    0x48, 0xC7, 0xC0, 0x42, 0x00, 0x00, 0x00, /* mov rax, 0x42 */
    0xF4,                                       /* hlt            */
};

const uint8_t *guest_get_code(void) {
    return guest_code;
}

uint32_t guest_get_code_size(void) {
    return sizeof(guest_code);
}

/* 10進数出力ヘルパー */
static void uart_put_hex_val(uint64_t n) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (n >> i) & 0xF;
        if (digit != 0 || started || i == 0) {
            uart_putc(hex[digit]);
            started = 1;
        }
    }
}

/* 10進数出力 */
static void uart_put_dec(uint64_t n) {
    if (n >= 10) {
        uart_put_dec(n / 10);
    }
    uart_putc('0' + (char)(n % 10));
}

/*
 * vmread_field --- VMCS フィールドの読み取り
 */
static inline uint64_t vmread_field(uint64_t field) {
    uint64_t value;
    __asm__ volatile (
        "vmread %1, %0\n"
        : "=r"(value)
        : "r"(field)
        : "cc"
    );
    return value;
}

/*
 * vmwrite_field_local --- VMCS フィールドへの書き込み（ローカル版）
 */
static inline void vmwrite_field_local(uint64_t field, uint64_t value) {
    __asm__ volatile (
        "vmwrite %1, %0\n"
        : : "r"(field), "r"(value) : "cc"
    );
}

/*
 * VM exit reason の定数
 */
#define VMEXIT_HLT   12
#define VMEXIT_CPUID  10
#define VMEXIT_IO     30

int guest_run(void) {
    if (!vmx_detect()) {
        /*
         * VMX 非対応環境: シミュレーション出力
         *
         * 実際のゲスト実行はできないが、何が起こるかを出力する。
         */
        uart_puts("Guest: skipped (no VT-x)\n");
        uart_puts("[simulated] Guest code: mov rax, 0x42; hlt\n");
        uart_puts("[simulated] vmlaunch: guest started\n");
        uart_puts("[simulated] VM exit: reason=HLT(12), guest RAX=0x42\n");
        uart_puts("[simulated] Guest executed successfully\n");
        return 0;
    }

    /*
     * VMX 対応環境: 実際のゲスト実行
     */

    /* VMCS にゲスト RIP を設定 */
    vmwrite_field_local(VMCS_GUEST_RIP, (uint64_t)guest_code);

    uart_puts("Guest RIP set to ");
    uart_put_hex_val((uint64_t)guest_code);
    uart_puts("\n");

    /*
     * vmlaunch --- ゲスト実行開始
     *
     * vmlaunch は VMCS の launch 状態が "clear" のときに使用する（初回実行）。
     * 2 回目以降は vmresume を使う（v30 で実装）。
     *
     * vmlaunch が成功すると:
     *   - CPU はゲスト状態に切り替わる
     *   - ゲスト RIP からゲストコードが実行される
     *   - VM exit が発生するとホスト RIP にジャンプする
     *
     * vmlaunch が失敗すると:
     *   - CF=1 (VMfailInvalid) または ZF=1 (VMfailValid)
     *   - 制御は vmlaunch の次の命令に進む
     */
    uint8_t cf_err, zf_err;
    __asm__ volatile (
        "vmlaunch\n"
        "setc %0\n"
        "setz %1\n"
        : "=rm"(cf_err), "=rm"(zf_err)
        : : "cc", "memory"
    );

    /*
     * ここに到達 = vmlaunch が失敗した
     * 成功した場合は VM exit 時にホスト RIP (vmexit_stub) にジャンプする。
     */
    if (cf_err) {
        uart_puts("vmlaunch failed: VMfailInvalid\n");
    } else if (zf_err) {
        uart_puts("vmlaunch failed: VMfailValid\n");
        /* VM-instruction error field を読む */
        uint64_t error = vmread_field(0x4400); /* VM_INSTRUCTION_ERROR */
        uart_puts("  error code: ");
        uart_put_dec(error);
        uart_puts("\n");
    }

    return -1;
}
