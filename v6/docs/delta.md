# Delta

`v5` に対して以下を追加します。

- `sys_read` (syscall #0) — stdin からのデータ読み取り
- `sys_open` (syscall #2) — ファイルのオープン/作成
- `sys_close` (syscall #3) — ファイルディスクリプタのクローズ
- `.bss` セクションでのバッファ確保（`resb`）
- O_WRONLY|O_CREAT|O_TRUNC フラグの組み合わせ
- fd テーブルの理解（stdin=0, stdout=1, stderr=2, 新規=3）
