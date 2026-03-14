# countdown_trace.gdb — countdown のループ観察
break _start
run
echo === Initial state ===\n
info registers rcx rip eflags
# 各ループイテレーションを観察
stepi 2
echo \n=== After first iteration (rcx should be 4) ===\n
info registers rcx rip eflags
stepi 2
echo \n=== After second iteration (rcx should be 3) ===\n
info registers rcx rip eflags
continue
quit
