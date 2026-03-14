# Container Guide

- Host assumption: `macOS arm64`
- Canonical runtime: `linux/amd64` Docker container
- Build and test inside the container to avoid host-tool drift.
- Use `just` as the top-level entrypoint.

## Read the stage books

```bash
just docs
```

## Enter the container

```bash
just shell
```

## Run a stage test

```bash
just test v3
```
