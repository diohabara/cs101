#!/usr/bin/env bash
set -euo pipefail

PROG="$(dirname "$0")/../build/regs"

"$PROG" && rc=$? || rc=$?

# 1+2+3+4 = 10
if [[ $rc -eq 10 ]]; then
  echo "PASS: regs returned 10 (1+2+3+4)"
else
  echo "FAIL: regs returned $rc, expected 10" >&2
  exit 1
fi
