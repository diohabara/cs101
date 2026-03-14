; fork_basic.asm — fork でプロセスを複製する
; 親: 子の終了を wait し、exit code 42 で終了
; 子: stdout に "child\n" を出力して exit 0
; 親: stdout に "parent\n" を出力して exit 42

section .data
    child_msg:  db "child", 10
    child_len:  equ $ - child_msg
    parent_msg: db "parent", 10
    parent_len: equ $ - parent_msg

section .text
global _start

_start:
    ; sys_fork()
    mov rax, 57            ; sys_fork
    syscall
    ; rax = 0 (子プロセス) or 子の PID (親プロセス)

    cmp rax, 0
    jz .child              ; rax=0 なら子プロセス

.parent:
    ; 子プロセスの PID を保存
    mov rbx, rax

    ; sys_wait4(pid, NULL, 0, NULL)
    mov rax, 61            ; sys_wait4
    mov rdi, rbx           ; pid = 子の PID
    mov rsi, 0             ; status = NULL
    mov rdx, 0             ; options = 0
    mov r10, 0             ; rusage = NULL
    syscall

    ; stdout に "parent\n" を出力
    mov rax, 1
    mov rdi, 1
    lea rsi, [parent_msg]
    mov rdx, parent_len
    syscall

    ; exit(42)
    mov rax, 60
    mov rdi, 42
    syscall

.child:
    ; stdout に "child\n" を出力
    mov rax, 1
    mov rdi, 1
    lea rsi, [child_msg]
    mov rdx, child_len
    syscall

    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall
