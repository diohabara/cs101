#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 vN" >&2
  exit 2
fi

stage="$1"

if [[ -f "${stage}/Makefile" ]]; then
  docker compose run --rm --build lab bash -c "make -C ${stage} test"
elif [[ -f "${stage}/Cargo.toml" ]]; then
  docker compose run --rm --build lab bash -c "cargo test --manifest-path ${stage}/Cargo.toml"
else
  echo "No build system found for ${stage}" >&2
  exit 2
fi
