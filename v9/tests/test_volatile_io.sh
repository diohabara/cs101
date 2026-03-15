#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/volatile_io"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 20 ]]; then
  echo "PASS: volatile_io returned 20"
else
  echo "FAIL: volatile_io returned $rc, expected 20" >&2
  exit 1
fi
