/*
 * boot.c --- ベアメタルカーネルのエントリポイント
 *
 * QEMU の -kernel オプションでフラットバイナリとしてロードされる。
 * ロードアドレス 0x100000 (1MB) からカーネルが配置され、
 * リンカスクリプトで _start シンボルをエントリポイントに指定する。
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定、PIC をリマップ
 *   5. 割り込み有効化 --- sti 命令で割り込みを受け付け開始
 *   6. 50 ティック待機 --- hlt ループでタイマー割り込みを待つ
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded\n");

    idt_init();
    uart_puts("IDT loaded\n");

    timer_init(100);
    uart_puts("Timer started: 100Hz\n");

    /* 割り込み有効化 */
    __asm__ volatile ("sti");

    /* 50 ティック到達まで hlt ループで待機 */
    while (timer_get_ticks() < 50) {
        __asm__ volatile ("hlt");
    }
    uart_puts("50 ticks reached\n");

    /* 停止: 割り込み無効化 + hlt ループ */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
