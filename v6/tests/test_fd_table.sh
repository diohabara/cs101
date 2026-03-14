#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/fd_table"
"$PROG" && rc=$? || rc=$?
if [[ $rc -gt 2 ]]; then
  echo "PASS: fd_table returned $rc (fd > stderr=2)"
else
  echo "FAIL: fd_table returned $rc, expected > 2" >&2
  exit 1
fi
