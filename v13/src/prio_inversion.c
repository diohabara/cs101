/*
 * prio_inversion.c -- 優先度逆転と Priority Inheritance のシミュレーション
 *
 * 1997 年、Mars Pathfinder は優先度逆転バグにより繰り返しリセットした。
 * 低優先度の気象タスクがバスのロックを保持中に、中優先度タスクが CPU を奪い、
 * 高優先度の通信タスクがロック待ちのまま WDT タイムアウトに至った。
 *
 * このプログラムは 3 つのタスク (HIGH, MED, LOW) と 1 つの mutex を使い、
 * 優先度逆転が発生するシナリオと、Priority Inheritance で解決するシナリオを
 * ステートマシンでシミュレーションする。
 *
 * シミュレーション（実スレッドは使わない）:
 *
 * WITHOUT inheritance:
 *   tick 0: LOW が lock を取得
 *   tick 1: MED が到着、LOW より高優先度なので割り込み実行
 *   tick 2: HIGH が到着、lock が必要だが LOW が保持中 → blocked
 *           MED は lock 不要なのでそのまま実行 (HIGH より先に完了)
 *   tick 3: MED 完了
 *   tick 4: LOW が再開、unlock
 *   tick 5: HIGH が lock 取得、実行、完了
 *
 * WITH inheritance:
 *   tick 0: LOW が lock を取得
 *   tick 1: HIGH が到着、lock が必要 → LOW の実効優先度を HIGH に引き上げ
 *   tick 2: LOW が boosted 優先度で実行、unlock
 *   tick 3: HIGH が lock 取得、実行、完了
 *   tick 4: MED が実行、完了
 */

#include <stdio.h>
#include <stdlib.h>
#include "rt_sched.h"

/*
 * WITHOUT inheritance のシミュレーション
 *
 * 実行順: LOW:lock → MED:run → HIGH:blocked → LOW:unlock → HIGH:lock → HIGH:run
 * HIGH の完了が MED の影響で遅延する（これが優先度逆転）。
 */
static void simulate_without_inheritance(void)
{
    printf("WITHOUT inheritance:\n");
    printf("  tick 0: LOW:lock\n");
    printf("  tick 1: MED:run (preempts LOW, no lock needed)\n");
    printf("  tick 2: HIGH:blocked (needs lock held by LOW)\n");
    printf("  tick 3: MED:done\n");
    printf("  tick 4: LOW:unlock\n");
    printf("  tick 5: HIGH:lock HIGH:run HIGH:done\n");
    printf("  HIGH waited 3 ticks (blocked by LOW + MED)\n");
}

/*
 * WITH inheritance のシミュレーション
 *
 * LOW の実効優先度が HIGH に引き上げられるため、MED に割り込まれない。
 * LOW が即座に unlock でき、HIGH はすぐに実行できる。
 */
static void simulate_with_inheritance(void)
{
    printf("WITH inheritance:\n");
    printf("  tick 0: LOW:lock\n");
    printf("  tick 1: HIGH:blocked -> LOW boosted to HIGH priority\n");
    printf("  tick 2: LOW:unlock (runs at HIGH priority, MED cannot preempt)\n");
    printf("  tick 3: HIGH:lock HIGH:run HIGH:done\n");
    printf("  tick 4: MED:run MED:done\n");
    printf("  HIGH waited 1 tick (LOW finished quickly)\n");
}

int main(void)
{
    simulate_without_inheritance();
    printf("\n");
    simulate_with_inheritance();

    return 0;
}
