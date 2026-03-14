#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/array_sum"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 100 ]]; then
  echo "PASS: array_sum returned 100"
else
  echo "FAIL: array_sum returned $rc, expected 100" >&2
  exit 1
fi
