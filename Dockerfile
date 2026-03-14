FROM rust:1.93-bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    binutils \
    ca-certificates \
    clang \
    curl \
    gdb \
    lld \
    llvm \
    make \
    nasm \
    pkg-config \
    python3 \
    qemu-system-x86 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
