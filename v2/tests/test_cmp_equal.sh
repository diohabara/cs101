#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/cmp_equal"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 0 ]]; then
  echo "PASS: cmp_equal returned 0 (values equal)"
else
  echo "FAIL: cmp_equal returned $rc, expected 0" >&2
  exit 1
fi
