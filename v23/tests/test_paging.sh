#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をキャプチャ
output="$(timeout 10 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial stdio \
  -display none \
  -no-reboot \
  -nographic \
  2>/dev/null || true)"

PASS=0
FAIL=0

# ページング有効化メッセージの確認
if echo "$output" | grep -qF "Paging enabled"; then
  echo "PASS: Paging enabled"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected 'Paging enabled'" >&2
  FAIL=$((FAIL + 1))
fi

# ページング後の UART 動作確認
if echo "$output" | grep -qF "UART works after paging"; then
  echo "PASS: UART works after paging"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected 'UART works after paging'" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
