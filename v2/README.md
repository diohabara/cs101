# v2

`v2` は `cmp` と条件分岐を追加します。比較結果がフラグを通じて制御フローへつながることを理解する段階です。

## プログラム

- `cmp_equal.asm` — 等値比較。`cmp` で ZF を設定し `jz` で分岐する
- `countdown.asm` — カウントダウンループ。`sub` + `cmp` + `jnz` でループを構成する
- `max.asm` — 2 値の最大値。`cmp` + `jg` で大小比較し、大きい方を返す

## ビルドとテスト

```bash
# コンテナ内
just test v2

# または直接
make -C v2 test
```

## GDB トレース

```bash
gdb -x gdb/countdown_trace.gdb build/countdown
gdb -x gdb/max_trace.gdb build/max
```
