set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

default:
  @just --list

# Enter the canonical linux/amd64 container shell.
shell:
  ./scripts/container/enter.sh

# Start the Markdown textbook viewer locally.
docs:
  npm run dev

# Build the Markdown textbook viewer.
docs-build:
  npm run build

# Build the shared container image explicitly.
docker-build:
  docker compose build lab

# Run a single stage test in the container.
test stage:
  ./scripts/container/test-version.sh {{stage}}

# Run a single stage test on the host (asm stages need linux/amd64 container).
test-local stage:
  @if [ -f {{stage}}/Makefile ]; then \
    echo "asm stage detected — running in container (linux/amd64 required)"; \
    just test {{stage}}; \
  else \
    cargo test --manifest-path {{stage}}/Cargo.toml; \
  fi

# Run the currently implemented CPU stages in order.
test-cpu:
  for stage in v1 v2 v3 v4 v5 v6 v7 v8; do \
    just test "$stage"; \
  done

# Run the OS + Hypervisor stages in order.
test-os:
  for stage in v9 v10 v11 v12 v13 v14 v15 v16 v17 v18 v19 v20 v21 v22 v23 v24 v25 v26 v27 v28 v29 v30 v31; do \
    just test "$stage"; \
  done

# Run everything that is implemented today.
test-all:
  just test-cpu
  just test-os
