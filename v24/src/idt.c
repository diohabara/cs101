/*
 * idt.c --- IDT の初期化とロード
 *
 * 256 エントリの IDT を構築し、lidt 命令で CPU にロードする。
 * 初期状態ではすべてのベクタにデフォルトハンドラ（hlt ループ）を設定する。
 *
 * PIC (Programmable Interrupt Controller) の再マッピングも行い、
 * IRQ 0-15 をベクタ 32-47 に配置する（デフォルトの 0-15 は CPU 例外と衝突するため）。
 */

#include "idt.h"
#include "uart.h"

static struct idt_entry idt[256];
static struct idt_ptr idtr;

/*
 * デフォルト割り込みハンドラ
 * 未設定のベクタに対して呼ばれる。hlt で停止する。
 */
__attribute__((interrupt))
static void default_handler(void *frame) {
    (void)frame;
    uart_puts("Unhandled interrupt\n");
    for (;;) __asm__ volatile ("hlt");
}

void idt_set_entry(int vector, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].selector    = selector;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].reserved    = 0;
}

/*
 * PIC 再マッピング
 *
 * 8259 PIC のデフォルトでは IRQ 0-7 がベクタ 0x08-0x0F に、
 * IRQ 8-15 がベクタ 0x70-0x77 にマッピングされている。
 * ベクタ 0x08 は CPU の #DF (Double Fault) と衝突するため、
 * IRQ 0-15 をベクタ 0x20-0x2F (32-47) に再マッピングする。
 */
static void pic_remap(void) {
    /* ICW1: 初期化開始 + ICW4 が必要 */
    outb(0x20, 0x11);  /* マスター PIC */
    outb(0xA0, 0x11);  /* スレーブ PIC */

    /* ICW2: ベクタオフセット */
    outb(0x21, 0x20);  /* マスター: IRQ 0-7 → ベクタ 32-39 */
    outb(0xA1, 0x28);  /* スレーブ: IRQ 8-15 → ベクタ 40-47 */

    /* ICW3: マスター/スレーブ接続 */
    outb(0x21, 0x04);  /* マスター: IRQ 2 にスレーブ接続 */
    outb(0xA1, 0x02);  /* スレーブ: カスケード ID = 2 */

    /* ICW4: 8086 モード */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    /* すべての IRQ をマスク（個別に有効化する） */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void idt_init(void) {
    /* PIC を再マッピング */
    pic_remap();

    /* すべてのベクタにデフォルトハンドラを設定 */
    for (int i = 0; i < 256; i++) {
        /*
         * type_attr = 0x8E:
         *   P=1 (Present)
         *   DPL=00 (Ring 0)
         *   Type=1110 (64-bit Interrupt Gate)
         */
        idt_set_entry(i, (uint64_t)default_handler, 0x08, 0x8E);
    }

    /* IDTR 設定 */
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    /* lidt でロード */
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
