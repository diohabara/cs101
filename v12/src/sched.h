/*
 * sched.h — スケジューラ共通定義
 *
 * ucontext API によるユーザー空間コンテキストスイッチの基盤。
 * タスク構造体とスケジューラインターフェースを定義する。
 *
 * ucontext_t はレジスタセット＋スタックポインタの塊。
 * swapcontext() で「現在の CPU 状態を保存し、別の状態に復元する」。
 * これは v4 で学んだ push/pop（レジスタ退避・復元）の究極形。
 *
 *   v4 の push/pop:        1 レジスタずつ退避
 *   ucontext swapcontext:  全レジスタ＋スタックを一括退避・復元
 */

#ifndef SCHED_H
#define SCHED_H

#include <ucontext.h>

/* タスク用スタックサイズ: 64KB */
#define STACK_SIZE (64 * 1024)

/* タスクの最大数 */
#define MAX_TASKS 8

/* タスクの状態 */
typedef enum {
    TASK_READY,     /* 実行可能 */
    TASK_FINISHED   /* 完了済み */
} task_state_t;

/* タスク構造体 */
typedef struct {
    ucontext_t ctx;             /* コンテキスト（レジスタ＋スタック情報） */
    char       stack[STACK_SIZE]; /* タスク専用スタック */
    const char *name;           /* タスク名（表示用） */
    int        priority;        /* 優先度（大きいほど高優先） */
    task_state_t state;         /* 現在の状態 */
} task_t;

#endif /* SCHED_H */
