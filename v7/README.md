# v7

プロセスとシグナルを扱う段階です。

## 内容

- `getpid.asm` — sys_getpid でプロセス ID を取得
- `fork_basic.asm` — sys_fork でプロセスを複製、親子で異なる処理
- `execve_echo.asm` — fork + execve で別プログラムを実行

## ビルド・テスト

```bash
just test v7
```

## GDB トレース

```bash
gdb -x gdb/getpid_trace.gdb build/getpid
gdb -x gdb/fork_trace.gdb build/fork_basic
```
