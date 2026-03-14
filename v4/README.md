# v4

スタックと `call` / `ret` を扱う段階です。

## 内容

- `push` / `pop` による LIFO 動作
- `call` / `ret` による関数呼び出しと復帰
- ネストした関数呼び出しと戻りアドレスの積み重なり

## ビルド・テスト

```bash
# コンテナ内でビルド・テスト
just shell
cd v4
make test

# または
just test v4
```

## GDB で観察

```bash
gdb -x gdb/stack_trace.gdb build/push_pop
gdb -x gdb/call_trace.gdb build/call_ret
```
