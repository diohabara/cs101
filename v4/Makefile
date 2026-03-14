ASM_DIR   := asm
BUILD_DIR := build
SRCS      := $(wildcard $(ASM_DIR)/*.asm)
BINS      := $(patsubst $(ASM_DIR)/%.asm,$(BUILD_DIR)/%,$(SRCS))

all: $(BINS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(ASM_DIR)/%.asm | $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf $< -o $@

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	ld $< -o $@

test: all
	bash tests/run_all.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test
