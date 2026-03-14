# flags_trace.gdb — ZF の変化を観察
break _start
run
echo === Initial state ===\n
info registers rax rip eflags
stepi
echo \n=== After mov rax, 7 ===\n
info registers rax rip eflags
stepi
echo \n=== After sub rax, 7 (ZF should be set) ===\n
info registers rax rip eflags
stepi
echo \n=== After jz (should jump to .zero) ===\n
info registers rdi rip
continue
quit
