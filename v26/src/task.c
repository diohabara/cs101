/*
 * task.c --- サンプルタスク
 *
 * カーネルに登録されるタスク関数を定義する。
 * 各タスクは 1 回の呼び出しで簡単な処理を行い、return で制御を返す。
 * カーネルのスケジューラが再び呼び出すまで待つ。
 *
 * 協調的マルチタスクでは、タスク関数は必ず return すること。
 * 無限ループするとスケジューラに制御が戻らず、他のタスクが実行されない。
 */

#include "uart.h"
#include "kernel.h"

/*
 * task_alpha --- タスク A
 *
 * 呼び出されるたびに "Alpha working" と出力する。
 */
void task_alpha(void) {
    uart_puts("  Alpha working\n");
}

/*
 * task_beta --- タスク B
 *
 * 呼び出されるたびに "Beta working" と出力する。
 */
void task_beta(void) {
    uart_puts("  Beta working\n");
}
