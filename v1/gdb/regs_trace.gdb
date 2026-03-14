# regs_trace.gdb — regs プログラムの逐次実行トレース
# 各 mov 命令の実行後にレジスタの変化を観察する。

break _start
run

echo === Step 0: before execution ===\n
info registers rax rbx rcx rdx rip

echo \n=== Step 1: mov rax, 1 ===\n
stepi
info registers rax rbx rcx rdx rip

echo \n=== Step 2: mov rbx, 2 ===\n
stepi
info registers rax rbx rcx rdx rip

echo \n=== Step 3: mov rcx, 3 ===\n
stepi
info registers rax rbx rcx rdx rip

echo \n=== Step 4: mov rdx, 4 ===\n
stepi
info registers rax rbx rcx rdx rip

continue
quit
