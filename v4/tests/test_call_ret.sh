#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/call_ret"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 15 ]]; then
  echo "PASS: call_ret returned 15 (10+5)"
else
  echo "FAIL: call_ret returned $rc, expected 15" >&2
  exit 1
fi
