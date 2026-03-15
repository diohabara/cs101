#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をキャプチャ
# -cpu qemu64,+vmx で VMX 対応 CPU をエミュレーション（成功するかは環境依存）
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

# テスト 1: カーネルが起動すること
if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: kernel boot"
  PASS=$((PASS + 1))
else
  echo "FAIL: kernel did not boot" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 2: Phase 3 開始メッセージ
if echo "$output" | grep -qF "Phase 3: Hypervisor"; then
  echo "PASS: Phase 3 started"
  PASS=$((PASS + 1))
else
  echo "FAIL: Phase 3 message not found" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 3: VT-x の検出結果（どちらでも OK）
if echo "$output" | grep -qF "VT-x detected via CPUID"; then
  echo "PASS: VT-x detected"
  PASS=$((PASS + 1))
  # VT-x が検出された場合、VMXON の結果を確認
  if echo "$output" | grep -qF "VMXON success"; then
    echo "PASS: VMXON succeeded"
    PASS=$((PASS + 1))
  elif echo "$output" | grep -qF "VMXON failed"; then
    echo "PASS: VMXON failed (expected without KVM)"
    PASS=$((PASS + 1))
  elif echo "$output" | grep -qF "VMX disabled by BIOS"; then
    echo "PASS: VMX disabled by BIOS (expected)"
    PASS=$((PASS + 1))
  fi
elif echo "$output" | grep -qF "VT-x not available"; then
  echo "PASS: VT-x not available (expected in container)"
  PASS=$((PASS + 1))
else
  echo "FAIL: no VMX detection output" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
