#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/write_stderr"
# Capture stderr, expect exit code 1
stderr_output=$("$PROG" 2>&1 1>/dev/null) && rc=$? || rc=$?
if [[ $rc -eq 1 ]] && echo "$stderr_output" | grep -q "stderr"; then
  echo "PASS: write_stderr wrote to stderr and exited with 1"
else
  echo "FAIL: write_stderr rc=$rc, stderr='$stderr_output'" >&2
  exit 1
fi
