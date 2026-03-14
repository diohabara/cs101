# call_trace.gdb — call_ret の関数呼び出し観察
break _start
run
echo === Before call ===\n
info registers rdi rsp rip
stepi
echo \n=== After mov rdi, 10 ===\n
info registers rdi rsp rip
echo \n=== After call add_five (rip should jump, rsp decreased) ===\n
stepi
info registers rdi rsp rip
x/1xg $rsp
echo \n=== After add rdi, 5 ===\n
stepi
info registers rdi rsp rip
echo \n=== After ret (rip returns, rsp restored) ===\n
stepi
info registers rdi rsp rip
continue
quit
