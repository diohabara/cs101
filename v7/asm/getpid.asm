; getpid.asm — プロセス ID を取得する
; sys_getpid(39) で現在のプロセスの PID を取得し、
; 下位 8 ビットを exit code として返す。

section .text
global _start

_start:
    ; sys_getpid()
    mov rax, 39            ; sys_getpid
    syscall
    ; rax = PID

    ; exit with PID & 0xFF
    mov rdi, rax           ; PID を exit code に（下位 8 ビットのみ有効）
    mov rax, 60
    syscall
