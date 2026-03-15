/*
 * vmcs.c --- VMCS (Virtual Machine Control Structure) の初期化
 *
 * VMCS はゲスト VM の CPU 状態と制御パラメータを格納する構造体。
 * vmclear → vmptrld → vmwrite の順で初期化する。
 *
 * 初期化手順:
 *   1. vmclear: VMCS 領域をクリアして初期状態にする
 *   2. vmptrld: この VMCS を「現在の VMCS」として CPU に登録する
 *   3. vmwrite: ゲスト状態、ホスト状態、制御フィールドを設定する
 *
 * VMX が利用できない環境では、設定内容をシミュレーション出力する。
 */

#include "vmcs.h"
#include "vmx.h"
#include "uart.h"

/* VMCS 領域（4KB アラインメント） */
static struct vmcs_region vmcs __attribute__((aligned(4096)));

/* --- VMX 命令のインラインアセンブリラッパー --- */

static inline uint64_t rdmsr_local(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

/*
 * vmclear --- VMCS をクリアする
 *
 * VMCS のデータを初期化し、launch 状態を "clear" にする。
 * 新しい VMCS を使う前に必ず実行する。
 */
static inline int vmclear(uint64_t *phys) {
    uint8_t error;
    __asm__ volatile (
        "vmclear %1\n"
        "setc %0\n"
        : "=rm"(error)
        : "m"(*phys)
        : "cc", "memory"
    );
    return error ? -1 : 0;
}

/*
 * vmptrld --- VMCS を現在の VMCS として登録する
 *
 * 以後の vmread/vmwrite はこの VMCS に対して操作される。
 * 一度に1つの VMCS のみがアクティブ。
 */
static inline int vmptrld(uint64_t *phys) {
    uint8_t error;
    __asm__ volatile (
        "vmptrld %1\n"
        "setc %0\n"
        : "=rm"(error)
        : "m"(*phys)
        : "cc", "memory"
    );
    return error ? -1 : 0;
}

/*
 * vmwrite_field --- VMCS フィールドに値を書き込む
 *
 * field: VMCS フィールドのエンコーディング番号
 * value: 設定する値
 */
static inline int vmwrite_field(uint64_t field, uint64_t value) {
    uint8_t error;
    __asm__ volatile (
        "vmwrite %2, %1\n"
        "setc %0\n"
        : "=rm"(error)
        : "r"(field), "r"(value)
        : "cc"
    );
    return error ? -1 : 0;
}

/* 10進数出力ヘルパー */
static void uart_put_hex(uint64_t n) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    /* 先頭のゼロをスキップ */
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
 * vmcs_simulate --- VMX 非対応環境でのシミュレーション出力
 *
 * 実際の VMCS 操作はできないが、設定する内容を教育目的で出力する。
 */
static void vmcs_simulate(void) {
    uart_puts("VMCS: simulated (no VT-x)\n");
    uart_puts("  [sim] vmclear: VMCS region cleared\n");
    uart_puts("  [sim] vmptrld: VMCS set as current\n");
    uart_puts("  [sim] Guest state:\n");
    uart_puts("    Guest RIP = 0x0 (guest entry point)\n");
    uart_puts("    Guest RSP = 0x0 (guest stack)\n");
    uart_puts("    Guest CR0 = 0x20 (NE bit)\n");
    uart_puts("    Guest CR4 = 0x2000 (VMXE bit)\n");
    uart_puts("    Guest CS  = 0x08 (kernel code selector)\n");
    uart_puts("  [sim] Host state:\n");
    uart_puts("    Host RIP  = VM exit handler address\n");
    uart_puts("    Host RSP  = host stack pointer\n");
    uart_puts("    Host CR0  = current CR0\n");
    uart_puts("    Host CR4  = current CR4 (with VMXE)\n");
    uart_puts("    Host CS   = 0x08 (kernel code selector)\n");
    uart_puts("  [sim] VM execution controls configured\n");
    uart_puts("VMCS initialized (simulated)\n");
}

/* CR0/CR3 の読み取り */
static inline uint64_t read_cr0(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr3(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr4(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(val));
    return val;
}

/* ゲストスタック（4KB） */
static uint8_t guest_stack[4096] __attribute__((aligned(16)));

/*
 * vmexit_stub --- VM exit ハンドラのスタブ
 *
 * VM exit が発生すると CPU はホスト RIP に設定されたアドレスにジャンプする。
 * ここでは単に停止する。v29 以降で本格的なハンドリングを実装する。
 */
void __attribute__((noreturn)) vmexit_stub(void) {
    uart_puts("VM exit occurred\n");
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}

int vmcs_init(void) {
    if (!vmx_detect()) {
        vmcs_simulate();
        return -1;
    }

    /* VMX を有効化（v27 の vmx_enable と同等） */
    if (vmx_enable() != 0) {
        vmcs_simulate();
        return -1;
    }

    /* VMCS リビジョン ID を設定 */
    uint64_t vmx_basic = rdmsr_local(0x480);
    vmcs.revision_id = (uint32_t)(vmx_basic & 0x7FFFFFFF);

    /* ステップ 1: vmclear */
    uint64_t vmcs_phys = (uint64_t)&vmcs;
    if (vmclear(&vmcs_phys) != 0) {
        uart_puts("vmclear failed\n");
        vmx_disable();
        return -1;
    }
    uart_puts("vmclear: VMCS cleared\n");

    /* ステップ 2: vmptrld */
    if (vmptrld(&vmcs_phys) != 0) {
        uart_puts("vmptrld failed\n");
        vmx_disable();
        return -1;
    }
    uart_puts("vmptrld: VMCS set as current\n");

    /* ステップ 3: vmwrite でフィールドを設定 */

    /* --- ゲスト状態 --- */
    vmwrite_field(VMCS_GUEST_CR0, read_cr0());
    vmwrite_field(VMCS_GUEST_CR3, 0);
    vmwrite_field(VMCS_GUEST_CR4, read_cr4());
    vmwrite_field(VMCS_GUEST_DR7, 0x400);
    vmwrite_field(VMCS_GUEST_RSP, (uint64_t)&guest_stack[4096]);
    vmwrite_field(VMCS_GUEST_RIP, 0); /* v29 でゲストコードのアドレスを設定 */
    vmwrite_field(VMCS_GUEST_RFLAGS, 0x02); /* bit 1 は常に 1 */

    /* ゲストセグメントセレクタ */
    vmwrite_field(VMCS_GUEST_CS_SELECTOR, 0x08);
    vmwrite_field(VMCS_GUEST_DS_SELECTOR, 0x10);
    vmwrite_field(VMCS_GUEST_ES_SELECTOR, 0x10);
    vmwrite_field(VMCS_GUEST_FS_SELECTOR, 0x10);
    vmwrite_field(VMCS_GUEST_GS_SELECTOR, 0x10);
    vmwrite_field(VMCS_GUEST_SS_SELECTOR, 0x10);
    vmwrite_field(VMCS_GUEST_TR_SELECTOR, 0);
    vmwrite_field(VMCS_GUEST_LDTR_SELECTOR, 0);

    /* ゲストセグメントリミット */
    vmwrite_field(VMCS_GUEST_CS_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_DS_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_ES_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_FS_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_GS_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_SS_LIMIT, 0xFFFFFFFF);
    vmwrite_field(VMCS_GUEST_TR_LIMIT, 0xFF);
    vmwrite_field(VMCS_GUEST_LDTR_LIMIT, 0);
    vmwrite_field(VMCS_GUEST_GDTR_LIMIT, 0);
    vmwrite_field(VMCS_GUEST_IDTR_LIMIT, 0);

    /* ゲストセグメントアクセス権 */
    vmwrite_field(VMCS_GUEST_CS_ACCESS, 0xA09B);
    vmwrite_field(VMCS_GUEST_DS_ACCESS, 0xC093);
    vmwrite_field(VMCS_GUEST_ES_ACCESS, 0xC093);
    vmwrite_field(VMCS_GUEST_FS_ACCESS, 0xC093);
    vmwrite_field(VMCS_GUEST_GS_ACCESS, 0xC093);
    vmwrite_field(VMCS_GUEST_SS_ACCESS, 0xC093);
    vmwrite_field(VMCS_GUEST_TR_ACCESS, 0x008B);
    vmwrite_field(VMCS_GUEST_LDTR_ACCESS, 0x10000); /* unusable */

    /* ゲストセグメントベース */
    vmwrite_field(VMCS_GUEST_CS_BASE, 0);
    vmwrite_field(VMCS_GUEST_DS_BASE, 0);
    vmwrite_field(VMCS_GUEST_ES_BASE, 0);
    vmwrite_field(VMCS_GUEST_FS_BASE, 0);
    vmwrite_field(VMCS_GUEST_GS_BASE, 0);
    vmwrite_field(VMCS_GUEST_SS_BASE, 0);
    vmwrite_field(VMCS_GUEST_TR_BASE, 0);
    vmwrite_field(VMCS_GUEST_LDTR_BASE, 0);
    vmwrite_field(VMCS_GUEST_GDTR_BASE, 0);
    vmwrite_field(VMCS_GUEST_IDTR_BASE, 0);

    /* VMCS link pointer (-1 = 無効) */
    vmwrite_field(VMCS_GUEST_VMCS_LINK, 0xFFFFFFFFFFFFFFFFULL);

    /* ゲストアクティビティ状態: 0 = active */
    vmwrite_field(VMCS_GUEST_ACTIVITY_STATE, 0);
    vmwrite_field(VMCS_GUEST_INTERRUPTIBILITY, 0);

    uart_puts("Guest state configured\n");

    /* --- ホスト状態 --- */
    vmwrite_field(VMCS_HOST_CR0, read_cr0());
    vmwrite_field(VMCS_HOST_CR3, read_cr3());
    vmwrite_field(VMCS_HOST_CR4, read_cr4());
    vmwrite_field(VMCS_HOST_RSP, (uint64_t)&guest_stack[4096]); /* 仮のスタック */
    vmwrite_field(VMCS_HOST_RIP, (uint64_t)vmexit_stub);

    vmwrite_field(VMCS_HOST_CS_SELECTOR, 0x08);
    vmwrite_field(VMCS_HOST_DS_SELECTOR, 0x10);
    vmwrite_field(VMCS_HOST_ES_SELECTOR, 0x10);
    vmwrite_field(VMCS_HOST_FS_SELECTOR, 0x10);
    vmwrite_field(VMCS_HOST_GS_SELECTOR, 0x10);
    vmwrite_field(VMCS_HOST_SS_SELECTOR, 0x10);
    vmwrite_field(VMCS_HOST_TR_SELECTOR, 0);

    vmwrite_field(VMCS_HOST_IA32_SYSENTER_CS, 0);
    vmwrite_field(VMCS_HOST_IA32_SYSENTER_ESP, 0);
    vmwrite_field(VMCS_HOST_IA32_SYSENTER_EIP, 0);

    vmwrite_field(VMCS_HOST_GDTR_BASE, 0);
    vmwrite_field(VMCS_HOST_IDTR_BASE, 0);
    vmwrite_field(VMCS_HOST_FS_BASE, 0);
    vmwrite_field(VMCS_HOST_GS_BASE, 0);
    vmwrite_field(VMCS_HOST_TR_BASE, 0);

    uart_puts("Host state configured\n");

    /* --- VM 実行制御 --- */

    /*
     * Pin-based controls: 外部割り込みや NMI で VM exit するか
     * Proc-based controls: HLT, I/O, MSR アクセスで VM exit するか
     *
     * MSR から許可されるビットの範囲を取得して、最小限のビットを設定する。
     */
    uint64_t pin_msr = rdmsr_local(0x481); /* IA32_VMX_PINBASED_CTLS */
    uint32_t pin_ctls = (uint32_t)pin_msr; /* allowed-0 bits (must be 1) */
    vmwrite_field(VMCS_CTRL_PIN_BASED, pin_ctls);

    uint64_t proc_msr = rdmsr_local(0x482); /* IA32_VMX_PROCBASED_CTLS */
    uint32_t proc_ctls = (uint32_t)proc_msr; /* allowed-0 bits */
    /* HLT exit を有効化 (bit 7) */
    proc_ctls |= (1 << 7);
    vmwrite_field(VMCS_CTRL_PROC_BASED, proc_ctls);

    uint64_t exit_msr = rdmsr_local(0x483); /* IA32_VMX_EXIT_CTLS */
    uint32_t exit_ctls = (uint32_t)exit_msr;
    /* Host address-space size (bit 9) --- 64bit ホスト */
    exit_ctls |= (1 << 9);
    vmwrite_field(VMCS_CTRL_EXIT, exit_ctls);

    uint64_t entry_msr = rdmsr_local(0x484); /* IA32_VMX_ENTRY_CTLS */
    uint32_t entry_ctls = (uint32_t)entry_msr;
    /* IA-32e mode guest (bit 9) --- 64bit ゲスト */
    entry_ctls |= (1 << 9);
    vmwrite_field(VMCS_CTRL_ENTRY, entry_ctls);

    vmwrite_field(VMCS_CTRL_EXCEPTION_BITMAP, 0);
    vmwrite_field(VMCS_CTRL_ENTRY_MSR_LOAD_COUNT, 0);
    vmwrite_field(VMCS_CTRL_EXIT_MSR_LOAD_COUNT, 0);
    vmwrite_field(VMCS_CTRL_EXIT_MSR_STORE_COUNT, 0);

    uart_puts("VM execution controls configured\n");

    uart_puts("VMCS initialized\n");
    return 0;
}

void vmcs_dump(void) {
    if (!vmx_detect()) {
        uart_puts("VMCS dump: skipped (no VT-x)\n");
        uart_puts("  Guest RIP = (not available)\n");
        uart_puts("  Guest RSP = (not available)\n");
        uart_puts("  Host RIP  = (not available)\n");
        return;
    }

    uart_puts("VMCS dump:\n");
    uart_puts("  VMCS address = ");
    uart_put_hex((uint64_t)&vmcs);
    uart_puts("\n");
    uart_puts("  Revision ID  = ");
    uart_put_hex(vmcs.revision_id);
    uart_puts("\n");
}
