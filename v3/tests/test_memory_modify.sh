#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/memory_modify"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 8 ]]; then
  echo "PASS: memory_modify returned 8"
else
  echo "FAIL: memory_modify returned $rc, expected 8" >&2
  exit 1
fi
