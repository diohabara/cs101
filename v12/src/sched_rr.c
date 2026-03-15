/*
 * sched_rr.c — Round-Robin スケジューラ
 *
 * ucontext API で 3 つのタスク (A, B, C) を交互に実行する。
 * 各タスクは 3 回ループし、毎回 "NAME:ITER" を印字して yield する。
 *
 * 期待される出力:
 *   A:1
 *   B:1
 *   C:1
 *   A:2
 *   B:2
 *   C:2
 *   A:3
 *   B:3
 *   C:3
 *
 * コンテキストスイッチの流れ:
 *   1. スケジューラが swapcontext でタスク A に切り替え
 *   2. タスク A が印字後、swapcontext でスケジューラに戻る（yield）
 *   3. スケジューラが次のタスク B に切り替え
 *   4. ... 以下繰り返し
 *
 * swapcontext(&from, &to) は:
 *   - from に現在の全レジスタ＋スタックポインタを保存
 *   - to から全レジスタ＋スタックポインタを復元
 *   これは v4 の push/pop の究極形。
 */

#include <stdio.h>
#include <stdlib.h>
#include "sched.h"

/* グローバル状態 */
static task_t     tasks[MAX_TASKS];
static int        num_tasks = 0;
static int        current_task = -1;  /* 現在実行中のタスク番号 */
static ucontext_t sched_ctx;          /* スケジューラのコンテキスト */

/*
 * yield — 現在のタスクからスケジューラに制御を戻す
 *
 * swapcontext で「自分のレジスタ状態を保存し、スケジューラの状態に復元」。
 * 次に swapcontext でこのタスクに戻ると、ここから再開する。
 */
static void yield(void) {
    int me = current_task;
    swapcontext(&tasks[me].ctx, &sched_ctx);
}

/*
 * task_func — タスクのエントリポイント
 *
 * makecontext で登録され、タスクが初めて実行されるときに呼ばれる。
 * ループで印字 → yield を繰り返す。
 */
static void task_func(int task_id) {
    const int iterations = 3;
    for (int i = 1; i <= iterations; i++) {
        printf("%s:%d\n", tasks[task_id].name, i);
        yield();
    }
    /* ループ終了 → タスク完了 */
    tasks[task_id].state = TASK_FINISHED;
    /* スケジューラに戻る（二度とこのタスクには戻らない） */
    swapcontext(&tasks[task_id].ctx, &sched_ctx);
}

/*
 * add_task — タスクを登録する
 *
 * ucontext_t を初期化し、makecontext でエントリポイントを設定する。
 * getcontext → スタック設定 → makecontext の 3 ステップ。
 */
static void add_task(const char *name) {
    int id = num_tasks++;
    task_t *t = &tasks[id];

    t->name     = name;
    t->priority = 0;
    t->state    = TASK_READY;

    /* 現在のコンテキストをコピー（テンプレート取得） */
    getcontext(&t->ctx);

    /* タスク専用スタックを設定 */
    t->ctx.uc_stack.ss_sp   = t->stack;
    t->ctx.uc_stack.ss_size = STACK_SIZE;

    /* タスク完了時の遷移先（ここではスケジューラに戻る） */
    t->ctx.uc_link = &sched_ctx;

    /* エントリポイントを登録（task_func(id) が呼ばれる） */
    makecontext(&t->ctx, (void (*)(void))task_func, 1, id);
}

/*
 * scheduler — Round-Robin スケジューラ本体
 *
 * 全タスクを順番に実行し、全タスクが FINISHED になるまでループ。
 * swapcontext でタスクに制御を渡し、yield() で戻ってくる。
 */
static void scheduler(void) {
    while (1) {
        int all_done = 1;

        for (int i = 0; i < num_tasks; i++) {
            if (tasks[i].state == TASK_READY) {
                all_done = 0;
                current_task = i;
                /* スケジューラ → タスクに切り替え */
                swapcontext(&sched_ctx, &tasks[i].ctx);
            }
        }

        if (all_done) {
            break;
        }
    }
}

int main(void) {
    /* 3 つのタスクを登録 */
    add_task("A");
    add_task("B");
    add_task("C");

    /* スケジューラ開始 */
    scheduler();

    return 0;
}
