#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/fork_basic"
output=$("$PROG") && rc=$? || rc=$?
has_child=false
has_parent=false
echo "$output" | grep -q "child" && has_child=true
echo "$output" | grep -q "parent" && has_parent=true
if [[ $rc -eq 42 ]] && $has_child && $has_parent; then
  echo "PASS: fork_basic exited 42, printed 'child' and 'parent'"
else
  echo "FAIL: fork_basic rc=$rc, child=$has_child, parent=$has_parent" >&2
  echo "output: $output" >&2
  exit 1
fi
