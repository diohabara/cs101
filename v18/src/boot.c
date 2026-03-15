/*
 * boot.c — ベアメタルカーネルのエントリポイント
 *
 * QEMU の -kernel オプションでフラットバイナリとしてロードされる。
 * ロードアドレス 0x100000 (1MB) からカーネルが配置され、
 * リンカスクリプトで _start シンボルをエントリポイントに指定する。
 *
 * OS なし、libc なし。利用できるのは CPU 命令とハードウェアレジスタだけ。
 * 最初に UART を初期化して、シリアルポートに文字を出力する。
 */

#include "uart.h"

/*
 * _start — カーネルエントリポイント
 *
 * リンカスクリプトが _start をエントリポイントとして指定する。
 * QEMU が -kernel でロードした後、CPU はこの関数から実行を開始する。
 *
 * __attribute__((noreturn)): この関数は返らない。
 * ベアメタルでは「返る先」がないため、最後に hlt ループで停止する。
 */
void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    /* 停止: hlt ループ */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
