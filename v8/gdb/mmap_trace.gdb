# mmap_trace.gdb — mmap_write の逐次実行トレース
break _start
run
echo === Before mmap ===\n
info registers rax rip
# Skip past mmap syscall setup (7 instructions) + syscall
stepi 8
echo \n=== After mmap (rax = allocated address) ===\n
info registers rax rip
echo \n=== After write to page ===\n
stepi
x/1xg $rax
echo \n=== After read from page ===\n
stepi
info registers rbx
continue
quit
