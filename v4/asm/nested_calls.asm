; nested_calls.asm — ネストした関数呼び出し
; double_add_five → add_five を 2 回呼ぶ。
; スタックに戻りアドレスが積み重なる様子を観察する。

section .text
global _start

_start:
    mov rdi, 3
    call double_add_five  ; 3 + 5 + 5 = 13
    mov rax, 60
    syscall

double_add_five:
    call add_five      ; rdi += 5
    call add_five      ; rdi += 5
    ret

add_five:
    add rdi, 5
    ret
