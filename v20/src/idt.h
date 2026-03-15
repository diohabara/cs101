/*
 * idt.h — Interrupt Descriptor Table（ヘッダ）
 *
 * IDT は CPU に「割り込みが起きたらどの関数を呼ぶか」を教えるテーブル。
 * 256 エントリで、各エントリがハンドラ関数のアドレスと属性を持つ。
 *
 * 割り込みの種類:
 *   0-31:   CPU 例外（ゼロ除算、ページフォルト等）。CPU が自動的に発生させる。
 *   32-255: 外部割り込み（IRQ）やソフトウェア割り込み。
 *
 * 64ビットモードでは各エントリが 16 バイト（32ビットモードの 8 バイトから拡張）。
 * ハンドラのアドレスを 64 ビットで格納するため。
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/*
 * IDT ゲートディスクリプタ（16バイト、64ビットモード）
 *
 *   ビット     フィールド       説明
 *   ─────     ────────         ────
 *   0-15      offset_low       ハンドラアドレス下位16ビット
 *   16-31     selector         コードセグメントセレクタ（0x08 = Kernel Code）
 *   32-34     ist              Interrupt Stack Table インデックス（通常 0）
 *   35-39     reserved         予約（0）
 *   40-43     type             ゲートタイプ（0xE = 64ビット割り込みゲート）
 *   44        zero             0
 *   45-46     dpl              Descriptor Privilege Level
 *   47        present          1 = 有効
 *   48-63     offset_mid       ハンドラアドレス中位16ビット
 *   64-95     offset_high      ハンドラアドレス上位32ビット
 *   96-127    reserved         予約（0）
 */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;     /* コードセグメントセレクタ（0x08） */
    uint8_t  ist;          /* Interrupt Stack Table（通常 0） */
    uint8_t  type_attr;    /* タイプ + DPL + Present */
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

/*
 * IDTR（IDT レジスタ）に渡す構造体
 *
 * lidt 命令はこの構造体のアドレスを受け取る。
 *   limit: IDT のサイズ - 1（バイト単位）
 *   base:  IDT の先頭アドレス
 */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/*
 * CPU 例外フレーム
 *
 * CPU が割り込みハンドラを呼ぶとき、自動的にスタックにプッシュする情報。
 * __attribute__((interrupt)) を使うハンドラの第一引数として渡される。
 *
 *   フィールド  説明
 *   ─────────  ────
 *   rip        例外発生時の命令ポインタ
 *   cs         コードセグメントセレクタ
 *   rflags     フラグレジスタ
 *   rsp        スタックポインタ
 *   ss         スタックセグメントセレクタ
 */
struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

/* IDT を初期化し、lidt で CPU にロードする */
void idt_init(void);

#endif /* IDT_H */
