; regs.asm — 複数レジスタに値をロードする
;
; mov 命令でレジスタに即値を書き込み、
; 合計値を exit code として返すことで正しさを検証する。
; GDB で 1 命令ずつレジスタの変化を確認する。

section .text
global _start

_start:
    mov rax, 1
    mov rbx, 2
    mov rcx, 3
    mov rdx, 4
    ; 合計 = 1+2+3+4 = 10 を exit code にする
    add rax, rbx
    add rax, rcx
    add rax, rdx       ; rax = 10
    mov rdi, rax
    mov rax, 60
    syscall
