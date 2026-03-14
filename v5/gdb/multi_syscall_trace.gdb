# multi_syscall_trace.gdb — rax が syscall 戻り値で上書きされることを観察
break _start
run
echo === Before sys_write ===\n
info registers rax rdi rip
stepi 4
echo \n=== Just before sys_write syscall ===\n
info registers rax rdi rsi rdx
stepi
echo \n=== After sys_write: rax is now the return value ===\n
info registers rax rip
echo Note: rax was 1 (sys_write number), now it is the number of bytes written\n
stepi 2
echo \n=== Before sys_exit syscall ===\n
info registers rax rdi
continue
quit
