#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/mmio_sim"
output="$("$PROG")"

fail=0

if echo "$output" | grep -q "command: CMD_READ sent"; then
  echo "PASS: found 'command: CMD_READ sent'"
else
  echo "FAIL: missing 'command: CMD_READ sent'" >&2
  fail=1
fi

if echo "$output" | grep -q "polling status..."; then
  echo "PASS: found 'polling status...'"
else
  echo "FAIL: missing 'polling status...'" >&2
  fail=1
fi

if echo "$output" | grep -q "status: DONE"; then
  echo "PASS: found 'status: DONE'"
else
  echo "FAIL: missing 'status: DONE'" >&2
  fail=1
fi

if echo "$output" | grep -q "data: 0xDEADBEEF"; then
  echo "PASS: found 'data: 0xDEADBEEF'"
else
  echo "FAIL: missing 'data: 0xDEADBEEF'" >&2
  fail=1
fi

exit $fail
