#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/nested_calls"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 13 ]]; then
  echo "PASS: nested_calls returned 13 (3+5+5)"
else
  echo "FAIL: nested_calls returned $rc, expected 13" >&2
  exit 1
fi
