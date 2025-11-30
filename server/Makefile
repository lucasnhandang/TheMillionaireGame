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
CONFIG_SRC = config.cpp
SERVER_SRC = server.cpp
EXAMPLE_SRC = stream_handler_example.cpp
TEST_SRC = stream_handler_test.cpp

# Object files
STREAM_HANDLER_OBJ = $(OBJ_DIR)/stream_handler.o
LOGGER_OBJ = $(OBJ_DIR)/logger.o
CONFIG_OBJ = $(OBJ_DIR)/config.o
SERVER_OBJ = $(OBJ_DIR)/server.o
EXAMPLE_OBJ = $(OBJ_DIR)/stream_handler_example.o
TEST_OBJ = $(OBJ_DIR)/stream_handler_test.o

# Executables
SERVER_EXE = $(BIN_DIR)/server
EXAMPLE_EXE = $(BIN_DIR)/stream_handler_example
TEST_EXE = $(BIN_DIR)/stream_handler_test

# Default target
all: directories $(SERVER_EXE) $(EXAMPLE_EXE) $(TEST_EXE)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build main server
$(SERVER_EXE): $(SERVER_OBJ) $(STREAM_HANDLER_OBJ) $(LOGGER_OBJ) $(CONFIG_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

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

# Run main server
run: $(SERVER_EXE)
	./$(SERVER_EXE)

# Run example server
run-example: $(EXAMPLE_EXE)
	./$(EXAMPLE_EXE)

# Phony targets
.PHONY: all clean test run run-example directories

