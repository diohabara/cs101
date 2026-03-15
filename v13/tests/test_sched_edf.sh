#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/sched_edf"

# stdout + exit code をキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: sched_edf returned $rc, expected 0" >&2
  exit 1
fi

expected="EDF order: B(deadline=5) -> C(deadline=8) -> A(deadline=10)
B completed at tick 2 (deadline 5: met)
C completed at tick 3 (deadline 8: met)
A completed at tick 6 (deadline 10: met)
All tasks met their deadlines."

if [[ "$output" == "$expected" ]]; then
  echo "PASS: sched_edf output correct"
else
  echo "FAIL: sched_edf output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
