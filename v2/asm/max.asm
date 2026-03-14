; max.asm — 2 値の最大値
; rax=15, rbx=27 の大きい方を exit code として返す。

section .text
global _start

_start:
    mov rax, 15
    mov rbx, 27
    cmp rax, rbx
    jg .rax_greater    ; rax > rbx ならジャンプ
    ; rbx >= rax
    mov rdi, rbx
    jmp .exit
.rax_greater:
    mov rdi, rax
.exit:
    mov rax, 60
    syscall
