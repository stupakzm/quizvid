##
# Project Title
#
# @file
# @version 0.1
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include $(shell pkg-config --cflags freetype2)
LDFLAGS = -lavformat -lavcodec -lavutil -lswscale -lfreetype -ljson-c
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/quizvid
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/video.c $(SRC_DIR)/text.c $(SRC_DIR)/quiz.c $(SRC_DIR)/colors.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/video.o $(BUILD_DIR)/text.o $(BUILD_DIR)/quiz.o $(BUILD_DIR)/colors.o

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

run: $(TARGET)
	./$(TARGET)


test: $(TARGET)
	@echo "Running QuizVid..."
	./$(TARGET)
	@echo ""
	@echo "Playing video..."
	ffplay -autoexit quiz_video.mp4


quick: clean all run

.PHONY: all clean run test quick

compile_commands.json:
	bear -- make
# end
