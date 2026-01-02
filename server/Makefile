# Makefile for Millionaire Game Server
# Network Programming Capstone Project

CXX = g++
# Try to find PostgreSQL include path
PG_INCLUDE := $(shell pg_config --includedir 2>/dev/null || echo "/usr/include/postgresql")
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g -I. -I$(PG_INCLUDE)
LDFLAGS = -pthread -lpq

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Source files
STREAM_HANDLER_SRC = stream_handler.cpp
LOGGER_SRC = logger.cpp
CONFIG_SRC = config.cpp
SERVER_SRC = server.cpp
SERVER_CORE_SRC = server_core.cpp
SESSION_MANAGER_SRC = session_manager.cpp
AUTH_MANAGER_SRC = auth_manager.cpp
REQUEST_ROUTER_SRC = request_router.cpp
CLIENT_HANDLER_SRC = client_handler.cpp
EVENT_LOOP_SRC = event_loop.cpp
JSON_UTILS_SRC = json_utils.cpp
GAME_STATE_MANAGER_SRC = game_state_manager.cpp
DATABASE_SRC = database.cpp
AUTH_HANDLERS_SRC = request_handlers/auth_handlers.cpp
GAME_HANDLERS_SRC = request_handlers/game_handlers.cpp
SOCIAL_HANDLERS_SRC = request_handlers/social_handlers.cpp
USER_HANDLERS_SRC = request_handlers/user_handlers.cpp
ADMIN_HANDLERS_SRC = request_handlers/admin_handlers.cpp
CONNECTION_HANDLERS_SRC = request_handlers/connection_handlers.cpp
NOTIFICATION_UTILS_SRC = notification_utils.cpp
EXAMPLE_SRC = stream_handler_example.cpp
TEST_SRC = stream_handler_test.cpp

# Object files
STREAM_HANDLER_OBJ = $(OBJ_DIR)/stream_handler.o
LOGGER_OBJ = $(OBJ_DIR)/logger.o
CONFIG_OBJ = $(OBJ_DIR)/config.o
SERVER_OBJ = $(OBJ_DIR)/server.o
SERVER_CORE_OBJ = $(OBJ_DIR)/server_core.o
SESSION_MANAGER_OBJ = $(OBJ_DIR)/session_manager.o
AUTH_MANAGER_OBJ = $(OBJ_DIR)/auth_manager.o
REQUEST_ROUTER_OBJ = $(OBJ_DIR)/request_router.o
CLIENT_HANDLER_OBJ = $(OBJ_DIR)/client_handler.o
EVENT_LOOP_OBJ = $(OBJ_DIR)/event_loop.o
JSON_UTILS_OBJ = $(OBJ_DIR)/json_utils.o
GAME_STATE_MANAGER_OBJ = $(OBJ_DIR)/game_state_manager.o
DATABASE_OBJ = $(OBJ_DIR)/database.o
AUTH_HANDLERS_OBJ = $(OBJ_DIR)/auth_handlers.o
GAME_HANDLERS_OBJ = $(OBJ_DIR)/game_handlers.o
SOCIAL_HANDLERS_OBJ = $(OBJ_DIR)/social_handlers.o
USER_HANDLERS_OBJ = $(OBJ_DIR)/user_handlers.o
ADMIN_HANDLERS_OBJ = $(OBJ_DIR)/admin_handlers.o
CONNECTION_HANDLERS_OBJ = $(OBJ_DIR)/connection_handlers.o
NOTIFICATION_UTILS_OBJ = $(OBJ_DIR)/notification_utils.o
EXAMPLE_OBJ = $(OBJ_DIR)/stream_handler_example.o
TEST_OBJ = $(OBJ_DIR)/stream_handler_test.o

# Executables
SERVER_EXE = $(BIN_DIR)/server
EXAMPLE_EXE = $(BIN_DIR)/stream_handler_example
TEST_EXE = $(BIN_DIR)/stream_handler_test

# Default target
all: directories $(SERVER_EXE)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build main server
$(SERVER_EXE): $(SERVER_OBJ) $(SERVER_CORE_OBJ) $(SESSION_MANAGER_OBJ) $(AUTH_MANAGER_OBJ) \
	$(REQUEST_ROUTER_OBJ) $(EVENT_LOOP_OBJ) $(JSON_UTILS_OBJ) $(GAME_STATE_MANAGER_OBJ) \
	$(DATABASE_OBJ) $(AUTH_HANDLERS_OBJ) $(GAME_HANDLERS_OBJ) $(SOCIAL_HANDLERS_OBJ) $(USER_HANDLERS_OBJ) \
	$(ADMIN_HANDLERS_OBJ) $(CONNECTION_HANDLERS_OBJ) $(NOTIFICATION_UTILS_OBJ) $(STREAM_HANDLER_OBJ) $(LOGGER_OBJ) $(CONFIG_OBJ)
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

# Compile request_handlers object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/request_handlers/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean complete."

# Deep clean - remove all object files individually
deepclean: clean
	@echo "Performing deep clean..."
	@find . -name "*.o" -type f -delete 2>/dev/null || true
	@find . -name "*.a" -type f -delete 2>/dev/null || true
	@find . -name "*.so" -type f -delete 2>/dev/null || true
	@echo "Deep clean complete."

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
.PHONY: all clean deepclean test run run-example directories

