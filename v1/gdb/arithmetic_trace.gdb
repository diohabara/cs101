# arithmetic_trace.gdb — add/sub の状態変化を観察
break _start
run
echo === Initial state ===\n
info registers rax rip
stepi
echo \n=== After mov rax, 10 ===\n
info registers rax rip
stepi
echo \n=== After add rax, 5 (should be 15) ===\n
info registers rax rip eflags
stepi
echo \n=== After sub rax, 2 (should be 13) ===\n
info registers rax rip eflags
continue
quit
