/*
 * bitops.c — ビット演算
 *
 * v2 の RFLAGS レジスタ操作に相当。フラグの設定・クリア・テストを
 * C のビット演算子で表現する。
 *
 * アセンブリとの対応:
 *   OR  → フラグセット   (flags |= FLAG)
 *   AND → フラグテスト   (flags & FLAG)
 *   AND+NOT → フラグクリア (flags &= ~FLAG)
 *   XOR → フラグトグル   (flags ^= FLAG)
 */

#include <stdio.h>
#include <stdlib.h>

/* ビットフラグ定義（RFLAGS の CF, ZF, SF に対応するイメージ） */
#define FLAG_CARRY    (1 << 0)  /* bit 0: キャリーフラグ */
#define FLAG_ZERO     (1 << 1)  /* bit 1: ゼロフラグ */
#define FLAG_SIGN     (1 << 2)  /* bit 2: 符号フラグ */
#define FLAG_OVERFLOW (1 << 3)  /* bit 3: オーバーフローフラグ */

int main(void) {
    unsigned int flags = 0;

    /* OR: フラグを立てる（RFLAGS で CF=1 にするのと同じ） */
    flags |= FLAG_CARRY;
    flags |= FLAG_ZERO;
    printf("set CF,ZF:  flags = 0x%02x\n", flags);  /* 0x03 */

    /* AND: フラグをテスト（test 命令に相当） */
    if (flags & FLAG_CARRY) {
        printf("CF is set\n");
    }
    if (!(flags & FLAG_SIGN)) {
        printf("SF is clear\n");
    }

    /* AND+NOT: フラグをクリア */
    flags &= ~FLAG_ZERO;
    printf("clear ZF:   flags = 0x%02x\n", flags);  /* 0x01 */

    /* XOR: フラグをトグル */
    flags ^= FLAG_CARRY;
    flags ^= FLAG_SIGN;
    printf("toggle CF,SF: flags = 0x%02x\n", flags);  /* 0x04 */

    /* 最終値: FLAG_SIGN のみ = 4 */
    return (int)flags;
}
