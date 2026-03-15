/*
 * pmm.h --- 物理メモリマネージャ（ヘッダ）
 *
 * ビットマップベースの物理ページアロケータ。
 * 物理メモリを 4KB ページ単位で管理し、alloc/free を提供する。
 *
 * ビットマップ: 1 ビットが 1 ページ (4KB) に対応
 *   0 = 空きページ
 *   1 = 使用中ページ
 */

#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

/* 物理メモリマネージャを初期化する */
void pmm_init(uint64_t mem_start, uint64_t mem_size);

/* 1 ページ (4KB) を確保し、物理アドレスを返す。失敗時は 0 */
uint64_t pmm_alloc(void);

/* 1 ページを解放する */
void pmm_free(uint64_t addr);

/* 空きページ数を返す */
uint64_t pmm_free_count(void);

#endif /* PMM_H */
