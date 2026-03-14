#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/swap"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 5 ]]; then
  echo "PASS: swap returned 5"
else
  echo "FAIL: swap returned $rc, expected 5" >&2
  exit 1
fi
