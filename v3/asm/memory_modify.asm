; memory_modify.asm — メモリの値を変更する
; メモリに値を書き、読み出し、加算して書き戻す。

section .data
    counter: dq 5

section .text
global _start

_start:
    mov rax, [counter]     ; rax = 5
    add rax, 3             ; rax = 8
    mov [counter], rax     ; counter = 8
    mov rbx, [counter]     ; rbx = 8 (確認)
    mov rdi, rbx
    mov rax, 60
    syscall
