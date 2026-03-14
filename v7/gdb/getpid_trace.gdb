# getpid_trace.gdb — sys_getpid の戻り値を観察
break _start
run
echo === Before sys_getpid ===\n
info registers rax rip
stepi
echo \n=== After mov rax, 39 ===\n
info registers rax
stepi
echo \n=== After sys_getpid: rax = PID ===\n
info registers rax
continue
quit
