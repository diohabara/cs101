/*
 * paging.h --- ページング（ヘッダ）
 *
 * x86-64 の 4 レベルページテーブルを設定し、仮想アドレスを物理アドレスにマッピングする。
 *
 * ページテーブル構造（4 レベル）:
 *   PML4 (Page Map Level 4)     → 512 エントリ、各 512GB カバー
 *   PDPT (Page Directory Pointer Table) → 512 エントリ、各 1GB カバー
 *   PD   (Page Directory)       → 512 エントリ、各 2MB カバー
 *   PT   (Page Table)           → 512 エントリ、各 4KB カバー
 *
 * 簡略化のため、最初の 2MB を 4KB ページでアイデンティティマッピングする。
 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* ページテーブルエントリのフラグ */
#define PTE_PRESENT  0x01   /* ページが存在する */
#define PTE_WRITABLE 0x02   /* 書き込み可能 */
#define PTE_USER     0x04   /* ユーザーモードからアクセス可能 */

/* ページングを初期化し、CR3 を設定する */
void paging_init(void);

#endif /* PAGING_H */
