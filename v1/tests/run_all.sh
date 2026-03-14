#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0

for test_script in "$DIR"/test_*.sh; do
  name="$(basename "$test_script")"
  echo "--- $name ---"
  if bash "$test_script"; then
    PASS=$((PASS + 1))
  else
    FAIL=$((FAIL + 1))
  fi
done

echo ""
echo "Results: ${PASS} passed, ${FAIL} failed"

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
