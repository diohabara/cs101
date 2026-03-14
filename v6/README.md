# v6

ファイルディスクリプタと I/O を扱う段階です。

## 内容

- `read_stdin.asm` — stdin から読み取りエコーバック。.bss セクション初登場
- `write_file.asm` — ファイル作成 → 書き込み → クローズのライフサイクル
- `fd_table.asm` — open すると fd=3 が返ることを確認

## ビルド・テスト

```bash
just test v6
```

## GDB トレース

```bash
gdb -x gdb/fd_trace.gdb build/fd_table
gdb -x gdb/read_trace.gdb build/read_stdin
```
