#!/usr/bin/env bash
set -euo pipefail

# ホスト上でコンパイルが成功することを確認するテスト
DIR="$(cd "$(dirname "$0")/.." && pwd)"

if make -C "$DIR" clean all 2>&1; then
  echo "PASS: compilation succeeded"
else
  echo "FAIL: compilation failed" >&2
  exit 1
fi

# kernel.bin が生成されたことを確認
if [[ -f "$DIR/build/kernel.bin" ]]; then
  echo "PASS: kernel.bin exists"
else
  echo "FAIL: kernel.bin not found" >&2
  exit 1
fi
