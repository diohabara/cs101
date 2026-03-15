/*
 * syscall_handler.c --- システムコールハンドラ
 *
 * syscall 命令で呼び出されるエントリポイントと、
 * システムコール番号に基づくディスパッチャを実装する。
 *
 * syscall 命令の動作:
 *   1. RCX ← 戻り先 RIP（次の命令のアドレス）
 *   2. R11 ← 現在の RFLAGS
 *   3. RFLAGS &= ~SFMASK（IF をクリアして割り込み無効化）
 *   4. CS ← STAR[47:32]（カーネルコードセレクタ）
 *   5. RIP ← LSTAR（このファイルの syscall_entry）
 *
 * sysret 命令の動作:
 *   1. RIP ← RCX（保存した戻り先アドレス）
 *   2. RFLAGS ← R11（保存した RFLAGS を復元）
 *   3. CS ← STAR[63:48] + 16（ユーザーコードセレクタ）
 *   4. SS ← STAR[63:48] + 8（ユーザーデータセレクタ）
 *
 * サポートするシステムコール:
 *   番号 1:  sys_write(buf, unused, len) --- UART にバッファの内容を出力
 *   番号 60: sys_exit(code) --- 終了コードを表示して停止
 */

#include "uart.h"
#include <stdint.h>

/*
 * syscall_entry --- syscall 命令のエントリポイント
 *
 * naked 属性: コンパイラがプロローグ/エピローグを生成しない。
 * レジスタの保存と復元を手動で行い、sysretq で戻る。
 *
 * syscall 時のレジスタ:
 *   RAX = システムコール番号
 *   RDI = 第 1 引数
 *   RSI = 第 2 引数
 *   RDX = 第 3 引数
 *   RCX = 戻り先 RIP（CPU が自動設定）
 *   R11 = 保存 RFLAGS（CPU が自動設定）
 */
__attribute__((naked)) void syscall_entry(void) {
    __asm__ volatile (
        "push %%rcx\n"   /* RCX = 戻り先 RIP を保存 */
        "push %%r11\n"   /* R11 = RFLAGS を保存 */
        "push %%rbx\n"   /* callee-saved レジスタ */

        /* syscall_dispatch を呼び出し（RAX, RDI, RSI, RDX はそのまま渡される） */
        "call syscall_dispatch\n"

        "pop %%rbx\n"
        "pop %%r11\n"
        "pop %%rcx\n"
        "sysretq\n"
        ::: "memory"
    );
}

/*
 * syscall_dispatch --- システムコール番号に基づくディスパッチャ
 *
 * レジスタからシステムコール番号と引数を取得し、対応する処理を実行する。
 */
void syscall_dispatch(void) {
    uint64_t num, arg1, arg2, arg3;
    __asm__ volatile ("" : "=a"(num), "=D"(arg1), "=S"(arg2), "=d"(arg3));

    switch (num) {
        case 1: {
            /* sys_write(buf, unused, len) --- UART にバッファを出力 */
            const char *buf = (const char *)arg1;
            for (uint64_t i = 0; i < arg3; i++) {
                uart_putc(buf[i]);
            }
            break;
        }
        case 60: {
            /* sys_exit(code) --- 終了コードを表示して停止 */
            uart_puts("User exit: code ");
            char c = '0' + (arg1 % 10);
            uart_putc(c);
            uart_putc('\n');
            for (;;) __asm__ volatile ("hlt");
        }
        default:
            uart_puts("Unknown syscall\n");
            break;
    }
}
