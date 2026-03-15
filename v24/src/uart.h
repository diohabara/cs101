/*
 * uart.h --- UART 16550A ドライバ（ヘッダ）
 *
 * UART (Universal Asynchronous Receiver/Transmitter) はシリアル通信デバイス。
 * PC の COM1 ポートは I/O ポート 0x3F8 にマッピングされている。
 *
 * ベアメタル環境で最初に動かすデバイスがシリアルポート。
 * 画面（VGA）より設定が簡単で、QEMU が -serial stdio で自動的にホストの端末に接続する。
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/* COM1 の I/O ポートベースアドレス */
#define UART_COM1 0x3F8

/*
 * UART レジスタオフセット（ベースアドレスからの相対）
 *
 *   オフセット  DLAB=0 読み      DLAB=0 書き      DLAB=1
 *   ---------  --------------  --------------  ----------
 *   +0         受信バッファ     送信バッファ     ボーレート下位
 *   +1         割り込み有効     割り込み有効     ボーレート上位
 *   +2         割り込みID       FIFO制御         -
 *   +3         ラインコントロール                 -
 *   +4         モデムコントロール                 -
 *   +5         ラインステータス                   -
 */
#define UART_THR 0  /* Transmit Holding Register（送信バッファ） */
#define UART_IER 1  /* Interrupt Enable Register */
#define UART_FCR 2  /* FIFO Control Register */
#define UART_LCR 3  /* Line Control Register */
#define UART_MCR 4  /* Modem Control Register */
#define UART_LSR 5  /* Line Status Register */

/* Line Status Register のビット */
#define UART_LSR_THRE 0x20  /* Transmit Holding Register Empty */

/* x86 の I/O ポートアクセス（in/out 命令） */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* UART 初期化 */
void uart_init(void);

/* 1 バイト送信（送信バッファが空くまでビジーウェイト） */
void uart_putc(char c);

/* 文字列送信 */
void uart_puts(const char *s);

#endif /* UART_H */
