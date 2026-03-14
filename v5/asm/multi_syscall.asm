; multi_syscall.asm — 複数の syscall を連続呼び出しする
; sys_write で "7" を stdout に出力し、sys_exit(7) で終了。
; rax が syscall の戻り値で上書きされることを示す。

section .data
    digit: db "7"
    digitlen: equ $ - digit

section .text
global _start

_start:
    ; sys_write(stdout, digit, 1)
    mov rax, 1             ; sys_write
    mov rdi, 1             ; fd = stdout
    lea rsi, [digit]
    mov rdx, digitlen
    syscall
    ; ここで rax = 書き込んだバイト数（1）に上書きされている
    ; rax はもう 1 (sys_write) ではない！

    ; sys_exit(7)
    mov rax, 60            ; sys_exit
    mov rdi, 7
    syscall
