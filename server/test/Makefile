# Makefile for Unit Tests
# TheMillionaireGame Server - Test Suite
#
# Usage:
#   make              - Build all tests
#   make all          - Build all tests
#   make test         - Build and run all tests
#   make <test_name>  - Build specific test (e.g., make stream_handler_test)
#   make run          - Run all tests
#   make clean        - Remove compiled test binaries
#
# Individual tests:
#   make stream_handler_test
#   make socket_io_test
#   make session_manager_test
#   make auth_manager_test

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -I..
LDFLAGS = -pthread

# Directories
SRC_DIR = ..
TEST_DIR = .
BIN_DIR = bin
OBJ_DIR = obj

# Source files from parent directory
COMMON_SOURCES = $(SRC_DIR)/logger.cpp

# Test executables
TEST_BINS = \
	$(BIN_DIR)/stream_handler_test \
	$(BIN_DIR)/socket_io_test \
	$(BIN_DIR)/session_manager_test \
	$(BIN_DIR)/auth_manager_test

# Default target
all: dirs $(TEST_BINS)

# Create directories
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

# ============================================================================
# Stream Handler Test
# ============================================================================

$(BIN_DIR)/stream_handler_test: $(TEST_DIR)/stream_handler_test.cpp $(SRC_DIR)/stream_handler.cpp $(COMMON_SOURCES)
	@echo "Building stream_handler_test..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Done: $@"

stream_handler_test: dirs $(BIN_DIR)/stream_handler_test

# ============================================================================
# Socket I/O Test
# ============================================================================

$(BIN_DIR)/socket_io_test: $(TEST_DIR)/socket_io_test.cpp $(COMMON_SOURCES)
	@echo "Building socket_io_test..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Done: $@"

socket_io_test: dirs $(BIN_DIR)/socket_io_test

# ============================================================================
# Session Manager Test
# ============================================================================

$(BIN_DIR)/session_manager_test: $(TEST_DIR)/session_manager_test.cpp $(SRC_DIR)/session_manager.cpp $(COMMON_SOURCES)
	@echo "Building session_manager_test..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Done: $@"

session_manager_test: dirs $(BIN_DIR)/session_manager_test

# ============================================================================
# Auth Manager Test
# ============================================================================

$(BIN_DIR)/auth_manager_test: $(TEST_DIR)/auth_manager_test.cpp $(SRC_DIR)/auth_manager.cpp $(SRC_DIR)/session_manager.cpp $(SRC_DIR)/json_utils.cpp $(COMMON_SOURCES)
	@echo "Building auth_manager_test..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Done: $@"

auth_manager_test: dirs $(BIN_DIR)/auth_manager_test

# ============================================================================
# Run Tests
# ============================================================================

run: all
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║                Running All Unit Tests                          ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@failed=0; \
	for test in $(TEST_BINS); do \
		echo ""; \
		echo "────────────────────────────────────────────────────────────────"; \
		echo "Running: $$test"; \
		echo "────────────────────────────────────────────────────────────────"; \
		if $$test; then \
			echo ""; \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "════════════════════════════════════════════════════════════════"; \
	if [ $$failed -eq 0 ]; then \
		echo "✓ All test suites passed!"; \
	else \
		echo "✗ $$failed test suite(s) had failures"; \
	fi; \
	echo "════════════════════════════════════════════════════════════════"; \
	exit $$failed

# Alias for run
test: run

# Run individual tests
run-stream: stream_handler_test
	@$(BIN_DIR)/stream_handler_test

run-socket: socket_io_test
	@$(BIN_DIR)/socket_io_test

run-session: session_manager_test
	@$(BIN_DIR)/session_manager_test

run-auth: auth_manager_test
	@$(BIN_DIR)/auth_manager_test

# ============================================================================
# Clean
# ============================================================================

clean:
	@echo "Cleaning test binaries..."
	rm -rf $(BIN_DIR) $(OBJ_DIR)
	@echo "Clean complete."

# ============================================================================
# Help
# ============================================================================

help:
	@echo "Millionaire Game Server - Test Suite Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all                  - Build all tests (default)"
	@echo "  test / run           - Build and run all tests"
	@echo "  stream_handler_test  - Build stream handler test"
	@echo "  socket_io_test       - Build socket I/O test"
	@echo "  session_manager_test - Build session manager test"
	@echo "  auth_manager_test    - Build auth manager test"
	@echo "  run-stream           - Run stream handler test"
	@echo "  run-socket           - Run socket I/O test"
	@echo "  run-session          - Run session manager test"
	@echo "  run-auth             - Run auth manager test"
	@echo "  clean                - Remove compiled binaries"
	@echo "  help                 - Show this help message"
	@echo ""
	@echo "Example:"
	@echo "  make test            - Build and run all tests"
	@echo "  make auth_manager_test && ./bin/auth_manager_test"

.PHONY: all dirs test run clean help \
	stream_handler_test socket_io_test session_manager_test auth_manager_test \
	run-stream run-socket run-session run-auth
