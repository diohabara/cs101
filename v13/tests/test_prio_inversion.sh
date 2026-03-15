#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/prio_inversion"

# stdout + exit code をキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: prio_inversion returned $rc, expected 0" >&2
  exit 1
fi

expected="WITHOUT inheritance:
  tick 0: LOW:lock
  tick 1: MED:run (preempts LOW, no lock needed)
  tick 2: HIGH:blocked (needs lock held by LOW)
  tick 3: MED:done
  tick 4: LOW:unlock
  tick 5: HIGH:lock HIGH:run HIGH:done
  HIGH waited 3 ticks (blocked by LOW + MED)

WITH inheritance:
  tick 0: LOW:lock
  tick 1: HIGH:blocked -> LOW boosted to HIGH priority
  tick 2: LOW:unlock (runs at HIGH priority, MED cannot preempt)
  tick 3: HIGH:lock HIGH:run HIGH:done
  tick 4: MED:run MED:done
  HIGH waited 1 tick (LOW finished quickly)"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: prio_inversion output correct"
else
  echo "FAIL: prio_inversion output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
