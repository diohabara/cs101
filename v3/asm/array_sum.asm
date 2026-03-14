; array_sum.asm — 配列の合計
; .data セクションに配列を定義し、全要素を合計する。

section .data
    array: dq 10, 20, 30, 40   ; 4 要素の配列
    len:   equ 4                ; 要素数

section .text
global _start

_start:
    xor rax, rax           ; rax = 0 (合計)
    xor rcx, rcx           ; rcx = 0 (インデックス)
    lea rsi, [array]       ; rsi = 配列の先頭アドレス
.loop:
    cmp rcx, len
    jge .done
    add rax, [rsi + rcx*8] ; rax += array[rcx]
    inc rcx
    jmp .loop
.done:
    ; exit with sum (10+20+30+40 = 100)
    mov rdi, rax
    mov rax, 60
    syscall
