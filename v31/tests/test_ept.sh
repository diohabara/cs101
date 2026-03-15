#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

output="$(timeout 5 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial stdio \
  -display none \
  -no-reboot \
  -nographic \
  -cpu qemu64,+vmx \
  2>/dev/null || true)"

PASS=0
FAIL=0

# テスト 1: カーネル起動
if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: kernel boot"
  PASS=$((PASS + 1))
else
  echo "FAIL: kernel did not boot" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 2: Phase 3 開始
if echo "$output" | grep -qF "Phase 3: Hypervisor"; then
  echo "PASS: Phase 3 started"
  PASS=$((PASS + 1))
else
  echo "FAIL: Phase 3 message not found" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 3: EPT デモの出力
if echo "$output" | grep -qF "EPT Demo"; then
  echo "PASS: EPT demo shown"
  PASS=$((PASS + 1))
else
  echo "FAIL: EPT demo not found" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 4: メモリ隔離の実証
if echo "$output" | grep -qF "guests are isolated"; then
  echo "PASS: memory isolation demonstrated"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "Different HPAs"; then
  echo "PASS: different HPAs shown"
  PASS=$((PASS + 1))
else
  echo "FAIL: memory isolation not demonstrated" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 5: EPT violation
if echo "$output" | grep -qF "EPT violation"; then
  echo "PASS: EPT violation shown"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "unmapped"; then
  echo "PASS: unmapped GPA detected"
  PASS=$((PASS + 1))
else
  echo "FAIL: EPT violation not shown" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 6: ハイパーバイザ統合完了
if echo "$output" | grep -qF "Hypervisor initialization complete"; then
  echo "PASS: hypervisor initialization complete"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "Hypervisor Summary"; then
  echo "PASS: hypervisor summary shown"
  PASS=$((PASS + 1))
else
  echo "FAIL: hypervisor completion not shown" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
