# mprotect_trace.gdb — mprotect_crash の SIGSEGV 観察
break _start
run
# Skip past mmap (8 instr) + write (1 instr) + mprotect setup (4 instr) + syscall (1 instr)
stepi 14
echo === After mprotect (page is now PROT_NONE) ===\n
info registers r12 rip
echo \n=== Attempting read from protected page... ===\n
stepi
echo \n=== Should have received SIGSEGV ===\n
info proc mappings
continue
quit
