# Based on: https://developer.ibm.com/tutorials/au-lexyacc/

CFLAGS := -Wall -pedantic
LIBFLAGS := -lncurses -lm

BUILD_DIR := build
SRC_DIR := src

BIN_NAME := sim

PROG := $(BUILD_DIR)/$(BIN_NAME)

# SRCS := $(shell find $(SRC_DIR) -name '*.c')
SRCQ := debugger.c disassembler.c 65816.c 65816-util.c 65816-ops.c
SRCS := $(SRCQ:%.c=$(SRC_DIR)/%.c)
# OBJS := ${SRCS:.c=.o}
# OBJSP :=$(SRCS:%.c=$(BUILD_DIR)/%.o)
# SNAMES := ${SRCS:.c=}

CC := gcc

# .PHONY: all
all: $(BUILD_DIR) $(PROG)

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBFLAGS) -iquote$(SRC_DIR) -iquote$(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	$(BUILD_DIR)/$(BIN_NAME)

clean:
	rm -rf $(BUILD_DIR)



