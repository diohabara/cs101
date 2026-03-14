# max_trace.gdb — max の条件分岐観察
break _start
run
echo === Initial state ===\n
info registers rax rbx rip
stepi 2
echo \n=== After loading values ===\n
info registers rax rbx rip
stepi
echo \n=== After cmp (check eflags) ===\n
info registers rax rbx rip eflags
stepi
echo \n=== After conditional jump ===\n
info registers rdi rip
continue
quit
