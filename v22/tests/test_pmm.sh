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

# PMM 初期化メッセージの確認
if echo "$output" | grep -qF "PMM: initialized"; then
  echo "PASS: PMM initialized"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected 'PMM: initialized'" >&2
  FAIL=$((FAIL + 1))
fi

# ページ割り当てメッセージの確認
alloc_count=$(echo "$output" | grep -cF "PMM: allocated page at" || true)
if [[ "$alloc_count" -ge 4 ]]; then
  echo "PASS: 4 pages allocated"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected 4 'PMM: allocated page at' messages, got $alloc_count" >&2
  FAIL=$((FAIL + 1))
fi

# ユニークページの確認
if echo "$output" | grep -qF "4 unique pages"; then
  echo "PASS: 4 unique pages"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected '4 unique pages'" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
