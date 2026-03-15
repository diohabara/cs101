/*
 * rt_sched.h -- リアルタイムスケジューリング共通定義
 *
 * EDF スケジューラと優先度逆転シミュレーションで共有するデータ構造。
 * タスクの deadline, priority, 実行時間をまとめた Task 構造体と、
 * 優先度レベルの enum を定義する。
 */

#ifndef RT_SCHED_H
#define RT_SCHED_H

#define MAX_TASKS 16

/* 優先度レベル（数字が大きいほど高優先度） */
typedef enum {
    PRIO_LOW  = 1,
    PRIO_MED  = 2,
    PRIO_HIGH = 3
} Priority;

/* リアルタイムタスク */
typedef struct {
    const char *name;       /* タスク名 ("A", "B" など) */
    int         deadline;   /* 絶対デッドライン (tick) */
    int         exec_time;  /* 実行に必要な tick 数 */
    Priority    priority;   /* 静的優先度 */
    Priority    effective;  /* 実効優先度 (inheritance で変わる) */
    int         completed;  /* 完了 tick */
} Task;

#endif /* RT_SCHED_H */
