; two_mappings.asm — 独立した 2 つのマッピング
;
; 2 つの独立したページを mmap で確保し、
; それぞれに異なる値を書き込んで互いに干渉しないことを確認する。

section .text
global _start

_start:
    ; 1st mapping
    mov rax, 9
    xor rdi, rdi
    mov rsi, 4096
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    syscall
    mov r12, rax        ; r12 = page1

    ; 2nd mapping
    mov rax, 9
    xor rdi, rdi
    mov rsi, 4096
    mov rdx, 3
    mov r10, 0x22
    mov r8, -1
    xor r9, r9
    syscall
    mov r13, rax        ; r13 = page2

    ; 異なる値を書き込む
    mov qword [r12], 11  ; page1 = 11
    mov qword [r13], 22  ; page2 = 22

    ; page1 を読んで exit code にする (should be 11, not 22)
    mov rdi, [r12]
    mov rax, 60
    syscall
