/*
 * gdt.c --- GDT の初期化とロード
 *
 * 5 エントリの GDT を構築する:
 *   [0] ヌルディスクリプタ  --- CPU が要求する先頭エントリ（全ゼロ）
 *   [1] カーネルコード (0x08) --- Ring 0, 実行可能 + 読み取り可
 *   [2] カーネルデータ (0x10) --- Ring 0, 読み書き可
 *   [3] ユーザーコード (0x18) --- Ring 3, 実行可能 + 読み取り可
 *   [4] ユーザーデータ (0x20) --- Ring 3, 読み書き可
 *
 * lgdt でテーブルをロードした後、far return で CS をリロードし、
 * mov 命令で DS/ES/FS/GS/SS をカーネルデータセレクタに設定する。
 */

#include "gdt.h"
#include "uart.h"

static struct gdt_entry gdt[5];
static struct gdt_ptr gdtr;

/*
 * gdt_set_entry --- GDT エントリを設定する
 *
 * base:   セグメントのベースアドレス（x86-64 フラットモデルでは 0）
 * limit:  セグメントリミット（0xFFFFF = 4GB @ 4K 粒度）
 * access: アクセスバイト
 *           bit 7: P (Present) = 1 でセグメント有効
 *           bit 5-6: DPL (Descriptor Privilege Level) = 0(カーネル) or 3(ユーザー)
 *           bit 4: S = 1 でコード/データセグメント
 *           bit 0-3: Type（コード: 0xA=実行+読み取り, データ: 0x2=読み書き）
 * gran:   粒度 + フラグ（上位 4 ビット）
 *           bit 7: G = 1 で 4K 粒度（リミットを 4096 倍）
 *           bit 6: D/B = 1 で 32 ビットセグメント
 *           bit 5: L = 1 で 64 ビットコードセグメント
 */
static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran) {
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

void gdt_init(void) {
    /*
     * access バイトの構成:
     *   0x9A = 1001 1010 = P=1, DPL=0, S=1, Type=Code+Readable
     *   0x92 = 1001 0010 = P=1, DPL=0, S=1, Type=Data+Writable
     *   0xFA = 1111 1010 = P=1, DPL=3, S=1, Type=Code+Readable
     *   0xF2 = 1111 0010 = P=1, DPL=3, S=1, Type=Data+Writable
     *
     * granularity バイトの構成:
     *   0xA0 = 1010 0000 = G=1, D=0, L=1 (64ビットコード)
     *   0xC0 = 1100 0000 = G=1, D=1, L=0 (32ビットデータ)
     */
    gdt_set_entry(0, 0, 0,       0,    0);     /* ヌルディスクリプタ */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);  /* カーネルコード */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);  /* カーネルデータ */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);  /* ユーザーコード */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);  /* ユーザーデータ */

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;

    /* lgdt 命令で GDT をロード */
    __asm__ volatile ("lgdt %0" : : "m"(gdtr));

    /*
     * CS リロード: far return で新しいコードセレクタ (0x08) に切り替え
     *
     * lretq は スタックから RIP と CS を pop して遠隔ジャンプする。
     * pushq $0x08 でカーネルコードセレクタを、lea + pushq で戻り先アドレスを積む。
     */
    __asm__ volatile (
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        : : : "rax", "memory"
    );

    /*
     * データセグメントレジスタのリロード
     * DS, ES, FS, GS, SS をカーネルデータセレクタ (0x10) に設定する。
     */
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "ax", "memory"
    );
}
