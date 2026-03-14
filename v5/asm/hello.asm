; hello.asm — sys_write で "Hello, world!\n" を stdout に出力する
; sys_write(1, msg, len) → rax=1, rdi=1, rsi=msg, rdx=len
; その後 sys_exit(0) で正常終了。

section .data
    msg: db "Hello, world!", 10   ; 10 = '\n'
    len: equ $ - msg              ; 文字列の長さを自動計算

section .text
global _start

_start:
    ; sys_write(stdout, msg, len)
    mov rax, 1             ; sys_write
    mov rdi, 1             ; fd = stdout
    lea rsi, [msg]         ; buf = msg のアドレス
    mov rdx, len           ; count = 文字列の長さ
    syscall

    ; sys_exit(0)
    mov rax, 60
    mov rdi, 0
    syscall
