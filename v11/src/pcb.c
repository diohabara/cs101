/*
 * pcb.c — PCB と状態遷移のデモンストレーション
 *
 * プロセス制御ブロック（PCB）を作成し、典型的な状態遷移を
 * シミュレートする。OS カーネルがプロセスの状態をどのように
 * 追跡するかを示す。
 *
 * 状態遷移シナリオ:
 *   READY → RUNNING:    スケジューラがプロセスを選択
 *   RUNNING → BLOCKED:  プロセスが I/O を要求
 *   BLOCKED → RUNNING:  I/O 完了、再びディスパッチ
 *   RUNNING → TERMINATED: プロセスが exit()
 */

#include <stdio.h>
#include <stdlib.h>
#include "process.h"

/*
 * transition — PCB の状態を遷移させ、遷移を表示する
 *
 * OS カーネルでは、状態遷移のたびに PCB を更新し、
 * 対応するキュー（Ready キュー、Wait キュー等）間で
 * PCB を移動させる。
 */
static void transition(PCB *pcb, ProcessState new_state) {
    ProcessState old_state = pcb->state;
    pcb->state = new_state;
    /* 遷移ログ: "PID 1: READY -> RUNNING" の形式 */
    printf("PID %d: %s -> %s\n",
           pcb->pid, state_name[old_state], state_name[new_state]);
}

int main(void) {
    /* PCB を初期化（生成直後は READY 状態） */
    PCB proc = {
        .pid   = 1,
        .state = READY,
        .name  = "demo_process"
    };

    /* 典型的なライフサイクルをシミュレート */

    /* 1. スケジューラがプロセスを選択 → 実行開始 */
    transition(&proc, RUNNING);

    /* 2. プロセスが I/O を要求 → ブロック状態へ */
    transition(&proc, BLOCKED);

    /* 3. I/O 完了 → 再び実行可能に（ここではすぐ RUNNING に遷移） */
    transition(&proc, RUNNING);

    /* 4. プロセスが exit() → 終了 */
    transition(&proc, TERMINATED);

    return 0;
}
