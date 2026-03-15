/*
 * timer.h --- PIT (Programmable Interval Timer) ドライバ（ヘッダ）
 *
 * PIT は x86 PC の標準タイマーデバイス。チャンネル 0 が IRQ0 に接続され、
 * 設定した周波数で定期的に割り込みを発生させる。
 *
 * PIT の基本周波数は 1193182 Hz。除数を設定して目的の周波数を得る:
 *   除数 = 1193182 / 目的周波数
 *   例: 100 Hz → 除数 = 11932
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * timer_init --- PIT を指定周波数で初期化
 *
 * PIC (Programmable Interrupt Controller) を初期化して IRQ0-7 をベクタ 32-39 に
 * リマップし、PIT チャンネル 0 を指定周波数に設定する。
 *
 * freq_hz: タイマー割り込みの周波数（Hz）。例: 100 = 10ms ごとに割り込み
 */
void timer_init(uint32_t freq_hz);

/*
 * timer_get_ticks --- 起動からの累積ティック数を返す
 *
 * タイマー割り込みが発生するたびにインクリメントされるカウンタの値。
 * 100 Hz なら 100 ティック = 1 秒。
 */
uint64_t timer_get_ticks(void);

#endif /* TIMER_H */
