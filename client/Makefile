# Makefile for Millionaire Game Client
# C++ Client with ImGui GUI

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -I. -I../imgui -I../imgui/backends -I../glfw/include
LDFLAGS = -pthread -lGL -lGLU -lX11 -lXrandr -lXinerama -lXcursor -ldl -lrt

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin
IMGUI_DIR = ../imgui
GLFW_DIR = ../glfw

# Source files
CLIENT_SRC = main.cpp socket_client.cpp protocol_handler.cpp json_utils.cpp
IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp \
            $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_BACKEND_SRC = $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
GLFW_SRC = $(GLFW_DIR)/src/*.c

# Object files
CLIENT_OBJ = $(CLIENT_SRC:%.cpp=$(OBJ_DIR)/%.o)
IMGUI_OBJ = $(IMGUI_SRC:$(IMGUI_DIR)/%.cpp=$(OBJ_DIR)/imgui_%.o)
IMGUI_BACKEND_OBJ = $(IMGUI_BACKEND_SRC:$(IMGUI_DIR)/backends/%.cpp=$(OBJ_DIR)/imgui_backend_%.o)

# GLFW library (use static library if available)
GLFW_LIB = $(GLFW_DIR)/lib/libglfw3.a
GLFW_LDFLAGS = $(GLFW_LIB) $(LDFLAGS)

# Executable
CLIENT_EXE = $(BIN_DIR)/client

# Default target
all: directories $(CLIENT_EXE)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build client
$(CLIENT_EXE): $(CLIENT_OBJ) $(IMGUI_OBJ) $(IMGUI_BACKEND_OBJ) | $(GLFW_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GLFW_LDFLAGS)

# Compile client object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ImGui object files
$(OBJ_DIR)/imgui_%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ImGui backend object files
$(OBJ_DIR)/imgui_backend_%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build GLFW if needed
$(GLFW_LIB):
	@echo "GLFW library not found. Building GLFW..."
	@cd $(GLFW_DIR) && cmake . && make

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run client
run: $(CLIENT_EXE)
	./$(CLIENT_EXE)

# Phony targets
.PHONY: all clean run directories

