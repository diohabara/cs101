/*
 * ept.c --- EPT (Extended Page Tables) の実装
 *
 * EPT テーブルの構築とアドレス変換を実装する。
 *
 * EPT アドレス変換（4 レベル）:
 *
 *   GPA (Guest Physical Address)
 *   ┌─────────┬─────────┬─────────┬─────────┬─────────────┐
 *   │ PML4    │ PDPT    │ PD      │ PT      │ Offset      │
 *   │ [47:39] │ [38:30] │ [29:21] │ [20:12] │ [11:0]      │
 *   │ 9 bits  │ 9 bits  │ 9 bits  │ 9 bits  │ 12 bits     │
 *   └─────────┴─────────┴─────────┴─────────┴─────────────┘
 *
 *   PML4[index] → PDPT テーブルの物理アドレス
 *   PDPT[index] → PD テーブルの物理アドレス
 *   PD[index]   → PT テーブルの物理アドレス
 *   PT[index]   → ホスト物理ページの物理アドレス (HPA)
 *
 * この実装では簡易化のため、各ゲストに 1 つずつの PML4/PDPT/PD/PT テーブルを
 * 静的に割り当て、最大 512 ページ (2MB) のマッピングをサポートする。
 */

#include "ept.h"
#include "uart.h"

/* 16進数出力ヘルパー（先頭ゼロ省略） */
static void uart_put_hex_short(uint64_t n) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (n >> i) & 0xF;
        if (digit != 0 || started || i == 0) {
            uart_putc(hex[digit]);
            started = 1;
        }
    }
}

/*
 * GPA からテーブルインデックスを抽出するマクロ
 */
#define PML4_INDEX(gpa) (((gpa) >> 39) & 0x1FF)
#define PDPT_INDEX(gpa) (((gpa) >> 30) & 0x1FF)
#define PD_INDEX(gpa)   (((gpa) >> 21) & 0x1FF)
#define PT_INDEX(gpa)   (((gpa) >> 12) & 0x1FF)
#define PAGE_OFFSET(gpa) ((gpa) & 0xFFF)

/*
 * memset_local --- ゼロクリア（libc なし環境用）
 */
