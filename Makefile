# Define the compiler and flags
CXX := g++
CXXFLAGS := -std=c++20  -Iinclude -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Werror -O3 -g

# Define the source files and object files
SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
SRC_FILES := $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))
MAIN_OBJ_FILE := $(BUILD_DIR)/main.o
TEST_FILES := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ_FILES := $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(TEST_FILES))

# Define the target executable
TARGET := $(BUILD_DIR)/image_compressor
TEST_TARGET := $(BUILD_DIR)/test_image_compressor

# Default target
all: build

# Build target
build: $(TARGET)

build_all: $(TARGET) $(TEST_TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJ_FILES) $(MAIN_OBJ_FILE)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile the source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Run target
run: $(TARGET)
	./$(TARGET)

# Test target
test: $(TEST_TARGET)
	./$(TEST_TARGET) --abortx 10000

run_test:
	./$(TEST_TARGET) --abortx 10000

# Link the test object files to create the test executable
$(TEST_TARGET): $(OBJ_FILES) $(TEST_OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile the test files into object files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@mkdir -p tmp_images logs
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile the main file separately
$(MAIN_OBJ_FILE): $(SRC_DIR)/main.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean target
clean:
	rm -rf $(BUILD_DIR)
	rm -rf tmp_images logs

.PHONY: all build run test clean