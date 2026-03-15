/*
 * gdt.h — Global Descriptor Table（ヘッダ）
 *
 * GDT は x86 CPU のセグメンテーション機構で使うテーブル。
 * 64ビットモード（ロングモード）ではセグメンテーションはほぼ無効化されるが、
 * カーネルモード/ユーザーモードの区別（特権レベル: Ring 0/Ring 3）に必要。
 *
 * 5 エントリ構成:
 *   [0] Null descriptor（CPU が要求する無効エントリ）
 *   [1] Kernel Code (selector 0x08): Ring 0, 実行可能, 64ビット
 *   [2] Kernel Data (selector 0x10): Ring 0, 読み書き可能
 *   [3] User Code   (selector 0x18): Ring 3, 実行可能, 64ビット
 *   [4] User Data   (selector 0x20): Ring 3, 読み書き可能
 */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * GDT エントリ（8バイト）
 *
 * レガシーな構造体だが、64ビットモードでも互換性のために同じフォーマットを使う。
 * ビットフィールドが複数バイトにまたがるため、packed 構造体で定義する。
 *
 *   ビット  フィールド
 *   ──────  ──────────
 *   0-15    limit_low     セグメントサイズ下位16ビット
 *   16-31   base_low      ベースアドレス下位16ビット
 *   32-39   base_mid      ベースアドレス中位8ビット
 *   40-47   access        アクセスバイト（タイプ、DPL、Present）
 *   48-51   limit_high    セグメントサイズ上位4ビット
 *   52-55   flags         フラグ（Granularity、サイズ、Long mode）
 *   56-63   base_high     ベースアドレス上位8ビット
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_high_and_flags;  /* 下位4ビット: limit_high, 上位4ビット: flags */
    uint8_t  base_high;
} __attribute__((packed));

/*
 * GDTR（GDT レジスタ）に渡す構造体
 *
 * lgdt 命令はこの構造体のアドレスを受け取る。
 *   limit: GDT のサイズ - 1（バイト単位）
 *   base:  GDT の先頭アドレス
 */
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* GDT を初期化し、lgdt で CPU にロードする */
void gdt_init(void);

#endif /* GDT_H */
