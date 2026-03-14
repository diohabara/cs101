# stack_trace.gdb — push_pop のスタック観察
break _start
run
echo === Initial state ===\n
info registers rsp rip
echo \n=== After mov rax, 10 ===\n
stepi
info registers rax rsp
echo \n=== After mov rbx, 20 ===\n
stepi
info registers rbx rsp
echo \n=== After push rax (rsp should decrease by 8) ===\n
stepi
info registers rsp
x/1xg $rsp
echo \n=== After push rbx ===\n
stepi
info registers rsp
x/2xg $rsp
echo \n=== After pop rcx (should get 20) ===\n
stepi
info registers rcx rsp
echo \n=== After pop rdx (should get 10) ===\n
stepi
info registers rdx rsp
continue
quit
