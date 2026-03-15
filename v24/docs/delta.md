# Delta

- v23 のページングの上に syscall/sysret 機構を追加
- 新しい概念: MSR (STAR, LSTAR, SFMASK), syscall/sysret 命令, TSS, Ring 0/Ring 3 遷移
- 新しいファイル: usermode.h, usermode.c, syscall_handler.c
- v24 では Ring 0 から syscall を呼び出す概念実証。完全な Ring 3 遷移は v26
