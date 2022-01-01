# makefile based on: https://makefiletutorial.com/#makefile-cookbook

TARGET_EXEC := preprocessor

BUILD_DIR := ./build
SRC_DIR := ./src

SRCS := $(shell find $(SRC_DIR) -name '*.c')

C_FLAGS := -Wall -pedantic 

all : $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRCS) $(C_FLAGS) -I $(SRC_DIR) -o $(BUILD_DIR)/$(TARGET_EXEC)

clean:
	rm -r $(BUILD_DIR)



