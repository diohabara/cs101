; write_stderr.asm — stderr にメッセージを出力する
; rdi=2 にすることで stdout ではなく stderr に書き込む。
; fd の違いは v6 で詳しく学ぶ。exit code 1 で終了。

section .data
    errmsg: db "This message goes to stderr (fd=2)", 10
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
