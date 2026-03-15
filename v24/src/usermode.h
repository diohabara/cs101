/*
 * usermode.h --- ユーザーモード遷移（ヘッダ）
 *
 * TSS (Task State Segment) と syscall/sysret 命令を使って、
 * Ring 0 (カーネルモード) から Ring 3 (ユーザーモード) に遷移する。
 *
 * TSS は x86-64 でユーザーモードからカーネルモードに戻るとき（syscall, 割り込み）に
 * CPU がカーネルスタックのアドレスを知るために必要。
 *   - RSP0: Ring 0 で使うスタックポインタ
 *   - IST[1-7]: 特定の割り込み用のスタック（オプション）
 *
 * v24 では syscall 命令の動作を Ring 0 からテストする（概念実証）。
 * 完全な Ring 3 遷移は TSS + GDT 修正を伴うため、v26 で統合する。
 */

#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

/* TSS (Task State Segment) for x86-64 */
struct tss {
    uint32_t reserved0;
    uint64_t rsp0;      /* Ring 0 用スタックポインタ */
    uint64_t rsp1;      /* Ring 1 用（未使用） */
    uint64_t rsp2;      /* Ring 2 用（未使用） */
    uint64_t reserved1;
    uint64_t ist[7];    /* Interrupt Stack Table エントリ 1-7 */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;      /* I/O Permission Bitmap のオフセット */
} __attribute__((packed));

/* syscall/sysret 機構を初期化する */
void usermode_init(void);

#endif /* USERMODE_H */
