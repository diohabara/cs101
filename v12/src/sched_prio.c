/*
 * sched_prio.c — 優先度スケジューラ
 *
 * ucontext API で 3 つのタスク (HIGH, MED, LOW) を優先度順に実行する。
 * スケジューラは常に最も優先度の高い READY タスクを選択する。
 *
 * タスク設定:
 *   HIGH (優先度 3): 2 回実行
 *   MED  (優先度 2): 2 回実行
 *   LOW  (優先度 1): 2 回実行
 *
 * 期待される出力:
 *   HIGH:1
 *   HIGH:2
 *   MED:1
 *   MED:2
 *   LOW:1
 *   LOW:2
 *
 * Round-Robin との違い:
 *   RR:   A→B→C→A→B→C（均等に順番）
 *   優先度: HIGH→HIGH→MED→MED→LOW→LOW（高優先度が先に完了）
 *
 * 実際の OS カーネルでは、飢餓（starvation）を防ぐために
 * 優先度の動的調整やエイジングを組み合わせる。
 */

#include <stdio.h>
#include <stdlib.h>
#include "sched.h"

/* グローバル状態 */
static task_t     tasks[MAX_TASKS];
static int        num_tasks = 0;
static int        current_task = -1;
static ucontext_t sched_ctx;

/* 各タスクの反復回数 */
#define ITERATIONS 2

/*
 * yield — 現在のタスクからスケジューラに制御を戻す
 */
static void yield(void) {
    int me = current_task;
    swapcontext(&tasks[me].ctx, &sched_ctx);
}

/*
 * task_func — タスクのエントリポイント
 */
static void task_func(int task_id) {
    for (int i = 1; i <= ITERATIONS; i++) {
        printf("%s:%d\n", tasks[task_id].name, i);
        yield();
    }
    tasks[task_id].state = TASK_FINISHED;
    swapcontext(&tasks[task_id].ctx, &sched_ctx);
}

/*
 * add_task — タスクを優先度付きで登録する
 */
static void add_task(const char *name, int priority) {
    int id = num_tasks++;
    task_t *t = &tasks[id];

    t->name     = name;
    t->priority = priority;
    t->state    = TASK_READY;

    getcontext(&t->ctx);

    t->ctx.uc_stack.ss_sp   = t->stack;
    t->ctx.uc_stack.ss_size = STACK_SIZE;
    t->ctx.uc_link          = &sched_ctx;

    makecontext(&t->ctx, (void (*)(void))task_func, 1, id);
}

/*
 * pick_highest — 最も優先度の高い READY タスクを返す
 *
 * 同じ優先度のタスクがある場合、先に登録されたものが選ばれる。
 * READY タスクがなければ -1 を返す。
 */
static int pick_highest(void) {
    int best_id  = -1;
    int best_pri = -1;

    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].state == TASK_READY && tasks[i].priority > best_pri) {
            best_pri = tasks[i].priority;
            best_id  = i;
        }
    }
    return best_id;
}

/*
 * scheduler — 優先度スケジューラ本体
 *
 * 毎回 pick_highest() で最高優先度のタスクを選択。
 * RR と違い、高優先度タスクが完了するまで低優先度は実行されない。
 */
static void scheduler(void) {
    while (1) {
        int next = pick_highest();
        if (next < 0) {
            break;  /* 全タスク完了 */
        }
        current_task = next;
        swapcontext(&sched_ctx, &tasks[next].ctx);
    }
}

int main(void) {
    /* 登録順は問わない — 優先度で実行順が決まる */
    add_task("LOW",  1);
    add_task("MED",  2);
    add_task("HIGH", 3);

    scheduler();

    return 0;
}
