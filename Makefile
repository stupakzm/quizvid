##
# Project Title
#
# @file
# @version 0.1
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include $(shell pkg-config --cflags freetype2)
LDFLAGS = -lavformat -lavcodec -lavutil -lswscale -lswresample -lfreetype -ljson-c
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/quizvid
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/video.c $(SRC_DIR)/text.c $(SRC_DIR)/quiz.c $(SRC_DIR)/colors.c $(SRC_DIR)/config.c $(SRC_DIR)/audio.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/video.o $(BUILD_DIR)/text.o $(BUILD_DIR)/quiz.o $(BUILD_DIR)/colors.o $(BUILD_DIR)/config.o $(BUILD_DIR)/audio.o

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


quick: clean all test

TEST_AUDIO_OBJS = build/video.o build/text.o build/quiz.o build/colors.o build/config.o build/audio.o
test-audio: $(TEST_AUDIO_OBJS)
	$(CC) $(CFLAGS) test_audio.c $(TEST_AUDIO_OBJS) -o bin/test_audio $(LDFLAGS)
	./bin/test_audio

.PHONY: all clean run test quick test-audio

compile_commands.json:
	bear -- make
# end
