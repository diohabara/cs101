/*
 * fork_exec.c — fork/exec/wait を C で表現
 *
 * UNIX のプロセス生成モデル:
 *   1. fork()  — 現在のプロセスを複製（PCB をコピー）
 *   2. exec()  — 子プロセスのメモリイメージを新プログラムで置換
 *   3. wait()  — 親が子の終了を待つ
 *
 * fork() の戻り値が親子で異なる理由:
 *   fork() は PCB を含むプロセス全体を複製する。
 *   複製後、カーネルは子の PCB には戻り値 0 を、
 *   親の PCB には子の PID を設定する。
 *   つまり「同じ関数呼び出しが 2 回返る」のではなく、
 *   「複製された 2 つのプロセスがそれぞれ異なる値で再開する」。
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    /*
     * fork() — プロセスの複製
     *
     * カーネルの動作:
     *   1. 親の PCB をコピーして子の PCB を作成
     *   2. 親のメモリ空間を子にコピー（実際は CoW で遅延）
     *   3. 子の PCB を Ready キューに追加
     *   4. 親には子の PID を、子には 0 を返す
     */
    pid_t pid = fork();

    if (pid < 0) {
        /* fork 失敗（PCB 割り当て不可など） */
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /*
         * 子プロセス（fork() が 0 を返した側）
         *
         * execve() — プロセスイメージの置換
         *   PCB（PID, 開いているファイル等）はそのままに、
         *   テキスト/データ/スタックセグメントを新しいプログラムで置換する。
         *   成功すると返らない（新プログラムの _start に飛ぶ）。
         */
        char *argv[] = { "echo", "hello from child", NULL };
        char *envp[] = { NULL };

        execve("/bin/echo", argv, envp);

        /* execve() が返った場合はエラー */
        perror("execve");
        _exit(127);
    }

    /*
     * 親プロセス（fork() が子の PID を返した側）
     *
     * waitpid() — 子の終了を待つ
     *   親の状態: RUNNING → BLOCKED（子が終了するまで）
     *   子が終了すると: BLOCKED → READY → RUNNING
     */
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        /* 子が正常終了した場合、exit code を表示 */
        printf("parent: child exited with %d\n", WEXITSTATUS(status));
    }

    return 0;
}
