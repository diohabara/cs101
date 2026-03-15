/*
 * usermode.c --- ユーザーモード遷移（実装）
 *
 * syscall/sysret 機構の初期化を行う。
 * TSS の設定と syscall MSR の初期化が主な役割。
 *
 * v24 では概念実証として、Ring 0 から syscall 命令を呼び出し、
 * LSTAR に登録したハンドラが正しく動作することを確認する。
 */

#include "usermode.h"
#include "uart.h"

/* MSR (Model Specific Register) アドレス */
#define MSR_EFER   0xC0000080   /* Extended Feature Enable Register */
#define MSR_STAR   0xC0000081   /* SYSCALL CS/SS セレクタ */
#define MSR_LSTAR  0xC0000082   /* SYSCALL エントリポイントアドレス */
#define MSR_SFMASK 0xC0000084   /* SYSCALL 時に RFLAGS からマスクするビット */

/* EFER のビット */
#define EFER_SCE   0x01         /* System Call Extensions 有効化 */

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

/* syscall エントリポイント（外部: syscall_handler.c で定義） */
extern void syscall_entry(void);

/* TSS インスタンス */
static struct tss tss;

/* カーネルスタック（syscall/割り込みから戻るとき用） */
static uint8_t kernel_stack[4096] __attribute__((aligned(16)));

void usermode_init(void) {
    /* TSS の初期化 */
    for (int i = 0; i < (int)sizeof(tss); i++) {
        ((uint8_t *)&tss)[i] = 0;
    }
    tss.rsp0 = (uint64_t)&kernel_stack[sizeof(kernel_stack)];
    tss.iopb = sizeof(struct tss);  /* I/O ビットマップなし */

    /* EFER.SCE を有効化（syscall/sysret 命令を使えるようにする） */
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | EFER_SCE);

    /*
     * STAR MSR の設定
     *
     * ビット 47:32 = SYSCALL 時の CS セレクタ（カーネルコード = 0x08）
     *   → SS は自動的に CS + 8 = 0x10（カーネルデータ）になる
     *
     * ビット 63:48 = SYSRET 時のベースセレクタ（0x10）
     *   → CS = base + 16 = 0x20 (Ring 3 的には 0x18|3 = ユーザーコード)
     *   → SS = base + 8  = 0x18 (Ring 3 的には 0x20|3 = ユーザーデータ)
     *
     * 注: v24 では Ring 0 から syscall をテストするため、
     *     SYSRET のセレクタは使われない。
     */
    wrmsr(MSR_STAR, ((uint64_t)0x0010 << 48) | ((uint64_t)0x0008 << 32));

    /* LSTAR: syscall 命令のエントリポイント */
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* SFMASK: syscall 時に IF (Interrupt Flag) をマスク */
    wrmsr(MSR_SFMASK, 0x200);
}
