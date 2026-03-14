# v5

システムコールの仕組みを深く理解する段階です。

## 内容

- `hello.asm` — sys_write で stdout に "Hello, world!" を出力
- `write_stderr.asm` — stderr への出力と fd=1/fd=2 の違い
- `multi_syscall.asm` — 複数 syscall の連続呼び出しと rax 戻り値の上書き

## ビルド・テスト

```bash
just test v5
```

## GDB トレース

```bash
gdb -x gdb/hello_trace.gdb build/hello
gdb -x gdb/multi_syscall_trace.gdb build/multi_syscall
```
