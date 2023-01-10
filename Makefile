# Based on: https://developer.ibm.com/tutorials/au-lexyacc/

CFLAGS := -Wall -pedantic

BUILD_DIR := build
SRC_DIR := src

BIN_NAME := sim

PROG := $(BUILD_DIR)/$(BIN_NAME)

SRCS := $(shell find $(SRC_DIR) -name '*.c')
SRCSP := $(SRCS:%.c=%.c)
OBJS := ${SRCS:.c=.o}
OBJSP :=$(SRCS:%.c=$(BUILD_DIR)/%.o)
SNAMES := ${SRCS:.c=}

CC := gcc

# .PHONY: all
all: $(BUILD_DIR) $(PROG)

$(PROG): $(SRCSP)
	$(CC) $(CFLAGS) $^ -o $@ -lncurses -iquote$(SRC_DIR) -iquote$(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	$(BUILD_DIR)/$(BIN_NAME)

clean:
	rm -rf $(BUILD_DIR)



