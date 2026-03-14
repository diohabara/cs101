; store_load.asm — メモリへの書き込みと読み出し
; .data セクションに変数を定義し、レジスタとの間でデータを移動する。

section .data
    value: dq 0            ; 8 バイトの変数（初期値 0）

section .text
global _start

_start:
    mov rax, 42
    mov [value], rax       ; メモリに書き込み (store)
    mov rbx, 0             ; rbx をクリア
    mov rbx, [value]       ; メモリから読み出し (load)
    ; exit with rbx (should be 42)
    mov rdi, rbx
    mov rax, 60
    syscall
