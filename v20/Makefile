SRC_DIR   := src
BUILD_DIR := build

# C ソースファイル
C_SRCS    := $(wildcard $(SRC_DIR)/*.c)
C_OBJS    := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))

KERNEL    := $(BUILD_DIR)/kernel.bin

# C コンパイラ（64ビットベアメタルターゲット）
CC        := clang
CFLAGS    := -Wall -Wextra -std=c11 -g -ffreestanding -nostdlib -mno-red-zone \
             -fno-stack-protector -mgeneral-regs-only -target x86_64-unknown-none -I$(SRC_DIR)

# アセンブラ（NASM、32ビット ELF 出力）
NASM      := nasm
NASMFLAGS := -f elf32

all: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Stage 1: C コードを 64ビットオブジェクトにコンパイル
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Stage 2: 64ビットオブジェクトを ELF にリンクし、フラットバイナリに変換
$(BUILD_DIR)/kernel_code.elf: $(C_OBJS)
	ld.lld -T $(SRC_DIR)/linker_code.ld --nostdlib $^ -o $@

$(BUILD_DIR)/kernel_code.bin: $(BUILD_DIR)/kernel_code.elf
	llvm-objcopy -O binary $< $@

# Stage 3: boot.asm が kernel_code.bin を incbin で埋め込み、32ビット ELF を生成
$(BUILD_DIR)/boot_asm.o: $(SRC_DIR)/boot.asm $(BUILD_DIR)/kernel_code.bin | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# Stage 4: 最終カーネル（32ビット ELF、QEMU PVH 対応）
$(KERNEL): $(BUILD_DIR)/boot_asm.o
	ld -m elf_i386 -T $(SRC_DIR)/linker.ld --nostdlib $< -o $@

test: all
	bash tests/run_all.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test
