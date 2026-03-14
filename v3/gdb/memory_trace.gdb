# memory_trace.gdb — store_load の逐次実行トレース
break _start
run
echo === Step 0: before execution ===\n
info registers rax rbx rip
echo \n=== Step 1: mov rax, 42 ===\n
stepi
info registers rax rbx rip
echo \n=== Step 2: mov [value], rax (store) ===\n
stepi
x/1xg &value
echo \n=== Step 3: mov rbx, 0 ===\n
stepi
info registers rax rbx rip
echo \n=== Step 4: mov rbx, [value] (load) ===\n
stepi
info registers rax rbx rip
continue
quit
