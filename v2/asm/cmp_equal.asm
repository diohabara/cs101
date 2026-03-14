; cmp_equal.asm — 等値比較
; 2 つの値が等しいとき ZF=1 になることを確認する。

section .text
global _start

_start:
    mov rax, 42
    cmp rax, 42       ; 42 - 42 = 0, ZF=1
    jz .equal         ; ZF=1 ならジャンプ
    ; ここには来ない
    mov rdi, 1
    jmp .exit
.equal:
    mov rdi, 0        ; equal → exit code 0
.exit:
    mov rax, 60
    syscall
