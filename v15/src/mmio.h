/*
 * mmio.h --- MMIO レジスタ定義
 *
 * Memory-Mapped I/O では、特定のメモリアドレスがデバイスのレジスタに対応する。
 * CPU から見ると通常のメモリアクセス（load/store）だが、
 * 実際にはデバイスへのコマンド送信やステータス読み出しになる。
 *
 * このヘッダでは仮想デバイスのレジスタレイアウトを定義する。
 * 実際のハードウェアでは、これらのオフセットはデバイスのデータシートに記載される。
 */

#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

/*
 * レジスタオフセット（バイト単位）
 *
 * デバイスの MMIO 領域先頭からのオフセットで各レジスタを識別する。
 * 各レジスタは 4 バイト（uint32_t）幅。
 *
 *   ベースアドレス + 0x00: COMMAND_REG  (Write) コマンドを書き込む
 *   ベースアドレス + 0x04: STATUS_REG   (Read)  デバイスの状態を読み出す
 *   ベースアドレス + 0x08: DATA_REG     (R/W)   データの読み書き
 */
#define MMIO_COMMAND_REG_OFFSET  0x00
#define MMIO_STATUS_REG_OFFSET   0x04
#define MMIO_DATA_REG_OFFSET     0x08

/* MMIO 領域の合計サイズ（バイト） */
#define MMIO_REGION_SIZE         0x0C

/*
 * コマンド値（COMMAND_REG に書き込む値）
 *
 * CPU がデバイスに対して「何をしてほしいか」を伝える。
 */
#define CMD_NOP   0x00  /* 何もしない */
#define CMD_READ  0x01  /* デバイスからデータを読み出す */
#define CMD_WRITE 0x02  /* デバイスにデータを書き込む */
#define CMD_RESET 0xFF  /* デバイスをリセットする */

/*
 * ステータス値（STATUS_REG から読み出す値）
 *
 * デバイスが「今どういう状態か」を CPU に伝える。
 */
#define STATUS_IDLE  0x00  /* アイドル状態（コマンド待ち） */
#define STATUS_BUSY  0x01  /* コマンド実行中 */
#define STATUS_DONE  0x02  /* コマンド完了（データ準備完了） */
#define STATUS_ERROR 0xFF  /* エラー発生 */

/*
 * LED レジスタオフセット
 *
 * LED コントローラ用の MMIO 領域レイアウト。
 * 各ビットが 1 つの LED に対応する（ビット 0 = LED 0, ビット 1 = LED 1, ...）。
 */
#define LED_CONTROL_REG_OFFSET   0x00  /* LED パターンを書き込む */
#define LED_STATUS_REG_OFFSET    0x04  /* 現在の LED 状態を読み出す */
#define LED_REGION_SIZE          0x08

/* LED 数 */
#define LED_COUNT 4

/* LED ビットマスク */
#define LED0 (1U << 0)
#define LED1 (1U << 1)
#define LED2 (1U << 2)
#define LED3 (1U << 3)
#define LED_ALL (LED0 | LED1 | LED2 | LED3)  /* 0xF */

#endif /* MMIO_H */
