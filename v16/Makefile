SRC_DIR   := src
BUILD_DIR := build
SRCS      := $(wildcard $(SRC_DIR)/*.c)
BINS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%,$(SRCS))
CC        := gcc
CFLAGS    := -Wall -Wextra -std=c11 -g

all: $(BINS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

test: all
	bash tests/run_all.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test
