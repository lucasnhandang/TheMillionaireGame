# Makefile for Millionaire Game Server
# Network Programming Capstone Project

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g
LDFLAGS = -pthread

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Source files
STREAM_HANDLER_SRC = stream_handler.cpp
LOGGER_SRC = logger.cpp
EXAMPLE_SRC = stream_handler_example.cpp
TEST_SRC = stream_handler_test.cpp

# Object files
STREAM_HANDLER_OBJ = $(OBJ_DIR)/stream_handler.o
LOGGER_OBJ = $(OBJ_DIR)/logger.o
EXAMPLE_OBJ = $(OBJ_DIR)/stream_handler_example.o
TEST_OBJ = $(OBJ_DIR)/stream_handler_test.o

# Executables
EXAMPLE_EXE = $(BIN_DIR)/stream_handler_example
TEST_EXE = $(BIN_DIR)/stream_handler_test

# Default target
all: directories $(EXAMPLE_EXE) $(TEST_EXE)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build example server
$(EXAMPLE_EXE): $(EXAMPLE_OBJ) $(STREAM_HANDLER_OBJ) $(LOGGER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build test executable
$(TEST_EXE): $(TEST_OBJ) $(STREAM_HANDLER_OBJ) $(LOGGER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run tests
test: $(TEST_EXE)
	./$(TEST_EXE)

# Run example server
run-example: $(EXAMPLE_EXE)
	./$(EXAMPLE_EXE)

# Phony targets
.PHONY: all clean test run-example directories

