; fd_table.asm — open すると fd=3 が返ることを確認する
; stdin=0, stdout=1, stderr=2 が既に使われているので、
; 新しく開いたファイルの fd は 3 になる。

section .data
    path: db "/tmp/v6_fd_test.txt", 0

section .text
global _start

_start:
    ; sys_open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644)
    mov rax, 2             ; sys_open
    lea rdi, [path]
    mov rsi, 577           ; O_WRONLY | O_CREAT | O_TRUNC
    mov rdx, 0644o
    syscall
    ; rax = 3 (最初に開いたファイルの fd)

    ; fd を保存して close
    mov rbx, rax           ; rbx = fd (3)
    mov rdi, rax
    mov rax, 3             ; sys_close
    syscall

    ; cleanup: ファイルを削除
    mov rax, 87            ; sys_unlink
    lea rdi, [path]
    syscall

    ; exit with fd value (should be 3)
    mov rdi, rbx
    mov rax, 60
    syscall
