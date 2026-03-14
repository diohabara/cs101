; write_file.asm — ファイルを作成して書き込む
; sys_open → sys_write → sys_close のライフサイクル。
; /tmp/v6_test.txt に "hello from asm\n" を書き込む。

section .data
    path: db "/tmp/v6_test.txt", 0     ; NULL 終端のファイルパス
    msg:  db "hello from asm", 10       ; 書き込む内容
    msglen: equ $ - msg

section .text
global _start

_start:
    ; sys_open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov rax, 2             ; sys_open
    lea rdi, [path]        ; pathname
    mov rsi, 577           ; O_WRONLY(1) | O_CREAT(64) | O_TRUNC(512) = 577
    mov rdx, 0644o         ; mode = rw-r--r--
    syscall
    ; rax = fd (should be 3)

    ; sys_write(fd, msg, msglen)
    mov rdi, rax           ; fd = 返されたファイルディスクリプタ
    mov rax, 1             ; sys_write
    lea rsi, [msg]
    mov rdx, msglen
    syscall

    ; sys_close(fd)
    mov rax, 3             ; sys_close
    ; rdi はまだ fd を保持している
    syscall

    ; sys_exit(0)
    mov rax, 60
    mov rdi, 0
    syscall
