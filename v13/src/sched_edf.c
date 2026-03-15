/*
 * sched_edf.c -- EDF (Earliest Deadline First) スケジューラ シミュレーション
 *
 * 3 つのタスクをデッドラインが近い順に実行する。
 * EDF はリアルタイムスケジューリングの最適アルゴリズムで、
 * 単一プロセッサ上でスケジュール可能なタスクセットを
 * 必ずスケジュールできることが証明されている。
 *
 * タスク:
 *   A: deadline=10, exec_time=3
 *   B: deadline=5,  exec_time=2
 *   C: deadline=8,  exec_time=1
 *
 * EDF はデッドラインが最も近いタスクを優先する。
 * ソート後の実行順: B(5) -> C(8) -> A(10)
 */

#include <stdio.h>
#include <stdlib.h>
#include "rt_sched.h"

/* デッドライン昇順でタスク配列をソート（選択ソート） */
static void sort_by_deadline(Task tasks[], int n)
{
    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (tasks[j].deadline < tasks[min_idx].deadline) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            Task tmp = tasks[i];
            tasks[i] = tasks[min_idx];
            tasks[min_idx] = tmp;
        }
    }
}

int main(void)
{
    Task tasks[] = {
        { .name = "A", .deadline = 10, .exec_time = 3 },
        { .name = "B", .deadline =  5, .exec_time = 2 },
        { .name = "C", .deadline =  8, .exec_time = 1 },
    };
    int n = (int)(sizeof(tasks) / sizeof(tasks[0]));

    /* EDF: デッドラインが近い順にソート */
    sort_by_deadline(tasks, n);

    /* 実行順を表示 */
    printf("EDF order: ");
    for (int i = 0; i < n; i++) {
        if (i > 0) printf(" -> ");
        printf("%s(deadline=%d)", tasks[i].name, tasks[i].deadline);
    }
    printf("\n");

    /* タスクを順番に実行し、完了 tick を記録 */
    int tick = 0;
    for (int i = 0; i < n; i++) {
        tick += tasks[i].exec_time;
        tasks[i].completed = tick;
        printf("%s completed at tick %d", tasks[i].name, tick);
        if (tick <= tasks[i].deadline) {
            printf(" (deadline %d: met)\n", tasks[i].deadline);
        } else {
            printf(" (deadline %d: MISSED)\n", tasks[i].deadline);
            /* デッドラインミスは本来あってはならない */
            return 1;
        }
    }

    printf("All tasks met their deadlines.\n");
    return 0;
}
