#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をキャプチャ
# タイマー割り込みで 50 ティック待つので十分な時間を確保
output="$(timeout 10 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial stdio \
  -display none \
  -no-reboot \
  -nographic \
  2>/dev/null || true)"

PASS=0
FAIL=0

# Timer started メッセージの確認
if echo "$output" | grep -qF "Timer started: 100Hz"; then
  echo "PASS: Timer started message"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected 'Timer started: 100Hz'" >&2
  FAIL=$((FAIL + 1))
fi

# 50 ticks reached メッセージの確認
if echo "$output" | grep -qF "50 ticks reached"; then
  echo "PASS: 50 ticks reached"
  PASS=$((PASS + 1))
else
  echo "FAIL: expected '50 ticks reached'" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
