; push_pop.asm — push/pop の基本動作
; push でスタックに値を積み、pop で取り出す。
; LIFO（後入れ先出し）の動作を確認する。

section .text
global _start

_start:
    mov rax, 10
    mov rbx, 20
    push rax           ; スタック: [10]
    push rbx           ; スタック: [10, 20]
    pop rcx            ; rcx = 20, スタック: [10]
    pop rdx            ; rdx = 10, スタック: []
    ; exit with rcx (should be 20, LIFO)
    mov rdi, rcx
    mov rax, 60
    syscall
