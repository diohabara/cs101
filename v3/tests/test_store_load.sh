#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/store_load"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 42 ]]; then
  echo "PASS: store_load returned 42"
else
  echo "FAIL: store_load returned $rc, expected 42" >&2
  exit 1
fi
