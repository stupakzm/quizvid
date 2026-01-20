##
# Project Title
#
# @file
# @version 0.1
CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lavformat -lavcodec -lavutil -lswscale
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/quizvid
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/video.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/video.o

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean

compile_commands.json:
	bear -- make
# end
