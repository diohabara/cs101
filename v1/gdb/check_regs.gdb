# check_regs.gdb — regs プログラムのレジスタ検証用
# mov rax,1 / mov rbx,2 / mov rcx,3 / mov rdx,4 の直後で停止し、
# exit 用の mov rax,60 の前にレジスタを確認する。

break _start
run

# 4 命令分進める (mov rax,1 / mov rbx,2 / mov rcx,3 / mov rdx,4)
stepi 4

print /d $rbx
print /d $rcx
print /d $rdx

quit
