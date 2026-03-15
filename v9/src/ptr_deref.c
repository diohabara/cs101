/*
 * ptr_deref.c — ポインタの間接参照
 *
 * v1 の `mov rax, 42` + `mov [addr], rax` に相当する操作を C で表現。
 * ポインタ経由で値を書き込み・読み出し、exit code で検証する。
 *
 * アセンブリとの対応:
 *   int x = 0;       → section .bss / x: resq 1
 *   int *ptr = &x;   → lea rax, [x]
 *   *ptr = 42;       → mov qword [rax], 42
 *   return *ptr;     → mov rdi, [rax] / exit
 */

#include <stdlib.h>

int main(void) {
    int x = 0;
    int *ptr = &x;

    /* mov qword [rax], 42 と同じ */
    *ptr = 42;

    /* exit code で値を返す（v1 の exit42.asm と同じパターン） */
    return *ptr;
}
