/*
 * paging.h --- 4レベルページテーブル（ヘッダ）
 *
 * x86-64 の仮想アドレス変換は 4 レベルのページテーブルで行う:
 *   PML4 (Page Map Level 4)    → 512 エントリ, 各 512GB 管理
 *   PDPT (Page Directory Pointer Table) → 512 エントリ, 各 1GB 管理
 *   PD   (Page Directory)      → 512 エントリ, 各 2MB 管理
 *   PT   (Page Table)          → 512 エントリ, 各 4KB 管理
 *
 * 仮想アドレス (48ビット) の分解:
 *   [47:39] PML4 インデックス  (9ビット, 0-511)
 *   [38:30] PDPT インデックス  (9ビット, 0-511)
 *   [29:21] PD インデックス    (9ビット, 0-511)
 *   [20:12] PT インデックス    (9ビット, 0-511)
 *   [11:0]  ページ内オフセット (12ビット, 0-4095)
 *
 * ページテーブルエントリのフラグ:
 *   bit 0: Present  --- エントリが有効
 *   bit 1: Write    --- 書き込み可能
 *   bit 2: User     --- ユーザーモードからアクセス可能
 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* ページテーブルエントリのフラグ */
#define PAGE_PRESENT  (1UL << 0)  /* エントリが有効 */
#define PAGE_WRITE    (1UL << 1)  /* 書き込み可能 */
#define PAGE_USER     (1UL << 2)  /* ユーザーモードアクセス可能 */

/*
 * paging_init --- 4レベルページテーブルを構築してページングを有効化
 *
 * 1. PMM からページテーブル用のページを割り当て
 * 2. 最初の 4MB をアイデンティティマップ（仮想アドレス = 物理アドレス）
 * 3. CR3 に PML4 のアドレスを設定
 *
 * アイデンティティマップにより、ページング有効化前後でアドレスが変わらず、
 * UART やカーネルコードがそのまま動作し続ける。
 */
void paging_init(void);

/*
 * paging_map --- 仮想アドレスを物理アドレスにマッピング
 *
 * 4レベルページテーブルのエントリを設定し、中間テーブルが存在しなければ
 * PMM から新しいページを割り当てて作成する。
 *
 * virt:  マッピングする仮想アドレス（4KB アラインメント必須）
 * phys:  マッピング先の物理アドレス（4KB アラインメント必須）
 * flags: ページテーブルエントリのフラグ（PAGE_PRESENT | PAGE_WRITE 等）
 */
void paging_map(uint64_t virt, uint64_t phys, uint64_t flags);

#endif /* PAGING_H */
