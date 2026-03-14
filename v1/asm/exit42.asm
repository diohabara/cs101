; exit42.asm — exit code 42 で終了する最小プログラム
;
; sys_exit(42) を呼ぶだけ。CPU が命令を順に実行し、
; syscall でカーネルに制御を渡す様子を観察する。
; `.text` は機械語の命令を置くセクション。
; `_start` は OS から実行開始される入口ラベルで、`global` でリンカに公開する。

section .text
global _start

_start:
    mov rax, 60         ; sys_exit のシステムコール番号
    mov rdi, 42         ; 終了コード
    syscall             ; カーネルを呼び出す
