/*
 * kernel.c --- ミニ OS カーネル（実装）
 *
 * ラウンドロビン方式の協調的スケジューラを実装する。
 *
 * スケジューリングフロー:
 *   1. タスクテーブルを順番にスキャン
 *   2. READY 状態のタスクを見つけたら RUNNING に変更して実行
 *   3. タスク関数が返ったら iterations を減らし、0 なら DONE に変更
 *   4. 全タスクが DONE になるまで繰り返し
 *
 * 協調的スケジューリングの特徴:
 *   - タスクが自発的に制御を返す（関数が return する）
 *   - プリエンプション（強制切り替え）なし
 *   - コンテキストスイッチが単純（スタック切り替え不要）
 *   - タスクが無限ループすると他のタスクが動けない
 */

#include "kernel.h"
#include "uart.h"

/* タスクテーブル */
static struct task tasks[MAX_TASKS];
static int task_count = 0;
static int current_task = -1;

void kernel_add_task(const char *name, void (*entry)(void), int iterations) {
    if (task_count >= MAX_TASKS) {
        uart_puts("Task table full\n");
        return;
    }

    struct task *t = &tasks[task_count];
    t->state      = TASK_READY;
    t->id         = (uint32_t)task_count;
    t->name       = name;
    t->entry      = entry;
    t->iterations = iterations;

    task_count++;
}

/*
 * schedule --- 次に実行するタスクを選択する
 *
 * ラウンドロビン: 現在のタスクの次から順にスキャンし、
 * 最初に見つかった READY 状態のタスクを返す。
 *
 * 戻り値: タスクインデックス、または -1（実行可能タスクなし）
 */
static int schedule(void) {
    for (int i = 0; i < task_count; i++) {
        int idx = (current_task + 1 + i) % task_count;
        if (tasks[idx].state == TASK_READY) {
            return idx;
        }
    }
    return -1;
}

/* 数値を 10 進で出力するヘルパー */
static void uart_put_dec(int val) {
    if (val < 0) {
        uart_putc('-');
        val = -val;
    }
    if (val >= 10) {
        uart_put_dec(val / 10);
    }
    uart_putc('0' + (val % 10));
}

void kernel_main(void) {
    uart_puts("Kernel: ");
    uart_put_dec(task_count);
    uart_puts(" tasks registered\n");

    /* スケジューラのメインループ */
    for (;;) {
        int next = schedule();
        if (next < 0) {
            /* 全タスク完了 */
            uart_puts("Kernel: all tasks done\n");
            break;
        }

        current_task = next;
        struct task *t = &tasks[current_task];

        /* タスク実行 */
        t->state = TASK_RUNNING;
        uart_puts("[");
        uart_puts(t->name);
        uart_puts("] iteration ");
        /* 残り回数を出力 */
        uart_put_dec(t->iterations);
        uart_puts(" remaining\n");

        /* タスクのエントリポイントを呼び出し */
        t->entry();

        /* イテレーション管理 */
        t->iterations--;
        if (t->iterations <= 0) {
            t->state = TASK_DONE;
            uart_puts("[");
            uart_puts(t->name);
            uart_puts("] completed\n");
        } else {
            t->state = TASK_READY;
        }
    }
}
