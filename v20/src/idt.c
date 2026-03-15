/*
 * idt.c — Interrupt Descriptor Table（実装）
 *
 * 256 エントリの IDT を構築し、lidt 命令で CPU にロードする。
 * CPU 例外 #DE (0), #BP (3), #PF (14) のハンドラを登録する。
 *
 * 割り込みが発生すると CPU は自動的に:
 *   1. 現在の RSP, RFLAGS, CS, RIP をスタックにプッシュ
 *   2. エラーコードがある例外（#PF 等）はさらにエラーコードをプッシュ
 *   3. IDT[ベクタ番号] からハンドラのアドレスを取得
 *   4. そのアドレスにジャンプ
 */

#include "idt.h"
#include "uart.h"

/* IDT 本体: 256 エントリ */
static struct idt_entry idt[256];

/* IDTR に渡す構造体 */
static struct idt_ptr idtr;

/*
 * IDT ゲートを設定するヘルパー関数
 *
 * 引数:
 *   vec       — ベクタ番号（0-255）
 *   handler   — ハンドラ関数のアドレス
 *   selector  — コードセグメントセレクタ（通常 0x08）
 *   type_attr — タイプ + DPL + Present
 *               0x8E = Present(1) + DPL=0(00) + 割り込みゲート(0xE)
 *               0xEE = Present(1) + DPL=3(11) + 割り込みゲート(0xE)
 */
static void idt_set_gate(uint8_t vec, uint64_t handler,
                         uint16_t selector, uint8_t type_attr) {
    idt[vec].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vec].selector    = selector;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vec].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vec].reserved    = 0;
}

/*
 * 例外ハンドラ: #DE — Divide Error（ベクタ 0）
 *
 * ゼロ除算（div/idiv 命令で除数が 0）のとき CPU が発生させる。
 * エラーコードなし。
 */
__attribute__((interrupt))
void exc_divide_error(struct interrupt_frame *frame) {
    (void)frame;
    uart_puts("Exception: #DE (Divide Error)\n");
    for (;;) __asm__ volatile ("hlt");
}

/*
 * 例外ハンドラ: #BP — Breakpoint（ベクタ 3）
 *
 * int3 命令で発生する。デバッガがブレークポイントに使う。
 * トラップ（fault ではない）なので、RIP は int3 の「次の命令」を指す。
 * 本来は処理後に実行を継続できるが、ここでは hlt で停止する。
 */
__attribute__((interrupt))
void exc_breakpoint(struct interrupt_frame *frame) {
    (void)frame;
    uart_puts("Exception: #BP (Breakpoint)\n");
    for (;;) __asm__ volatile ("hlt");
}

/*
 * 例外ハンドラ: #PF — Page Fault（ベクタ 14）
 *
 * 無効なメモリアクセスで発生する。
 * エラーコードあり: ビットフィールドで原因を示す。
 *   bit 0: Present（0=ページ不在, 1=保護違反）
 *   bit 1: Write（0=読み取り, 1=書き込み）
 *   bit 2: User（0=カーネルモード, 1=ユーザーモード）
 *
 * CR2 レジスタにフォルト発生アドレスが格納される。
 */
__attribute__((interrupt))
void exc_page_fault(struct interrupt_frame *frame, uint64_t error_code) {
    (void)frame;
    (void)error_code;
    uart_puts("Exception: #PF (Page Fault)\n");
    for (;;) __asm__ volatile ("hlt");
}

void idt_init(void) {
    /*
     * IDT を全エントリ 0 で初期化
     *
     * 未登録のベクタに割り込みが来ると、Present=0 のため
     * CPU が #GP (General Protection Fault) を発生させる。
     */
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].reserved    = 0;
    }

    /*
     * 例外ハンドラの登録
     *
     * 0x8E = Present(1) | DPL=0(00) | Interrupt Gate(0xE)
     *   - Present=1: このエントリは有効
     *   - DPL=0: Ring 0（カーネル）からのみ呼び出し可能
     *   - Interrupt Gate: 割り込み時に IF フラグをクリア（割り込み禁止）
     */
    idt_set_gate(0,  (uint64_t)exc_divide_error, 0x08, 0x8E);
    idt_set_gate(3,  (uint64_t)exc_breakpoint,   0x08, 0x8E);
    idt_set_gate(14, (uint64_t)exc_page_fault,   0x08, 0x8E);

    /* IDTR 設定 */
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt[0];

    /* lidt でロード */
    __asm__ volatile ("lidt (%0)" : : "r"(&idtr) : "memory");
}
