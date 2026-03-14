; read_stdin.asm — stdin から読み取ってエコーバックする
; sys_read(0, buf, 16) → sys_write(1, buf, n)
; .bss セクションで未初期化バッファを確保する。

section .bss
    buf: resb 16               ; 16 バイトのバッファ（未初期化）

section .text
global _start

_start:
    ; sys_read(stdin, buf, 16)
    mov rax, 0             ; sys_read
    mov rdi, 0             ; fd = stdin
    lea rsi, [buf]         ; buf
    mov rdx, 16            ; count
    syscall
    ; rax = 読み取ったバイト数

    ; sys_write(stdout, buf, n)
    mov rdx, rax           ; count = 読み取ったバイト数
    mov rax, 1             ; sys_write
    mov rdi, 1             ; fd = stdout
    lea rsi, [buf]
    syscall

    ; sys_exit(0)
    mov rax, 60
    mov rdi, 0
    syscall
