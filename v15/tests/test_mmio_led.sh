#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/mmio_led"
output="$("$PROG")"

fail=0

if echo "$output" | grep -q 'pattern 0xF: \[\*\]\[\*\]\[\*\]\[\*\]'; then
  echo "PASS: all LEDs on"
else
  echo "FAIL: expected all LEDs on pattern" >&2
  fail=1
fi

if echo "$output" | grep -q 'pattern 0xA: \[\*\]\[ \]\[\*\]\[ \]'; then
  echo "PASS: alternating LEDs"
else
  echo "FAIL: expected alternating LED pattern" >&2
  fail=1
fi

if echo "$output" | grep -q 'pattern 0x0: \[ \]\[ \]\[ \]\[ \]'; then
  echo "PASS: all LEDs off"
else
  echo "FAIL: expected all LEDs off pattern" >&2
  fail=1
fi

exit $fail
