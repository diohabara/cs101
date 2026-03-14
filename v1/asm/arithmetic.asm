; arithmetic.asm — add と sub を 1 つのプログラムで体験する
; mov rax, 10 → add rax, 5 → sub rax, 2 → exit code 13

section .text
global _start

_start:
    mov rax, 10
    add rax, 5         ; rax = 15
    sub rax, 2         ; rax = 13
    ; exit with rax
    mov rdi, rax
    mov rax, 60
    syscall
