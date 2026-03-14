# fd_trace.gdb — ファイルオープンと fd 番号の観察
break _start
run
echo === Before sys_open ===\n
info registers rax rdi rip
# Step through to syscall
stepi 4
echo \n=== Just before sys_open syscall ===\n
info registers rax rdi rsi rdx
stepi
echo \n=== After sys_open: rax should be 3 (first available fd) ===\n
info registers rax rip
continue
quit
