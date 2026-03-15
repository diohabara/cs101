/*
 * boot.c --- ベアメタルカーネルのエントリポイント
 *
 * 起動シーケンス:
 *   1. UART 初期化 --- シリアル出力を有効化
 *   2. GDT 初期化 --- セグメントセレクタを設定
 *   3. IDT 初期化 --- 割り込みディスクリプタテーブルをロード
 *   4. タイマー初期化 --- PIT を 100Hz に設定
 *   5. PMM 初期化 --- 物理メモリアロケータ
 *   6. ページング初期化 --- 4レベルページテーブル構築、アイデンティティマップ
 *   7. UART 動作確認 --- ページング後も UART が動作することを検証
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"

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

    /*
     * PMM 初期化: 0x200000 - 0x400000 (2MB) を管理対象にする
     *
     * ページテーブル構築にはページ割り当てが必要なので、
     * paging_init() の前に PMM を初期化する必要がある。
     */
    pmm_init(0x200000, 0x200000);

    /*
     * ページング初期化
     *
     * 最初の 4MB をアイデンティティマップして CR3 を設定する。
     * アイデンティティマップ: 仮想アドレス == 物理アドレス
     * これにより UART (0x3F8), カーネル (0x100000), PMM 領域 (0x200000)
     * が全てページング後もそのままのアドレスでアクセスできる。
     */
    paging_init();

    /* ページング後の UART 動作確認 */
    uart_puts("UART works after paging\n");

    /* 停止: 割り込み無効化 + hlt ループ */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
