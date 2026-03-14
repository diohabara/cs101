#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/write_file"
TESTFILE="/tmp/v6_test.txt"
# Cleanup before
rm -f "$TESTFILE"
"$PROG"
if [[ -f "$TESTFILE" ]] && grep -q "hello from asm" "$TESTFILE"; then
  echo "PASS: write_file created and wrote to $TESTFILE"
else
  echo "FAIL: write_file did not create expected file content" >&2
  [[ -f "$TESTFILE" ]] && cat "$TESTFILE" >&2
  exit 1
fi
# Cleanup after
rm -f "$TESTFILE"
