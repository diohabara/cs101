# hello_trace.gdb — sys_write の呼び出し規約を観察
break _start
run
echo === Initial state ===\n
info registers rax rdi rsi rdx rip
stepi 4
echo \n=== Before syscall (sys_write) ===\n
echo rax=1 (sys_write), rdi=1 (stdout), rsi=msg, rdx=len\n
info registers rax rdi rsi rdx rip
stepi
echo \n=== After sys_write (rax = bytes written) ===\n
info registers rax rip
continue
quit
