/*
 * idt.h --- IDT (Interrupt Descriptor Table) 定義
 *
 * IDT は割り込みや例外が発生したときに CPU がどのハンドラを呼ぶかを定義するテーブル。
 * x86-64 では各エントリが 16 バイトで、最大 256 エントリ。
 *
 * IDT エントリ構造（16 バイト）:
 *   offset_low  (16ビット): ハンドラアドレスの下位
 *   selector    (16ビット): コードセグメントセレクタ（通常 0x08）
 *   ist         (3ビット):  Interrupt Stack Table インデックス
 *   type_attr   (8ビット):  ゲートタイプ + DPL + Present
 *   offset_mid  (16ビット): ハンドラアドレスの中位
 *   offset_high (32ビット): ハンドラアドレスの上位
 *   reserved    (32ビット): 予約（ゼロ）
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT エントリ（16 バイト） */
struct idt_entry {
    uint16_t offset_low;    /* ハンドラアドレス [15:0] */
    uint16_t selector;      /* コードセグメントセレクタ */
    uint8_t  ist;           /* IST オフセット（下位 3 ビット） */
    uint8_t  type_attr;     /* タイプ + DPL + Present */
    uint16_t offset_mid;    /* ハンドラアドレス [31:16] */
    uint32_t offset_high;   /* ハンドラアドレス [63:32] */
    uint32_t reserved;      /* 予約（ゼロ） */
} __attribute__((packed));

/* IDT ポインタ --- lidt 命令に渡す構造体 */
struct idt_ptr {
    uint16_t limit;  /* IDT のバイトサイズ - 1 */
    uint64_t base;   /* IDT の先頭アドレス */
} __attribute__((packed));

/* IDT を初期化し、lidt でロードする */
void idt_init(void);

/* IDT エントリを設定する（外部からハンドラを登録するため） */
void idt_set_entry(int vector, uint64_t handler, uint16_t selector, uint8_t type_attr);

#endif /* IDT_H */
