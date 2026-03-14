; countdown.asm — ループ（カウントダウン）
; rcx を 5 から 0 までデクリメントする。
; ループの仕組みを学ぶ。

section .text
global _start

_start:
    mov rcx, 5        ; カウンタ = 5
.loop:
    sub rcx, 1        ; rcx--
    cmp rcx, 0
    jnz .loop         ; rcx != 0 なら繰り返す
    ; rcx = 0 で exit
    mov rdi, rcx
    mov rax, 60
    syscall
