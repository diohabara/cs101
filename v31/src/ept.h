/*
 * ept.h --- EPT (Extended Page Tables) 定義
 *
 * EPT はゲスト物理アドレス (GPA) をホスト物理アドレス (HPA) に変換する
 * ハードウェア機構。通常のページテーブルがバーチャル→物理の変換を行うのに対し、
 * EPT はゲスト物理→ホスト物理の変換を追加する。
 *
 * EPT により、各ゲスト VM は独立した物理アドレス空間を持つ。
 * ゲスト A の GPA 0x1000 とゲスト B の GPA 0x1000 は
 * 異なるホスト物理メモリを指す → メモリの完全な隔離が実現。
 *
 * EPT テーブルは通常のページテーブルと同じ 4 レベル構造:
 *   PML4 → PDPT → PD → PT → 4KB ページ
 *
 * EPT エントリのビット構成:
 *   bit 0: Read access
 *   bit 1: Write access
 *   bit 2: Execute access
 *   bit 3-5: Memory type (0=UC, 6=WB)
 *   bit 12-51: 物理アドレス（4KB アラインメント）
 */

#ifndef EPT_H
#define EPT_H

#include <stdint.h>

/* EPT エントリのフラグ */
#define EPT_READ    (1UL << 0)
#define EPT_WRITE   (1UL << 1)
#define EPT_EXEC    (1UL << 2)
#define EPT_RWX     (EPT_READ | EPT_WRITE | EPT_EXEC)

/* メモリタイプ（bit 3-5） */
#define EPT_MEMTYPE_UC  (0UL << 3)  /* Uncacheable */
#define EPT_MEMTYPE_WB  (6UL << 3)  /* Write-Back */

/* EPT テーブルのエントリ数（各レベル 512 エントリ = 4KB） */
#define EPT_ENTRIES_PER_TABLE 512

/* EPT ページテーブル（4KB アラインメント） */
typedef uint64_t ept_entry_t;

struct ept_table {
    ept_entry_t entries[EPT_ENTRIES_PER_TABLE];
} __attribute__((aligned(4096)));

/*
 * ept_guest --- ゲストの EPT コンテキスト
 *
 * 各ゲストが独立した EPT テーブルツリーを持つ。
 * pml4 がツリーのルート。
 */
struct ept_guest {
    struct ept_table pml4;   /* EPT PML4 テーブル（ルート） */
    struct ept_table pdpt;   /* EPT PDPT テーブル */
    struct ept_table pd;     /* EPT Page Directory テーブル */
    struct ept_table pt;     /* EPT Page Table */
    const char *name;        /* ゲスト名（デバッグ用） */
};

/*
 * ept_init --- EPT テーブルを初期化する
 *
 * 4 レベルの EPT テーブルを構築し、指定された GPA 範囲を
 * 対応する HPA にマッピングする。
 */
void ept_init(struct ept_guest *guest, uint64_t gpa_start, uint64_t hpa_start,
              uint64_t size);

/*
 * ept_map_page --- 1 ページ（4KB）の GPA→HPA マッピングを追加
 */
void ept_map_page(struct ept_guest *guest, uint64_t gpa, uint64_t hpa,
                  uint64_t flags);

/*
 * ept_lookup --- GPA から HPA を検索する（教育・デバッグ用）
 *
 * 戻り値: マッピングされた HPA。未マッピングなら (uint64_t)-1
 */
uint64_t ept_lookup(const struct ept_guest *guest, uint64_t gpa);

/*
 * ept_demo --- EPT の概念をデモする（シミュレーション）
 *
 * VMX の有無にかかわらず、EPT によるメモリ隔離を教育目的で出力する。
 */
void ept_demo(void);

#endif /* EPT_H */
