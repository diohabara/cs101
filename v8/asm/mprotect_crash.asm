; mprotect_crash.asm — mprotect で権限を変更し SIGSEGV を発生させる
;
; 1. mmap で読み書き可能なページを確保
; 2. 値を書き込む（成功する）
; 3. mprotect で PROT_NONE（アクセス不可）に変更
; 4. 読み出そうとする → SIGSEGV (exit code 139 = 128 + 11)

section .text
global _start

_start:
    ; mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
    mov rax, 9
    xor rdi, rdi
    mov rsi, 4096
    mov rdx, 3          ; PROT_READ | PROT_WRITE
    mov r10, 0x22       ; MAP_PRIVATE | MAP_ANONYMOUS
    mov r8, -1
    xor r9, r9
    syscall
    mov r12, rax        ; r12 = ページアドレス

    ; 書き込み（成功する）
    mov qword [r12], 99

    ; mprotect(page, 4096, PROT_NONE) — アクセス権を剥奪
    mov rax, 10         ; sys_mprotect
    mov rdi, r12        ; addr
    mov rsi, 4096       ; len
    xor rdx, rdx        ; prot = PROT_NONE (0)
    syscall

    ; 読み出そうとする → SIGSEGV
    mov rbx, [r12]      ; ← ここでクラッシュ

    ; ここには到達しない
    mov rdi, 0
    mov rax, 60
    syscall
