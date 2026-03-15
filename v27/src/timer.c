/*
 * timer.c --- PIT (Programmable Interval Timer) ドライバ
 *
 * 1. PIC を初期化して IRQ を適切なベクタにリマップ
 * 2. PIT チャンネル 0 を指定周波数に設定
 * 3. IRQ0 ハンドラでティックカウンタを更新し、10 ティックごとに UART に出力
 *
 * PIC (8259A) リマッピング:
 *   IRQ 0-7  → INT 32-39 (マスター PIC)
 *   IRQ 8-15 → INT 40-47 (スレーブ PIC)
 *
 * PIT レジスタ:
 *   0x40 : チャンネル 0 データポート
 *   0x43 : コマンドレジスタ
 */

#include "timer.h"
#include "uart.h"
#include "idt.h"

/* PIT の基本周波数 (Hz) */
#define PIT_FREQUENCY 1193182

/* PIT I/O ポート */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

/* PIC I/O ポート */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* EOI (End of Interrupt) コマンド */
#define PIC_EOI 0x20

/* グローバルティックカウンタ */
static volatile uint64_t ticks = 0;

/*
 * uart_put_dec --- 10進数を UART に出力
 *
 * printf がないベアメタル環境で数値を表示するためのヘルパー。
 * 再帰的に上位桁から出力する。
 */
static void uart_put_dec(uint64_t n) {
    if (n >= 10) {
        uart_put_dec(n / 10);
    }
    uart_putc('0' + (char)(n % 10));
}

/*
 * isr_timer_handler --- タイマー割り込みの C ハンドラ
 *
 * アセンブリスタブ (isr_timer_stub) から呼ばれる。
 * ティックカウンタをインクリメントし、10 ティックごとに UART に出力する。
 * 最後に PIC に EOI を送る。
 */
void isr_timer_handler(void) {
    ticks++;

    /* 10 ティックごとに進捗を出力 */
    if (ticks % 10 == 0) {
        uart_put_dec(ticks);
        uart_puts(" ticks\n");
    }

    /* PIC マスターに EOI (End of Interrupt) を送信 */
    outb(PIC1_COMMAND, PIC_EOI);
}

/*
 * isr_timer_stub --- タイマー割り込みのアセンブリスタブ
 *
 * 割り込み発生時に CPU が自動的にこのスタブを呼ぶ。
 * caller-saved レジスタを保存し、C ハンドラを呼び、復帰する。
 *
 * __attribute__((naked)): プロローグ/エピローグを生成しない。
 * 完全に inline asm で制御する。
 */
__attribute__((naked)) void isr_timer_stub(void) {
    __asm__ volatile (
        "push %%rax\n"
        "push %%rcx\n"
        "push %%rdx\n"
        "push %%rdi\n"
        "push %%rsi\n"
        "push %%r8\n"
        "push %%r9\n"
        "push %%r10\n"
        "push %%r11\n"
        "call isr_timer_handler\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rsi\n"
        "pop %%rdi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rax\n"
        "iretq\n"
        ::: "memory"
    );
}

/*
 * pic_init --- PIC (8259A) の初期化とリマッピング
 *
 * デフォルトでは IRQ 0-7 がベクタ 8-15 にマッピングされているが、
 * ベクタ 0-31 は CPU 例外に予約されているので衝突する。
 * IRQ 0-7 をベクタ 32-39 に、IRQ 8-15 をベクタ 40-47 にリマップする。
 *
 * ICW (Initialization Command Word) シーケンス:
 *   ICW1: 初期化開始（0x11 = ICW4 が必要）
 *   ICW2: ベクタオフセット
 *   ICW3: マスター/スレーブ接続
 *   ICW4: モード設定（0x01 = 8086 モード）
 */
static void pic_init(void) {
    /* ICW1: 初期化開始 */
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    /* ICW2: ベクタオフセット */
    outb(PIC1_DATA, 0x20);  /* マスター: IRQ 0-7 → INT 32-39 */
    outb(PIC2_DATA, 0x28);  /* スレーブ: IRQ 8-15 → INT 40-47 */

    /* ICW3: マスター/スレーブ接続 */
    outb(PIC1_DATA, 0x04);  /* マスター: IRQ2 にスレーブ接続 */
    outb(PIC2_DATA, 0x02);  /* スレーブ: カスケード ID = 2 */

    /* ICW4: 8086 モード */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* IRQ マスク: IRQ0 (タイマー) のみ有効、他は全てマスク */
    outb(PIC1_DATA, 0xFE);  /* マスター: bit 0 = 0 で IRQ0 有効 */
    outb(PIC2_DATA, 0xFF);  /* スレーブ: 全てマスク */
}

void timer_init(uint32_t freq_hz) {
    /* PIC を初期化 */
    pic_init();

    /* PIT 除数を計算 */
    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / freq_hz);

    /*
     * PIT コマンド: 0x36
     *   bit 7-6: 00 = チャンネル 0
     *   bit 5-4: 11 = 下位→上位の順にアクセス
     *   bit 3-1: 011 = モード 3 (方形波)
     *   bit 0:   0 = バイナリカウント
     */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));        /* 除数の下位バイト */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); /* 除数の上位バイト */

    /* IDT ベクタ 32 にタイマー ISR を登録 */
    idt_set_entry(32, (uint64_t)isr_timer_stub, 0x8E);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}
