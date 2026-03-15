/*
 * uart.c — UART 16550A ドライバ（実装）
 *
 * シリアルポート COM1 (0x3F8) を初期化し、1 バイトずつ文字を送信する。
 * QEMU の -serial stdio オプションにより、送信した文字がホストの端末に表示される。
 *
 * 初期化手順:
 *   1. 割り込みを無効化（ポーリングモードで使う）
 *   2. DLAB=1 にしてボーレートを設定（115200bps）
 *   3. DLAB=0 に戻し、8N1（8ビット、パリティなし、1ストップビット）を設定
 *   4. FIFO を有効化
 *   5. DTR + RTS を有効化
 */

#include "uart.h"

void uart_init(void) {
    /* 割り込み無効化 */
    outb(UART_COM1 + UART_IER, 0x00);

    /* DLAB=1: ボーレートレジスタにアクセス可能にする */
    outb(UART_COM1 + UART_LCR, 0x80);

    /* ボーレート 115200 の除数 = 1 (115200 / 115200) */
    outb(UART_COM1 + 0, 0x01);  /* 除数の下位バイト */
    outb(UART_COM1 + 1, 0x00);  /* 除数の上位バイト */

    /* DLAB=0 + 8N1: 8ビットデータ、パリティなし、1ストップビット */
    outb(UART_COM1 + UART_LCR, 0x03);

    /* FIFO 有効化、14バイトトリガ */
    outb(UART_COM1 + UART_FCR, 0xC7);

    /* DTR + RTS + OUT2 有効化 */
    outb(UART_COM1 + UART_MCR, 0x0B);
}

void uart_putc(char c) {
    /* 送信バッファが空くまで待つ（ビジーウェイト / ポーリング） */
    while ((inb(UART_COM1 + UART_LSR) & UART_LSR_THRE) == 0) {
        /* spin */
    }

    /* 1 バイト送信 */
    outb(UART_COM1 + UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s);
        s++;
    }
}
