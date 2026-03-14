; flags.asm — RFLAGS の ZF（ゼロフラグ）を観察する
; sub rax, rax で ZF=1 → jz で分岐 → exit code 0

section .text
global _start

_start:
    mov rax, 7
    sub rax, 7         ; rax = 0, ZF=1
    jz .zero           ; ZF=1 なら .zero へ
    ; ここには来ないはず
    mov rdi, 1
    jmp .exit
.zero:
    mov rdi, 0         ; ZF=1 → exit code 0
.exit:
    mov rax, 60
    syscall
