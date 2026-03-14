; mmap_write.asm — mmap でページを確保し、書き込んで読み出す
;
; mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
; → 確保したページに 42 を書き込み → 読み出して exit code にする

section .text
global _start

_start:
    ; mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
    mov rax, 9          ; sys_mmap
    xor rdi, rdi        ; addr = NULL (カーネルに任せる)
    mov rsi, 4096       ; len = 1 page
    mov rdx, 3          ; prot = PROT_READ | PROT_WRITE
    mov r10, 0x22       ; flags = MAP_PRIVATE | MAP_ANONYMOUS
    mov r8, -1          ; fd = -1 (anonymous)
    xor r9, r9          ; offset = 0
    syscall             ; rax = 確保されたアドレス

    ; 確保したページに値を書き込む
    mov qword [rax], 42 ; *page = 42

    ; 読み出す
    mov rbx, [rax]      ; rbx = 42

    ; exit with the value
    mov rdi, rbx
    mov rax, 60
    syscall