static void memset_zero(void *ptr, uint64_t size) {
    uint8_t *p = (uint8_t *)ptr;
    for (uint64_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

void ept_init(struct ept_guest *guest, uint64_t gpa_start, uint64_t hpa_start,
              uint64_t size) {
    /* テーブルをゼロクリア */
    memset_zero(&guest->pml4, sizeof(struct ept_table));
    memset_zero(&guest->pdpt, sizeof(struct ept_table));
    memset_zero(&guest->pd, sizeof(struct ept_table));
    memset_zero(&guest->pt, sizeof(struct ept_table));

    /*
     * テーブルチェーンを構築:
     *   PML4[0] → PDPT
     *   PDPT[0] → PD
     *   PD[0]   → PT
     */
    guest->pml4.entries[0] = ((uint64_t)&guest->pdpt & ~0xFFFULL) | EPT_RWX;
    guest->pdpt.entries[0] = ((uint64_t)&guest->pd & ~0xFFFULL) | EPT_RWX;
    guest->pd.entries[0]   = ((uint64_t)&guest->pt & ~0xFFFULL) | EPT_RWX;

    /* 指定された範囲のページをマッピング */
    uint64_t pages = size / 4096;
    if (pages > EPT_ENTRIES_PER_TABLE) {
        pages = EPT_ENTRIES_PER_TABLE; /* 最大 512 ページ */
    }

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t gpa = gpa_start + (i * 4096);
        uint64_t hpa = hpa_start + (i * 4096);
        uint64_t pt_index = PT_INDEX(gpa);
        guest->pt.entries[pt_index] = (hpa & ~0xFFFULL) | EPT_RWX | EPT_MEMTYPE_WB;
    }
}

void ept_map_page(struct ept_guest *guest, uint64_t gpa, uint64_t hpa,
                  uint64_t flags) {
    uint64_t pt_index = PT_INDEX(gpa);
    guest->pt.entries[pt_index] = (hpa & ~0xFFFULL) | flags;
}

uint64_t ept_lookup(const struct ept_guest *guest, uint64_t gpa) {
    /* PML4 エントリ確認 */
    uint64_t pml4e = guest->pml4.entries[PML4_INDEX(gpa)];
    if (!(pml4e & EPT_READ)) {
        return (uint64_t)-1; /* 未マッピング */
    }

    /* PDPT エントリ確認 */
    uint64_t pdpte = guest->pdpt.entries[PDPT_INDEX(gpa)];
    if (!(pdpte & EPT_READ)) {
        return (uint64_t)-1;
    }

    /* PD エントリ確認 */
    uint64_t pde = guest->pd.entries[PD_INDEX(gpa)];
    if (!(pde & EPT_READ)) {
        return (uint64_t)-1;
    }

    /* PT エントリ確認 */
    uint64_t pte = guest->pt.entries[PT_INDEX(gpa)];
    if (!(pte & EPT_READ)) {
        return (uint64_t)-1;
    }

    /* HPA = PT エントリの物理アドレス + ページ内オフセット */
    return (pte & ~0xFFFULL) | PAGE_OFFSET(gpa);
}

void ept_demo(void) {
    uart_puts("--- EPT Demo ---\n");

    /*
     * 2 つのゲスト VM を作成し、同じ GPA が異なる HPA に
     * マッピングされることを示す。
     */
    static struct ept_guest guest_a;
    static struct ept_guest guest_b;
    guest_a.name = "Guest A";
    guest_b.name = "Guest B";

    /*
     * メモリレイアウト:
     *   Guest A: GPA 0x0000-0x1FFF → HPA 0x300000-0x301FFF
     *   Guest B: GPA 0x0000-0x1FFF → HPA 0x400000-0x401FFF
     *
     * 両ゲストが GPA 0x1000 にアクセスしても、実際の物理メモリは異なる。
     */
    ept_init(&guest_a, 0x0000, 0x300000, 8192); /* 2 ページ */
    ept_init(&guest_b, 0x0000, 0x400000, 8192); /* 2 ページ */

    uart_puts("\nGuest A EPT mapping:\n");
    uart_puts("  GPA 0x0000 -> HPA ");
    uint64_t hpa_a0 = ept_lookup(&guest_a, 0x0000);
    uart_put_hex_short(hpa_a0);
    uart_puts("\n");
    uart_puts("  GPA 0x1000 -> HPA ");
    uint64_t hpa_a1 = ept_lookup(&guest_a, 0x1000);
    uart_put_hex_short(hpa_a1);
    uart_puts("\n");

    uart_puts("\nGuest B EPT mapping:\n");
    uart_puts("  GPA 0x0000 -> HPA ");
    uint64_t hpa_b0 = ept_lookup(&guest_b, 0x0000);
    uart_put_hex_short(hpa_b0);
    uart_puts("\n");
    uart_puts("  GPA 0x1000 -> HPA ");
    uint64_t hpa_b1 = ept_lookup(&guest_b, 0x1000);
    uart_put_hex_short(hpa_b1);
    uart_puts("\n");

    /* メモリ隔離の実証 */
    uart_puts("\nMemory isolation:\n");
    if (hpa_a1 != hpa_b1) {
        uart_puts("  Guest A GPA 0x1000 -> HPA ");
        uart_put_hex_short(hpa_a1);
        uart_puts("\n");
        uart_puts("  Guest B GPA 0x1000 -> HPA ");
        uart_put_hex_short(hpa_b1);
        uart_puts("\n");
        uart_puts("  Different HPAs: guests are isolated\n");
    }

    /* EPT violation のデモ（未マッピングの GPA） */
    uart_puts("\nEPT violation test:\n");
    uint64_t unmapped = ept_lookup(&guest_a, 0x2000);
    if (unmapped == (uint64_t)-1) {
        uart_puts("  Guest A GPA 0x2000: unmapped (EPT violation)\n");
        uart_puts("  -> Hypervisor would handle: inject #PF or terminate guest\n");
    }

    uart_puts("\nEPT demo complete\n");
}
