#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/freelist_alloc"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

# exit code 0（正常終了）
if [[ $rc -ne 0 ]]; then
  echo "FAIL: freelist_alloc returned $rc, expected 0" >&2
  exit 1
fi

# free 後に同じアドレスが再利用されることを確認
if ! echo "$output" | grep -q "reused: yes"; then
  echo "FAIL: freelist_alloc did not reuse address" >&2
  echo "got:"
  echo "$output"
  exit 1
fi

# コアレッシング後に大きなブロックが確保できることを確認
if ! echo "$output" | grep -q "coalesced: yes"; then
  echo "FAIL: freelist_alloc did not coalesce" >&2
  echo "got:"
  echo "$output"
  exit 1
fi

echo "PASS: freelist_alloc reuse and coalescing work correctly"
