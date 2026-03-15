/*
 * gdt.c — Global Descriptor Table（実装）
 *
 * GDT を構築し、lgdt 命令で CPU にロードする。
 * ロード後、セグメントレジスタを新しいセレクタで再設定する。
 *
 * 64ビットモードではベースアドレスとリミットは無視されるが、
 * access バイトと flags の設定は必要:
 *   - access: Present, DPL, タイプ（Code/Data）
 *   - flags:  Long mode ビット（コードセグメントのみ）
 */

#include "gdt.h"
#include "uart.h"

/*
 * GDT エントリのヘルパー関数
 *
 * 64ビットモードでは base=0, limit=0 で構わない。
 * access と flags だけが意味を持つ。
 *
 *   access ビット:
 *     bit 7: Present (1=有効)
 *     bit 6-5: DPL (0=Ring 0, 3=Ring 3)
 *     bit 4: Descriptor type (1=code/data)
 *     bit 3: Executable (1=code, 0=data)
 *     bit 1: Read/Write (code: 読み取り可, data: 書き込み可)
 *     bit 0: Accessed (CPU が自動設定)
 *
 *   flags ビット（上位4ビット）:
 *     bit 7 (of byte): Granularity
 *     bit 6: Size (0=16bit, 1=32bit)
 *     bit 5: Long mode (1=64bit code segment)
 */
static struct gdt_entry make_entry(uint8_t access, uint8_t flags) {
    struct gdt_entry e;
    e.limit_low = 0;
    e.base_low  = 0;
    e.base_mid  = 0;
    e.access    = access;
    e.limit_high_and_flags = flags;  /* 上位4ビット: flags, 下位4ビット: limit_high=0 */
    e.base_high = 0;
    return e;
}

/* GDT 本体: 5 エントリ */
static struct gdt_entry gdt[5];

/* GDTR に渡す構造体 */
static struct gdt_ptr gdtr;

void gdt_init(void) {
    /*
     * エントリ設定
     *
     *   # | Selector | access | flags | 用途
     *   ──|──────────|────────|───────|──────────────
     *   0 | 0x00     | 0x00   | 0x00  | Null（必須）
     *   1 | 0x08     | 0x9A   | 0xA0  | Kernel Code: Present, DPL=0, Exec+Read, Long mode
     *   2 | 0x10     | 0x92   | 0xC0  | Kernel Data: Present, DPL=0, Read+Write
     *   3 | 0x18     | 0xFA   | 0xA0  | User Code:   Present, DPL=3, Exec+Read, Long mode
     *   4 | 0x20     | 0xF2   | 0xC0  | User Data:   Present, DPL=3, Read+Write
     */
    gdt[0] = make_entry(0x00, 0x00);  /* Null descriptor */
    gdt[1] = make_entry(0x9A, 0xA0);  /* Kernel Code */
    gdt[2] = make_entry(0x92, 0xC0);  /* Kernel Data */
    gdt[3] = make_entry(0xFA, 0xA0);  /* User Code */
    gdt[4] = make_entry(0xF2, 0xC0);  /* User Data */

    /* GDTR 設定 */
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt[0];

    /* lgdt でロード */
    __asm__ volatile ("lgdt (%0)" : : "r"(&gdtr));

    /*
     * セグメントレジスタの再設定
     *
     * lgdt だけでは不十分。セグメントレジスタに新しいセレクタをロードする必要がある。
     *   CS (Code Segment): far return で設定（mov cs, xx はできない）
     *   DS, ES, FS, GS, SS: mov 命令で直接設定
     */

    /* CS を 0x08（Kernel Code）に設定: push selector + offset → retfq */
    __asm__ volatile (
        "pushq $0x08\n\t"        /* 新しい CS セレクタ */
        "leaq 1f(%%rip), %%rax\n\t"  /* 戻り先アドレス */
        "pushq %%rax\n\t"
        "lretq\n\t"             /* far return: CS:RIP をポップ */
        "1:\n\t"
        : : : "rax", "memory"
    );

    /* データセグメントレジスタを 0x10（Kernel Data）に設定 */
    __asm__ volatile (
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        : : : "rax"
    );
}
