/*
 * idt.c --- IDT の初期化とロード
 *
 * 256 エントリの IDT を構築する。
 * 各エントリは割り込みベクタ番号に対応するハンドラを指す。
 *
 * 初期状態では全エントリをデフォルトハンドラ（何もしない）に設定し、
 * 個別の割り込みハンドラは timer.c 等から idt_set_entry() で上書きする。
 */

#include "idt.h"
#include "uart.h"

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtr;

/*
 * デフォルト割り込みハンドラ
 *
 * 未設定のベクタに対するフォールバック。
 * 何もせず iretq で復帰する。
 */
__attribute__((naked)) void isr_default_stub(void) {
    __asm__ volatile ("iretq");
}

void idt_set_entry(int vector, uint64_t handler, uint8_t type_attr) {
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].selector    = 0x08;  /* カーネルコードセレクタ */
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].reserved    = 0;
}

void idt_init(void) {
    /* 全エントリをデフォルトハンドラで初期化 */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_entry(i, (uint64_t)isr_default_stub, 0x8E);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    /* lidt 命令で IDT をロード */
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
