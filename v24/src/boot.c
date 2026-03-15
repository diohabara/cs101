/*
 * boot.c --- ベアメタルカーネルのエントリポイント（v24）
 *
 * v18 からの累積:
 *   v18: UART 初期化 + シリアル出力
 *   v19: GDT 初期化 + セグメントセレクタ設定
 *   v20: IDT 初期化 + PIC 再マッピング
 *   v21: PIT タイマー初期化
 *   v22: 物理メモリマネージャ (PMM) 初期化
 *   v23: ページング初期化
 *   v24: syscall/sysret 初期化 + syscall テスト
 *
 * v24 では syscall 命令の動作を Ring 0 から概念実証する。
 * LSTAR MSR に登録したハンドラが呼ばれ、sys_write で UART に出力し、
 * sysretq で呼び出し元に戻ることを確認する。
 */

#include "uart.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "usermode.h"

void __attribute__((noreturn, section(".text.boot"))) _start(void) {
    uart_init();
    uart_puts("Hello, bare metal!\n");

    gdt_init();
    uart_puts("GDT loaded\n");

    idt_init();
    uart_puts("IDT loaded\n");

    timer_init(100);
    uart_puts("Timer started\n");

    /*
     * PMM 初期化
     * 0x400000 (4MB) から 1MB 分の物理メモリを管理対象にする。
     * カーネルは 0x100000-0x3FFFFF に配置されるため、それ以降を使う。
     */
    pmm_init(0x400000, 0x100000);
    uart_puts("PMM initialized\n");

    paging_init();
    uart_puts("Paging enabled\n");

    usermode_init();
    uart_puts("Syscall handler ready\n");

    /*
     * syscall テスト（Ring 0 から）
     *
     * 通常 syscall はユーザーモード (Ring 3) から呼ばれるが、
     * Ring 0 からでも LSTAR のハンドラにジャンプする。
     * これで syscall_dispatch → uart_putc のパスが正しく動くことを確認する。
     *
     * syscall 番号 1 (sys_write):
     *   RAX = 1 (syscall 番号)
     *   RDI = buf のアドレス
     *   RSI = 0 (未使用)
     *   RDX = 20 (バッファ長)
     */
    uart_puts("Testing syscall...\n");
    const char msg[] = "Hello from syscall!\n";
    __asm__ volatile (
        "mov $1, %%rax\n"       /* syscall 番号: sys_write */
        "mov %0, %%rdi\n"       /* 第 1 引数: バッファアドレス */
        "mov $0, %%rsi\n"       /* 第 2 引数: 未使用 */
        "mov $20, %%rdx\n"      /* 第 3 引数: バッファ長 */
        "syscall\n"
        : : "r"(msg) : "rax", "rcx", "rdx", "rdi", "rsi", "r11", "memory"
    );
    uart_puts("Syscall test complete\n");

    /* 停止: 割り込み無効化 + hlt ループ */
    for (;;) {
        __asm__ volatile ("cli\nhlt");
    }
}
