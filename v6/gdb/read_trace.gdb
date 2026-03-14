# read_trace.gdb — sys_read の呼び出し観察
break _start
run
echo === Before sys_read ===\n
info registers rax rdi rsi rdx rip
stepi 4
echo \n=== Just before sys_read syscall ===\n
echo rax=0 (sys_read), rdi=0 (stdin), rsi=buf, rdx=16\n
info registers rax rdi rsi rdx
continue
quit
