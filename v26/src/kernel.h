/*
 * kernel.h --- ミニ OS カーネル（ヘッダ）
 *
 * 協調的マルチタスクのカーネル。
 * 複数のタスクを作成し、ラウンドロビン方式で順番に実行する。
 *
 * タスク状態:
 *   READY:   実行可能（スケジューラが選択可能）
 *   RUNNING: 現在実行中
 *   BLOCKED: イベント待ち（将来の拡張用）
 *   DONE:    実行完了
 *
 * v26 では協調的スケジューリング（タスクが自発的に制御を返す）を使う。
 * プリエンプティブスケジューリング（タイマー割り込みで強制切り替え）は
 * より複雑なスタック切り替えが必要なため、概念の説明にとどめる。
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

/* タスクの最大数 */
#define MAX_TASKS 4

/* 各タスクのスタックサイズ */
#define TASK_STACK_SIZE 4096

/* タスクの状態 */
enum task_state {
    TASK_READY,     /* 実行可能 */
    TASK_RUNNING,   /* 現在実行中 */
    TASK_BLOCKED,   /* ブロック（将来の拡張用） */
    TASK_DONE       /* 実行完了 */
};

/* タスク制御ブロック (TCB) */
struct task {
    enum task_state state;    /* 現在の状態 */
    uint32_t id;              /* タスク ID */
    const char *name;         /* タスク名（デバッグ用） */
    void (*entry)(void);      /* エントリポイント関数 */
    int iterations;           /* 残り実行回数 */
};

/* カーネルメインループ */
void kernel_main(void);

/* タスクを登録する（task.c で定義されたタスク関数を使う） */
void kernel_add_task(const char *name, void (*entry)(void), int iterations);

#endif /* KERNEL_H */
