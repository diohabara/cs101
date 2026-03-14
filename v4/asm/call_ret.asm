; call_ret.asm — 関数呼び出しの基本
; call = push rip + jmp、ret = pop rip
; 関数 add_five が rdi に 5 を足して返す。

section .text
global _start

_start:
    mov rdi, 10
    call add_five      ; add_five(10) → rdi=15
    ; rdi = 15
    mov rax, 60
    syscall

add_five:
    add rdi, 5
    ret
