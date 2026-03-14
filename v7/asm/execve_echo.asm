; execve_echo.asm — fork 後に子で execve を実行する
; 子: /bin/echo "hello from execve" を実行
; 親: 子の終了を wait し、exit code 0 で終了

section .data
    ; execve に渡す引数
    prog:    db "/bin/echo", 0
    arg1:    db "hello from execve", 0

section .bss
    ; argv 配列: [&prog, &arg1, NULL]
    argv:    resq 3

section .text
global _start

_start:
    ; argv 配列を構築
    lea rax, [prog]
    mov [argv], rax            ; argv[0] = &prog
    lea rax, [arg1]
    mov [argv + 8], rax        ; argv[1] = &arg1
    mov qword [argv + 16], 0   ; argv[2] = NULL

    ; sys_fork()
    mov rax, 57
    syscall

    cmp rax, 0
    jz .child

.parent:
    ; sys_wait4(pid, NULL, 0, NULL)
    mov rdi, rax           ; pid = 子の PID
    mov rax, 61            ; sys_wait4
    mov rsi, 0
    mov rdx, 0
    mov r10, 0
    syscall

    ; exit(0)
    mov rax, 60
    mov rdi, 0
    syscall

.child:
    ; sys_execve(prog, argv, envp=NULL)
    mov rax, 59            ; sys_execve
    lea rdi, [prog]        ; pathname
    lea rsi, [argv]        ; argv
    mov rdx, 0             ; envp = NULL
    syscall

    ; execve が成功すればここには到達しない
    ; 失敗した場合は exit(1)
    mov rax, 60
    mov rdi, 1
    syscall
