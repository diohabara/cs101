# Delta

`v4` に対して以下を追加します。

- `sys_write` (syscall #1) — stdout/stderr へのデータ出力
- `.data` セクションでの文字列定義（`db` + 改行コード `10`）
- `equ` による長さの自動計算（`$ - label`）
- `lea` によるデータアドレスの取得
- fd=1 (stdout) と fd=2 (stderr) の使い分け
- syscall の戻り値が rax を上書きすることの確認
