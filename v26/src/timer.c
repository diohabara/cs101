/*
 * timer.c --- PIT (Programmable Interval Timer) 実装
 *
 * PIT チャネル 0 を設定し、一定間隔で IRQ 0（ベクタ 32）を発生させる。
 * 割り込みハンドラで tick カウンタをインクリメントする。
 *
 * IRQ 0 は PIC 再マッピング後にベクタ 32 にマッピングされている。
 * ハンドラの最後に EOI (End Of Interrupt) を PIC に送信する必要がある。
 */

#include "timer.h"
#include "idt.h"
#include "uart.h"

static volatile uint64_t ticks = 0;

/*
 * タイマー割り込みハンドラ
 *
 * IRQ 0 (ベクタ 32) で呼ばれる。
 * tick カウンタをインクリメントし、PIC に EOI を送信する。
 */
__attribute__((interrupt))
static void timer_handler(void *frame) {
    (void)frame;
    ticks++;

    /* EOI (End Of Interrupt) をマスター PIC に送信 */
    outb(0x20, 0x20);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

void timer_init(uint32_t hz) {
    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / hz);

    /*
     * PIT コマンドバイト: 0x36
     *   bit 7-6: チャネル 0
     *   bit 5-4: アクセスモード = lo/hi byte
     *   bit 3-1: モード 3 (square wave)
     *   bit 0:   16ビットバイナリ
     */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));        /* 下位バイト */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); /* 上位バイト */

    /* IDT ベクタ 32 にタイマーハンドラを登録 */
    idt_set_entry(32, (uint64_t)timer_handler, 0x08, 0x8E);

    /* IRQ 0 を有効化（マスター PIC のビット 0 をクリア） */
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~0x01);
}
