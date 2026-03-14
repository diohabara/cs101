# v3

メモリと load/store を扱う段階です。x86-64 アセンブリ（NASM Intel 構文）で `.data` セクションに変数を定義し、`mov [addr], reg`（store）と `mov reg, [addr]`（load）によるメモリ操作を学びます。

## ビルドとテスト

コンテナ内（linux/amd64）で実行してください。

```bash
just test v3
```

または直接：

```bash
make test
```

## ファイル構成

- `asm/store_load.asm` — メモリへの書き込みと読み出し
- `asm/array_sum.asm` — 配列の全要素を合計
- `asm/memory_modify.asm` — メモリの値を読み出し・加算・書き戻し
- `tests/` — 各プログラムの終了コードを検証するシェルテスト
- `gdb/memory_trace.gdb` — store_load の逐次実行トレーススクリプト
- `book.md` — Web ビューア用の解説書
- `docs/` — delta, test-map などの補助メタデータ
