/*
 * vmx_detect.c --- VT-x (VMX) の検出と有効化
 *
 * VMX の有効化は以下のステップで行う:
 *
 *   1. CPUID 検出:
 *      CPUID leaf=1 の ECX[5] が 1 なら CPU が VMX に対応している。
 *
 *   2. IA32_FEATURE_CONTROL MSR 確認:
 *      BIOS が VMX を許可しているか確認する。
 *      ロックビットが 0 なら自分で設定してロックする。
 *      ロック済みで VMXON ビットが 0 なら BIOS が VMX を禁止している。
 *
 *   3. CR4.VMXE 設定:
 *      CR4 レジスタのビット 13 を 1 にして VMX 命令を有効化する。
 *
 *   4. VMXON 領域準備:
 *      4KB アラインメントされた領域の先頭に VMCS リビジョン ID を書き込む。
 *      リビジョン ID は IA32_VMX_BASIC MSR (0x480) から取得する。
 *
 *   5. VMXON 実行:
 *      vmxon 命令で VMX ルート操作モードに入る。
 *      成功すると以後 VMCS 操作（vmclear, vmptrld, vmwrite 等）が可能になる。
 */

#include "vmx.h"
#include "uart.h"

/* --- CPU 命令のインラインアセンブリラッパー --- */

/*
 * cpuid --- CPUID 命令の実行
 *
 * leaf: EAX に設定する機能番号
 * 戻り値: EAX, EBX, ECX, EDX の各レジスタ値
 */
static inline void cpuid(uint32_t leaf,
                          uint32_t *eax, uint32_t *ebx,
                          uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile ("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

/*
 * rdmsr --- MSR (Model Specific Register) の読み取り
 *
 * MSR は CPU モデル固有のレジスタで、VMX 制御パラメータを含む。
 * rdmsr 命令は ECX にアドレスを取り、EDX:EAX に 64 ビット値を返す。
 */
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

/*
 * wrmsr --- MSR への書き込み
 *
 * ECX にアドレス、EDX:EAX に 64 ビット値を設定して書き込む。
 */
static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile ("wrmsr"
        : : "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

/*
 * read_cr4 / write_cr4 --- CR4 レジスタの読み書き
 *
 * CR4 は CPU の動作モードを制御するレジスタ。
 * ビット 13 (VMXE) を 1 にすると VMX 命令が有効になる。
 */
static inline uint64_t read_cr4(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(val));
    return val;
}

static inline void write_cr4(uint64_t val) {
    __asm__ volatile ("mov %0, %%cr4" : : "r"(val));
}

int vmx_detect(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    /*
     * CPUID.1:ECX のビット 5 が VMX 対応フラグ。
     *
     * 例: ECX = 0x00000020 → ビット 5 = 1 → VMX 対応
     *     ECX = 0x00000000 → ビット 5 = 0 → VMX 非対応
     */
    return (ecx & CPUID_VMX_BIT) != 0;
}

/* VMXON 領域（4KB アラインメント、BSS セクションに配置） */
static struct vmxon_region vmxon_region __attribute__((aligned(4096)));

int vmx_enable(void) {
    /* ステップ 1: CPUID で VMX 対応を確認 */
    if (!vmx_detect()) {
        uart_puts("VT-x not supported by CPU\n");
        return -1;
    }
    uart_puts("VT-x supported\n");

    /* ステップ 2: IA32_FEATURE_CONTROL MSR を確認 */
    uint64_t feat = rdmsr(MSR_IA32_FEATURE_CONTROL);
    if (!(feat & FEATURE_CONTROL_LOCKED)) {
        /*
         * MSR がロックされていない場合:
         * VMXON 有効ビットとロックビットを設定する。
         * ロック後は再起動まで変更不可。
         */
        feat |= FEATURE_CONTROL_VMXON | FEATURE_CONTROL_LOCKED;
        wrmsr(MSR_IA32_FEATURE_CONTROL, feat);
    } else if (!(feat & FEATURE_CONTROL_VMXON)) {
        /* ロック済みだが VMXON が許可されていない → BIOS が禁止 */
        uart_puts("VMX disabled by BIOS\n");
        return -1;
    }

    /* ステップ 3: CR4.VMXE を設定して VMX 命令を有効化 */
    write_cr4(read_cr4() | CR4_VMXE);

    /* ステップ 4: VMXON 領域に VMCS リビジョン ID を書き込む */
    uint64_t vmx_basic = rdmsr(0x480); /* IA32_VMX_BASIC MSR */
    vmxon_region.revision_id = (uint32_t)(vmx_basic & 0x7FFFFFFF);

    /*
     * ステップ 5: VMXON 命令を実行
     *
     * vmxon はメモリオペランドとして VMXON 領域の物理アドレスを取る。
     * 成功すると CPU は VMX ルート操作モードに入る。
     * CF=1 なら失敗。
     */
    uint64_t phys = (uint64_t)&vmxon_region;
    uint8_t error;
    __asm__ volatile (
        "vmxon %1\n"
        "setc %0\n"   /* CF=1 on failure */
        : "=rm"(error)
        : "m"(phys)
        : "cc", "memory"
    );

    if (error) {
        uart_puts("VMXON failed\n");
        return -1;
    }

    uart_puts("VMX mode entered\n");
    return 0;
}

void vmx_disable(void) {
    /*
     * vmxoff 命令で VMX ルート操作モードから出る。
     * その後 CR4.VMXE をクリアして VMX 命令を無効化する。
     */
    __asm__ volatile ("vmxoff" ::: "cc");
    write_cr4(read_cr4() & ~CR4_VMXE);
    uart_puts("VMX mode exited\n");
}
