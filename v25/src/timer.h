/*
 * timer.h --- PIT (Programmable Interval Timer) 定義
 *
 * PIT は一定間隔で IRQ 0 を発生させるタイマーデバイス。
 * OS のタスクスケジューリングやタイムキーピングの基盤。
 *
 * PIT の基本周波数は 1193182 Hz。
 * 分周値 (divisor) を設定して任意の割り込み頻度を得る:
 *   割り込み周波数 = 1193182 / divisor
 *   例: 100 Hz → divisor = 11932
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* PIT I/O ポート */
#define PIT_CHANNEL0 0x40  /* チャネル 0 データポート */
#define PIT_COMMAND  0x43  /* コマンドレジスタ */

/* PIT 基本周波数 */
#define PIT_FREQUENCY 1193182

/* 現在の tick カウントを取得 */
uint64_t timer_get_ticks(void);

/* PIT を初期化し、指定した Hz で IRQ 0 を発生させる */
void timer_init(uint32_t hz);

#endif /* TIMER_H */
