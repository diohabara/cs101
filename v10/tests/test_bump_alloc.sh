#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/bump_alloc"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

# exit code = ユニークなアドレスの数（期待値: 3）
if [[ $rc -ne 3 ]]; then
  echo "FAIL: bump_alloc returned $rc, expected 3" >&2
  exit 1
fi

# stdout にアドレスが連続していることを確認
if echo "$output" | grep -q "addresses are sequential: yes"; then
  echo "PASS: bump_alloc returned 3 unique sequential addresses"
else
  echo "FAIL: bump_alloc output mismatch" >&2
  echo "got:"
  echo "$output"
  exit 1
fi
