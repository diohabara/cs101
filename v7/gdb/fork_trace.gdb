# fork_trace.gdb — fork の戻り値を観察
# 注意: GDB では fork 後のデバッグには set follow-fork-mode が必要
break _start
run
echo === Before fork ===\n
info registers rax rip
# Step to fork syscall
stepi 2
echo \n=== Just before sys_fork ===\n
info registers rax
stepi
echo \n=== After fork: rax = child PID (parent) or 0 (child) ===\n
info registers rax
continue
quit
