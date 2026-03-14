# Testing Strategy

## CPU Foundations (v1-v8): シェルベーステスト

v1-v8 は x86-64 アセンブリ（NASM）で書かれており、テストはシェルスクリプトで行う。

### テスト方式

1. **Exit code 検証**: プログラムの計算結果を exit code（0-255）として返し、期待値と比較する
2. **シグナル検証**: 意図的なクラッシュ（SIGSEGV など）は exit code 139（128 + SIGSEGV）を期待する

### ディレクトリ構成

```
vN/
  tests/
    test_*.sh      # 個別テスト（exit code / シグナルで検証）
    run_all.sh     # 全テスト実行＋結果集計
  gdb/
    *.gdb          # GDB コマンドスクリプト（手動デバッグ・学習用）
```

### 実行方法

```bash
just test vN          # Docker コンテナ内でテスト（linux/amd64）
make -C vN test       # コンテナ内で直接実行
```

### GDB スクリプト

`gdb/` ディレクトリの `.gdb` ファイルはテスト自動化ではなく、`book.md` の学習素材として使用する。Docker の amd64 エミュレーション環境では ptrace の制限により GDB が動作しない場合があるため、自動テストは exit code ベースに統一している。

ネイティブ linux/amd64 環境での手動デバッグ：

```bash
gdb -batch -x vN/gdb/script.gdb vN/build/program
```

### テスト設計原則

- 計算結果は exit code として返す（0-255 の範囲内）
- 範囲外の値が必要な場合は GDB batch で検証（手動テスト扱い）
- 各テストは独立して実行可能
- `run_all.sh` が全テストを集計し、1 つでも失敗すれば exit code 1 を返す

### stdout/stderr キャプチャテスト（v5 以降）

v5 からは exit code だけでなく、stdout/stderr の出力内容も検証対象になる：

```bash
# stdout キャプチャ
output=$("$PROG")
echo "$output" | grep -q "expected string"

# stderr キャプチャ
stderr_output=$("$PROG" 2>&1 1>/dev/null) && rc=$? || rc=$?

# パイプ経由の stdin 入力
output=$(echo "input" | "$PROG")
```

v6 のファイル I/O テストでは `/tmp` のテンポラリファイルを使用し、テスト前後でクリーンアップを行う。
v7 の fork テストでは stdout に親子両方の出力が含まれることを検証する。PID テストは非決定的（PID > 0 のみ検証）。
