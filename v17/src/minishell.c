/*
 * minishell.c --- パイプとリダイレクト付きミニシェル
 *
 * インタラクティブではなく、コマンドを argv で受け取る。
 * テストの決定性を確保するための設計。
 *
 *   ./minishell "echo hello"                      --- 単純実行
 *   ./minishell "echo hello | cat"                --- パイプ
 *   ./minishell "echo hello > /tmp/out.txt"       --- リダイレクト
 *
 * fd テーブルの操作:
 *   - パイプ: pipe() で fd ペアを作り、子プロセスの stdout/stdin を付け替え
 *   - リダイレクト: open() でファイルを開き、dup2() で stdout を付け替え
 *
 *   パイプ実行時の fd テーブル:
 *     左の子:                右の子:
 *     fd 0 → stdin          fd 0 → pipe read端
 *     fd 1 → pipe write端   fd 1 → stdout
 *
 *   リダイレクト実行時の fd テーブル:
 *     子プロセス:
 *     fd 0 → stdin
 *     fd 1 → ファイル（dup2 で stdout を差し替え）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64

/*
 * trim --- 文字列の先頭と末尾の空白を取り除く
 *
 * 先頭のスペース/タブをスキップし、末尾のスペース/タブを '\0' で上書きする。
 */
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return s;
}

/*
 * parse_args --- コマンド文字列をスペース区切りで argv 配列に分割する
 *
 * 例: "echo hello world" → {"echo", "hello", "world", NULL}
 * strtok はポインタを内部的に保持して文字列を破壊的に分割する。
 */
static int parse_args(char *cmd, char **argv, int max_args) {
    int argc = 0;
    char *tok = strtok(cmd, " \t");
    while (tok && argc < max_args - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    return argc;
}

/*
 * exec_simple --- 単純なコマンドを fork + exec で実行する
 *
 * 親は wait で子の終了を待つ。
 */
static void exec_simple(char *cmd) {
    char *argv[MAX_ARGS];
    char buf[512];
    strncpy(buf, cmd, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    parse_args(buf, argv, MAX_ARGS);

    if (argv[0] == NULL) return;

    pid_t pid = fork();
    if (pid == 0) {
        /* 子プロセス: exec でコマンドを実行 */
        execvp(argv[0], argv);
        perror("exec");
        _exit(127);
    }
    /* 親プロセス: 子の終了を待つ */
    waitpid(pid, NULL, 0);
}

/*
 * exec_redirect --- リダイレクト付きコマンドを実行する
 *
 * "echo hello > file.txt" を処理する。
 *
 * fd の付け替え:
 *   1. open() でファイルを開く → fd N を得る
 *   2. dup2(N, STDOUT_FILENO) で stdout をファイルに差し替え
 *   3. exec でコマンド実行 → 出力はファイルへ
 */
static void exec_redirect(char *left, char *right) {
    char *cmd = trim(left);
    char *file = trim(right);

    char *argv[MAX_ARGS];
    char buf[512];
    strncpy(buf, cmd, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    parse_args(buf, argv, MAX_ARGS);

    if (argv[0] == NULL) return;

    pid_t pid = fork();
    if (pid == 0) {
        /* ファイルを開く（なければ作成、あれば上書き） */
        int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            _exit(1);
        }
        /* stdout をファイルに差し替え */
        dup2(fd, STDOUT_FILENO);
        close(fd); /* dup2 後は元の fd は不要 */

        execvp(argv[0], argv);
        perror("exec");
        _exit(127);
    }
    waitpid(pid, NULL, 0);
}

/*
 * exec_pipe --- パイプ付きコマンドを実行する
 *
 * "echo hello | cat" を処理する。
 *
 * pipe() は fd ペアを作る:
 *   pipefd[0] = read 端（右の子の stdin へ）
 *   pipefd[1] = write 端（左の子の stdout へ）
 *
 * fd の付け替え:
 *   左の子: dup2(pipefd[1], STDOUT_FILENO) → 出力をパイプへ
 *   右の子: dup2(pipefd[0], STDIN_FILENO)  → 入力をパイプから
 */
static void exec_pipe(char *left, char *right) {
    char *cmd1 = trim(left);
    char *cmd2 = trim(right);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }

    /* 左の子: stdout → パイプの write 端 */
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]); /* read 端は使わない */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char *argv[MAX_ARGS];
        char buf[512];
        strncpy(buf, cmd1, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        parse_args(buf, argv, MAX_ARGS);
        execvp(argv[0], argv);
        perror("exec");
        _exit(127);
    }

    /* 右の子: stdin → パイプの read 端 */
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]); /* write 端は使わない */
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        char *argv[MAX_ARGS];
        char buf[512];
        strncpy(buf, cmd2, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        parse_args(buf, argv, MAX_ARGS);
        execvp(argv[0], argv);
        perror("exec");
        _exit(127);
    }

    /* 親: パイプの両端を閉じて、両方の子を待つ */
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/*
 * run_command --- コマンド文字列を解析して実行する
 *
 * パイプ (|) とリダイレクト (>) を検出し、
 * 適切な実行関数に振り分ける。
 */
static void run_command(char *cmdline) {
    /* パイプのチェック */
    char *pipe_pos = strchr(cmdline, '|');
    if (pipe_pos) {
        *pipe_pos = '\0';
        printf("pipe: %s | %s\n", trim(cmdline), trim(pipe_pos + 1));
        fflush(stdout);
        exec_pipe(cmdline, pipe_pos + 1);
        return;
    }

    /* リダイレクトのチェック */
    char *redir_pos = strchr(cmdline, '>');
    if (redir_pos) {
        *redir_pos = '\0';
        printf("redirect: %s > %s\n", trim(cmdline), trim(redir_pos + 1));
        fflush(stdout);
        exec_redirect(cmdline, redir_pos + 1);
        return;
    }

    /* 単純実行 */
    printf("exec: %s\n", trim(cmdline));
    fflush(stdout);
    exec_simple(cmdline);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s \"command [| command] [> file]\"\n",
                argv[0]);
        return 1;
    }

    /* 各引数を独立したコマンドとして実行 */
    for (int i = 1; i < argc; i++) {
        char buf[1024];
        strncpy(buf, argv[i], sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        run_command(buf);
    }

    return 0;
}
