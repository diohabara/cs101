/*
 * idt.h --- IDT (Interrupt Descriptor Table) 定義
 *
 * IDT は割り込みベクタごとのハンドラアドレスを格納するテーブル。
 * x86-64 では各エントリが 16 バイト。最大 256 エントリ。
 *
 * IDT エントリ構造（16 バイト）:
 *
 *   offset_low  : ハンドラアドレスのビット 0..15
 *   selector    : コードセグメントセレクタ（GDT のカーネルコード = 0x08）
 *   ist         : IST (Interrupt Stack Table) インデックス（通常 0）
 *   type_attr   : タイプと属性
 *                   0x8E = Present + DPL=0 + Interrupt Gate (64-bit)
 *                   0x8F = Present + DPL=0 + Trap Gate (64-bit)
 *   offset_mid  : ハンドラアドレスのビット 16..31
 *   offset_high : ハンドラアドレスのビット 32..63
 *   reserved    : 0
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT エントリ（16 バイト）--- packed で構造体パディングを禁止 */
struct idt_entry {
    uint16_t offset_low;   /* ハンドラアドレスの下位 16 ビット */
    uint16_t selector;     /* コードセグメントセレクタ */
    uint8_t  ist;          /* IST インデックス (0 = 使わない) */
    uint8_t  type_attr;    /* タイプと属性 */
    uint16_t offset_mid;   /* ハンドラアドレスのビット 16..31 */
    uint32_t offset_high;  /* ハンドラアドレスのビット 32..63 */
    uint32_t reserved;     /* 予約（0） */
} __attribute__((packed));

/* IDT ポインタ --- lidt 命令に渡す構造体 */
struct idt_ptr {
    uint16_t limit;  /* IDT のバイトサイズ - 1 */
    uint64_t base;   /* IDT の先頭アドレス */
} __attribute__((packed));

/* IDT にエントリを設定する */
void idt_set_entry(int vector, uint64_t handler, uint8_t type_attr);

/* IDT を初期化し、lidt でロードする */
void idt_init(void);

#endif /* IDT_H */
