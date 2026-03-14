# exit42_trace.gdb — exit42 の逐次実行トレース
# book.md に埋め込む GDB セッション出力を再現するためのスクリプト。

break _start
run

echo === Step 0: before execution ===\n
info registers rax rdi rip

echo \n=== Step 1: mov rax, 60 ===\n
stepi
info registers rax rdi rip

echo \n=== Step 2: mov rdi, 42 ===\n
stepi
info registers rax rdi rip

echo \n=== Step 3: syscall (exit) ===\n
continue

quit
