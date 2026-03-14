# Version Map

## CPU Foundations (x86-64 Assembly)

- `v1`: CPU とアセンブリの基礎（レジスタ、RIP、mov/add/sub/flags、syscall）
- `v2`: 制御フロー（`cmp` と条件分岐 — `jz`/`jnz`/`jg`/`jmp`、ループ）
- `v3`: メモリと load/store（`.data` セクション、配列）
- `v4`: スタックと `call`/`ret`（`push`/`pop`、関数呼び出し）
- `v5`: システムコールの仕組み（`sys_write`、ユーザー/カーネルモード）
- `v6`: ファイルディスクリプタと I/O（`sys_read`/`sys_open`/`sys_close`、`.bss`）
- `v7`: プロセスとシグナル（`fork`/`execve`/`wait4`、PID）
- `v8`: 仮想メモリ（`mmap`/`mprotect` システムコール）
