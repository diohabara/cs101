#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/two_mappings"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 11 ]]; then
  echo "PASS: two_mappings returned 11"
else
  echo "FAIL: two_mappings returned $rc, expected 11" >&2
  exit 1
fi
