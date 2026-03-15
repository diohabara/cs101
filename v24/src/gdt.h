/*
 * gdt.h --- GDT (Global Descriptor Table) 定義
 *
 * GDT はメモリセグメントの属性と特権レベルを定義するテーブル。
 * x86-64 ではフラットメモリモデルを使うが、セグメントセレクタは
 * 特権レベル（Ring 0 = カーネル、Ring 3 = ユーザー）の切り替えに必須。
 *
 * GDT エントリ構造（8 バイト）:
 *
 *   ビット  63..56  55..52  51..48  47..40   39..32   31..16    15..0
 *          base_hi  flags  lim_hi  access   base_mid  base_low  limit_low
 *
 *   flags (4ビット): G=粒度(4K), D/B=デフォルトサイズ, L=ロングモード
 *   access (8ビット): P=存在, DPL=特権レベル(0-3), S=タイプ, Type=コード/データ
 */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* GDT エントリ（8 バイト）--- packed で構造体パディングを禁止 */
struct gdt_entry {
    uint16_t limit_low;   /* セグメントリミットの下位 16 ビット */
    uint16_t base_low;    /* ベースアドレスの下位 16 ビット */
    uint8_t  base_mid;    /* ベースアドレスのビット 16..23 */
    uint8_t  access;      /* アクセスバイト（P, DPL, S, Type） */
    uint8_t  granularity; /* フラグ (上位4ビット) + リミット上位 (下位4ビット) */
    uint8_t  base_high;   /* ベースアドレスのビット 24..31 */
} __attribute__((packed));

/* GDT ポインタ --- lgdt 命令に渡す構造体 */
struct gdt_ptr {
    uint16_t limit;  /* GDT のバイトサイズ - 1 */
    uint64_t base;   /* GDT の先頭アドレス */
} __attribute__((packed));

/* GDT を初期化し、lgdt でロード後にセグメントレジスタをリロードする */
void gdt_init(void);

#endif /* GDT_H */
