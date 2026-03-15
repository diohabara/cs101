SRC_DIR   := src
BUILD_DIR := build
SRCS      := $(filter-out $(SRC_DIR)/linker.ld, $(wildcard $(SRC_DIR)/*.c))
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
KERNEL    := $(BUILD_DIR)/kernel.bin
CC        := clang
CFLAGS    := -Wall -Wextra -std=c11 -g -ffreestanding -nostdlib -mno-red-zone \
             -fno-stack-protector -target x86_64-unknown-none -I$(SRC_DIR)
LD        := ld.lld
LDFLAGS   := -T $(SRC_DIR)/linker.ld --nostdlib

all: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

test: all
	bash tests/run_all.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test
