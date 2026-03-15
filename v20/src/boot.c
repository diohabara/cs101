/*
 * boot.c — ベアメタルカーネルのメイン関数
 *
 * boot.asm がハードウェア初期化（ページテーブル、ロングモード遷移）を行った後、
 * この kernel_main() が呼ばれる。
 *
 * 起動シーケンス:
 *   boot.asm: Multiboot → 32ビット保護モード → ページテーブル → 64ビットモード
 *   boot.c:   1. UART 初期化（シリアル出力を使えるようにする）
 *             2. GDT ロード（セグメントセレクタの設定）
 *             3. IDT ロード（例外ハンドラの登録）
 *             4. int3 で #BP 例外を発生させてテスト
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"

/*
 * kernel_main — カーネルメイン関数
 *
 * boot.asm の realm64 から call される。
 * UART、GDT、IDT を順に初期化し、int3 で例外ハンドラをテストする。
 *
 * __attribute__((noreturn)): この関数は返らない。
 * ベアメタルでは「返る先」がないため、最後に hlt ループで停止する。
 */
void __attribute__((noreturn, section(".text.boot"))) kernel_main(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded: selectors 0x08(code) 0x10(data)\n");

    idt_init();
    uart_puts("IDT loaded: 256 entries\n");

    /*
     * int3 命令で #BP (Breakpoint) 例外を発生させる
     *
     * CPU は IDT[3] からハンドラアドレスを取得し、exc_breakpoint() を呼ぶ。
     * ハンドラが "Exception: #BP (Breakpoint)" を出力して hlt する。
     */
    __asm__ volatile ("int3");

    /* int3 ハンドラが hlt するため、ここには到達しない */
    uart_puts("Unreachable after int3\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
