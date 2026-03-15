/*
 * vmexit_handler.c --- VM exit ハンドリング
 *
 * VM exit が発生すると CPU はホスト RIP にジャンプする。
 * ハンドラは exit reason を読み取り、適切な処理を行う。
 *
 * 処理パターン:
 *   CPUID exit → ハイパーバイザが CPUID 結果をエミュレートしてゲスト復帰
 *   HLT exit   → ゲストが停止を要求。ハイパーバイザが管理判断する
 *   I/O exit   → ゲストの I/O を仮想デバイスで処理してゲスト復帰
 *
 * v30 では VMX 非対応環境でのシミュレーション出力と、
 * VMX 対応環境での exit reason 読み取りを実装する。
 */

#include "vmexit.h"
#include "vmcs.h"
#include "vmx.h"
#include "uart.h"

/* 10進数出力ヘルパー */
static void uart_put_dec(uint64_t n) {
    if (n >= 10) {
        uart_put_dec(n / 10);
    }
    uart_putc('0' + (char)(n % 10));
}

/* 16進数出力ヘルパー */
static void uart_put_hex(uint64_t n) {
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

/*
 * exit_reason_name --- exit reason を人間が読める名前に変換
 */
static const char *exit_reason_name(uint32_t reason) {
    switch (reason) {
    case VMEXIT_REASON_CPUID:         return "CPUID";
    case VMEXIT_REASON_HLT:           return "HLT";
    case VMEXIT_REASON_IO:            return "I/O";
    case VMEXIT_REASON_EPT_VIOLATION: return "EPT_VIOLATION";
    default:                          return "UNKNOWN";
    }
}

/* vmread ラッパー */
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

/* vmwrite ラッパー */
static inline void vmwrite_field(uint64_t field, uint64_t value) {
    __asm__ volatile (
        "vmwrite %1, %0\n"
        : : "r"(field), "r"(value) : "cc"
    );
}

/*
 * handle_cpuid_exit --- CPUID VM exit の処理
 *
 * ゲストが CPUID を実行すると VM exit が発生する。
 * ハイパーバイザは「見せたい」CPU 情報を返す。
 * これにより、ゲストに対して CPU 機能をフィルタリングできる。
 *
 * 例: ゲストに VMX 機能を見せないようにする
 *     → 入れ子仮想化（nested virtualization）の制御
 */
static int handle_cpuid_exit(void) {
    uart_puts("  Handling CPUID exit\n");
    uart_puts("  -> returning filtered CPUID result to guest\n");

    /* ゲスト RIP を CPUID 命令の次に進める（CPUID は 2 バイト: 0F A2） */
    uint64_t rip = vmread_field(VMCS_GUEST_RIP);
    uint64_t len = vmread_field(VMCS_EXIT_INSTR_LENGTH);
    vmwrite_field(VMCS_GUEST_RIP, rip + len);

    return 0; /* ゲストを再開 */
}

/*
 * handle_hlt_exit --- HLT VM exit の処理
 *
 * ゲストが HLT を実行した。ハイパーバイザの判断:
 *   - ゲストを停止する（v29 と同様）
 *   - または、割り込みを注入してゲストを再開する
 *
 * v30 ではゲストを停止する。
 */
static int handle_hlt_exit(void) {
    uart_puts("  Handling HLT exit\n");
    uart_puts("  -> guest requested halt, stopping guest\n");
    return 1; /* 停止 */
}

/*
 * handle_io_exit --- I/O VM exit の処理
 *
 * ゲストが IN/OUT 命令で I/O ポートにアクセスした。
 * ハイパーバイザは仮想デバイスをエミュレートする。
 *
 * exit qualification のビット:
 *   bit 3:    方向（0=OUT, 1=IN）
 *   bit 15:0: ポート番号（bit 16 以降に格納）
 */
static int handle_io_exit(void) {
    uint64_t qualification = vmread_field(VMCS_EXIT_QUALIFICATION);
    uint16_t port = (uint16_t)((qualification >> 16) & 0xFFFF);
    int is_in = (qualification >> 3) & 1;

    uart_puts("  Handling I/O exit: port=");
    uart_put_hex(port);
    uart_puts(is_in ? " (IN)\n" : " (OUT)\n");

    /*
     * ポート 0x3F8 = COM1 (UART)
     * ゲストの UART 出力をホストにパススルーする例。
     */
    if (port == 0x3F8) {
        uart_puts("  -> UART I/O (COM1), pass-through\n");
    }

    /* ゲスト RIP を次の命令に進める */
    uint64_t rip = vmread_field(VMCS_GUEST_RIP);
    uint64_t len = vmread_field(VMCS_EXIT_INSTR_LENGTH);
    vmwrite_field(VMCS_GUEST_RIP, rip + len);

    return 0; /* ゲストを再開 */
}

int vmexit_handler(void) {
    /* exit reason を読み取る */
    uint64_t reason_full = vmread_field(VMCS_EXIT_REASON);
    uint32_t reason = (uint32_t)(reason_full & 0xFFFF); /* 下位 16 ビットが reason */

    uart_puts("VM exit: reason=");
    uart_put_dec(reason);
    uart_puts(" (");
    uart_puts(exit_reason_name(reason));
    uart_puts(")\n");

    /* exit qualification（追加情報） */
    uint64_t qual = vmread_field(VMCS_EXIT_QUALIFICATION);
    uart_puts("  qualification=");
    uart_put_hex(qual);
    uart_puts("\n");

    /* reason に応じたハンドリング */
    switch (reason) {
    case VMEXIT_REASON_CPUID:
        return handle_cpuid_exit();
    case VMEXIT_REASON_HLT:
        return handle_hlt_exit();
    case VMEXIT_REASON_IO:
        return handle_io_exit();
    default:
        uart_puts("  Unhandled exit reason: ");
        uart_put_dec(reason);
        uart_puts("\n");
        return 1; /* 停止 */
    }
}

void vmexit_demo(void) {
    uart_puts("--- VM Exit Handling Demo ---\n");

    uart_puts("Exit reason 10 (CPUID):\n");
    uart_puts("  Guest executes CPUID instruction\n");
    uart_puts("  -> VM exit to hypervisor\n");
    uart_puts("  -> Hypervisor returns filtered CPU info\n");
    uart_puts("  -> vmresume to guest (RIP advanced past CPUID)\n");
    uart_puts("\n");

    uart_puts("Exit reason 12 (HLT):\n");
    uart_puts("  Guest executes HLT instruction\n");
    uart_puts("  -> VM exit to hypervisor\n");
    uart_puts("  -> Hypervisor decides: stop guest or inject interrupt\n");
    uart_puts("  -> Guest stopped\n");
    uart_puts("\n");

    uart_puts("Exit reason 30 (I/O):\n");
    uart_puts("  Guest executes OUT 0x3F8, al (UART write)\n");
    uart_puts("  -> VM exit to hypervisor\n");
    uart_puts("  -> Hypervisor emulates virtual UART\n");
    uart_puts("  -> vmresume to guest (RIP advanced past OUT)\n");
    uart_puts("\n");

    uart_puts("VM exit demo complete\n");
}
