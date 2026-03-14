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

# Run everything that is implemented today.
test-all:
  just test-cpu
