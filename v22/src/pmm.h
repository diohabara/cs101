/*
 * pmm.h --- 物理メモリアロケータ（ヘッダ）
 *
 * バディアロケータ（buddy allocator）による物理ページ管理。
 *
 * バディアロケータはページをオーダー（order）単位で管理する:
 *   order 0: 1 ページ  = 4KB
 *   order 1: 2 ページ  = 8KB
 *   order 2: 4 ページ  = 16KB
 *   ...
 *   order 10: 1024 ページ = 4MB
 *
 * 空きブロックはフリーリスト（連結リスト）で管理する。
 * 空きページの先頭にリストの次ポインタを格納する（追加メモリ不要）。
 */

#ifndef PMM_H
#define PMM_H

#include <stdint.h>

/* ページサイズ: 4KB */
#define PAGE_SIZE 4096

/* 最大オーダー: order 10 = 1024 ページ = 4MB */
#define MAX_ORDER 10

/*
 * pmm_init --- 物理メモリアロケータを初期化
 *
 * 指定範囲のメモリをフリーリストに登録する。
 * mem_start はページ境界にアラインされている必要がある。
 *
 * mem_start: 管理対象メモリの開始アドレス（例: 0x200000 = 2MB）
 * mem_size:  管理対象メモリのバイト数（例: 0x200000 = 2MB）
 */
void pmm_init(uint64_t mem_start, uint64_t mem_size);

/*
 * pmm_alloc --- 物理ページを割り当て
 *
 * 2^order ページ（= 2^order * 4KB）の連続物理メモリを返す。
 * メモリ不足の場合は 0 (NULL) を返す。
 *
 * order: 0 = 4KB, 1 = 8KB, 2 = 16KB, ..., 10 = 4MB
 */
void *pmm_alloc(int order);

/*
 * pmm_free --- 物理ページを解放
 *
 * pmm_alloc で取得したメモリを返却する。
 * バディ（隣接する同サイズのブロック）が空いていればマージする。
 *
 * addr:  解放するメモリの先頭アドレス
 * order: 割り当て時のオーダー
 */
void pmm_free(void *addr, int order);

#endif /* PMM_H */
