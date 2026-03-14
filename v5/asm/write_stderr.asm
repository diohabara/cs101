; write_stderr.asm — stderr にメッセージを出力する
; fd=1 (stdout) と fd=2 (stderr) の違いを体験する。
; exit code 1 で終了。

section .data
    errmsg: db "Error: something went wrong", 10
    errlen: equ $ - errmsg

section .text
global _start

_start:
    ; sys_write(stderr, errmsg, errlen)
    mov rax, 1             ; sys_write
    mov rdi, 2             ; fd = stderr
    lea rsi, [errmsg]
    mov rdx, errlen
    syscall

    ; sys_exit(1)
    mov rax, 60
    mov rdi, 1
    syscall
