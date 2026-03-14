#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/push_pop"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 20 ]]; then
  echo "PASS: push_pop returned 20 (LIFO)"
else
  echo "FAIL: push_pop returned $rc, expected 20" >&2
  exit 1
fi
