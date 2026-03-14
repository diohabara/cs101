# Delta

`v6` に対して以下を追加します。

- `sys_getpid` (syscall #39) — プロセス ID の取得
- `sys_fork` (syscall #57) — プロセスの複製
- `sys_wait4` (syscall #61) — 子プロセスの終了待ち
- `sys_execve` (syscall #59) — プロセスイメージの置き換え
- `sys_unlink` (syscall #87) — ファイルの削除
- fork の戻り値による親子の区別（0 vs 子PID）
- argv 配列の構築（.bss セクション）
