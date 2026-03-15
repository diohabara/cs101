/*
 * process.h — プロセス制御ブロック（PCB）の定義
 *
 * OS がプロセスを管理するための基本データ構造。
 * 各プロセスは PCB を持ち、OS はこの構造体を通じて
 * プロセスの状態を追跡・制御する。
 *
 * 5 状態モデル:
 *   NEW → READY → RUNNING → TERMINATED
 *                    ↓  ↑
 *                  BLOCKED
 *
 * 状態遷移の契機:
 *   NEW → READY:       プロセス生成完了（fork 後）
 *   READY → RUNNING:   スケジューラがディスパッチ
 *   RUNNING → READY:   タイムスライス消費（プリエンプション）
 *   RUNNING → BLOCKED: I/O 待ちや wait() 呼び出し
 *   BLOCKED → READY:   I/O 完了や子プロセス終了
 *   RUNNING → TERMINATED: exit() や return
 */

#ifndef PROCESS_H
#define PROCESS_H

/* プロセスの状態（5 状態モデル）
 * 教科書では NEW を含めて 5 状態とするが、
 * ここでは生成直後に即 READY になるモデルとし、
 * 実行時に観測される 4 状態を定義する。 */
typedef enum {
    READY      = 0,  /* 実行可能。CPU 割り当て待ち */
    RUNNING    = 1,  /* CPU 上で実行中 */
    BLOCKED    = 2,  /* I/O 完了やイベント待ち */
    TERMINATED = 3   /* 実行完了。資源回収待ち */
} ProcessState;

/* 状態名の文字列表現（表示用） */
static const char *state_name[] = {
    "READY",
    "RUNNING",
    "BLOCKED",
    "TERMINATED"
};

/* プロセス制御ブロック（PCB）
 *
 * 実際の OS では PCB にはさらに多くのフィールドがある:
 *   - レジスタ保存領域（コンテキスト）
 *   - メモリマップ情報
 *   - 開いているファイルのリスト
 *   - スケジューリング優先度
 * ここでは最小限のフィールドで概念を示す。 */
typedef struct {
    int          pid;    /* プロセス ID */
    ProcessState state;  /* 現在の状態 */
    const char  *name;   /* プロセス名（デバッグ用） */
} PCB;

#endif /* PROCESS_H */
