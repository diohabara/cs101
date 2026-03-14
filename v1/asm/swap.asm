; swap.asm — 2 つのレジスタの値を交換する
; 直接交換する命令はないので、一時レジスタ (rcx) を使う。

section .text
global _start

_start:
    mov rax, 5
    mov rbx, 10
    ; swap rax and rbx using rcx as temp
    mov rcx, rax      ; rcx = 5 (temp)
    mov rax, rbx      ; rax = 10
    mov rbx, rcx      ; rbx = 5
    ; exit with rbx value (should be 5, original rax)
    mov rdi, rbx
    mov rax, 60
    syscall
