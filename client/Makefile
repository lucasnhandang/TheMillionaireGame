# Makefile for Millionaire Game Client
# C++ Client with ImGui GUI

CXX = g++
UNAME_S := $(shell uname -s)

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)  # macOS
    CXX = clang++
    CXXFLAGS = -std=c++11 -Wall -Wextra -g -I. -I../imgui -I../imgui/backends $(GLFW_CFLAGS)
    LDFLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else  # Linux
    CXXFLAGS = -std=c++11 -Wall -Wextra -g -I. -I../imgui -I../imgui/backends $(GLFW_CFLAGS)
    LDFLAGS = -pthread -lGL -lGLU -lX11 -lXrandr -lXinerama -lXcursor -ldl -lrt
endif

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

# GLFW library - try system library first, then build from source
GLFW_BUILD_DIR = $(GLFW_DIR)/build
GLFW_LIB_BUILT = $(GLFW_BUILD_DIR)/src/libglfw3.a
GLFW_LIB_OLD = $(GLFW_DIR)/lib/libglfw3.a

# Try to use system GLFW first, fallback to building from source
GLFW_PKG_CONFIG := $(shell pkg-config --exists glfw3 2>/dev/null && echo "yes" || echo "no")
ifeq ($(GLFW_PKG_CONFIG),yes)
    GLFW_CFLAGS := $(shell pkg-config --cflags glfw3)
    GLFW_LDFLAGS := $(shell pkg-config --libs glfw3) $(LDFLAGS)
    GLFW_LIB :=
else
    # Build from source - prefer new build location
    GLFW_CFLAGS := -I$(GLFW_DIR)/include
    ifeq ($(wildcard $(GLFW_LIB_BUILT)),)
        # Try old location
        ifeq ($(wildcard $(GLFW_LIB_OLD)),)
            GLFW_LIB := $(GLFW_LIB_BUILT)
        else
            GLFW_LIB := $(GLFW_LIB_OLD)
        endif
    else
        GLFW_LIB := $(GLFW_LIB_BUILT)
    endif
    GLFW_LDFLAGS := $(GLFW_LIB) $(LDFLAGS)
endif

# Executable
CLIENT_EXE = $(BIN_DIR)/client

# Default target
all: directories $(CLIENT_EXE)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build client
ifeq ($(GLFW_PKG_CONFIG),yes)
$(CLIENT_EXE): $(CLIENT_OBJ) $(IMGUI_OBJ) $(IMGUI_BACKEND_OBJ)
	@echo "Linking client (using system GLFW)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GLFW_LDFLAGS)
	@echo "Build complete! Executable: $@"
else
$(CLIENT_EXE): $(CLIENT_OBJ) $(IMGUI_OBJ) $(IMGUI_BACKEND_OBJ)
	@if [ ! -f "$(GLFW_LIB)" ]; then \
		echo "GLFW library not found. Building GLFW..."; \
		$(MAKE) $(GLFW_LIB); \
	fi
	@echo "Linking client (using built GLFW)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GLFW_LDFLAGS)
	@echo "Build complete! Executable: $@"
endif

# Compile client object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ImGui object files
$(OBJ_DIR)/imgui_%.o: $(IMGUI_DIR)/%.cpp
	@echo "Compiling ImGui: $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ImGui backend object files
$(OBJ_DIR)/imgui_backend_%.o: $(IMGUI_DIR)/backends/%.cpp
	@echo "Compiling ImGui backend: $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build GLFW if needed (platform-specific)
$(GLFW_LIB_BUILT):
	@echo "Building GLFW from source..."
	@mkdir -p $(GLFW_BUILD_DIR)
ifeq ($(UNAME_S),Darwin)  # macOS
	@cd $(GLFW_BUILD_DIR) && cmake .. \
		-DGLFW_BUILD_X11=OFF \
		-DGLFW_BUILD_WAYLAND=OFF \
		-DGLFW_BUILD_WIN32=OFF \
		-DGLFW_BUILD_COCOA=ON \
		-DBUILD_SHARED_LIBS=OFF \
		-DGLFW_BUILD_EXAMPLES=OFF \
		-DGLFW_BUILD_TESTS=OFF \
		-DGLFW_BUILD_DOCS=OFF
else  # Linux
	@cd $(GLFW_BUILD_DIR) && cmake .. \
		-DGLFW_BUILD_X11=ON \
		-DGLFW_BUILD_WAYLAND=OFF \
		-DGLFW_BUILD_WIN32=OFF \
		-DGLFW_BUILD_COCOA=OFF \
		-DBUILD_SHARED_LIBS=OFF \
		-DGLFW_BUILD_EXAMPLES=OFF \
		-DGLFW_BUILD_TESTS=OFF \
		-DGLFW_BUILD_DOCS=OFF
endif
	@cd $(GLFW_BUILD_DIR) && make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
	@echo "GLFW build complete!"

$(GLFW_LIB_OLD): $(GLFW_LIB_BUILT)
	@echo "Copying GLFW library to lib directory..."
	@mkdir -p $(GLFW_DIR)/lib
	@cp $(GLFW_LIB_BUILT) $(GLFW_LIB_OLD)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Rebuild GLFW (platform-specific)
rebuild-glfw:
	@echo "=== Rebuilding GLFW ==="
	@cd $(GLFW_DIR) && rm -rf build
	@mkdir -p $(GLFW_BUILD_DIR)
ifeq ($(UNAME_S),Darwin)  # macOS
	@cd $(GLFW_BUILD_DIR) && cmake .. \
		-DGLFW_BUILD_X11=OFF \
		-DGLFW_BUILD_WAYLAND=OFF \
		-DGLFW_BUILD_WIN32=OFF \
		-DGLFW_BUILD_COCOA=ON \
		-DBUILD_SHARED_LIBS=OFF \
		-DGLFW_BUILD_EXAMPLES=OFF \
		-DGLFW_BUILD_TESTS=OFF \
		-DGLFW_BUILD_DOCS=OFF
else  # Linux
	@cd $(GLFW_BUILD_DIR) && cmake .. \
		-DGLFW_BUILD_X11=ON \
		-DGLFW_BUILD_WAYLAND=OFF \
		-DGLFW_BUILD_WIN32=OFF \
		-DGLFW_BUILD_COCOA=OFF \
		-DBUILD_SHARED_LIBS=OFF \
		-DGLFW_BUILD_EXAMPLES=OFF \
		-DGLFW_BUILD_TESTS=OFF \
		-DGLFW_BUILD_DOCS=OFF
endif
	@cd $(GLFW_BUILD_DIR) && make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
	@if [ -f "$(GLFW_LIB_BUILT)" ]; then \
		mkdir -p $(GLFW_DIR)/lib; \
		cp $(GLFW_LIB_BUILT) $(GLFW_DIR)/lib/; \
		echo "GLFW rebuild complete! Library: $(GLFW_DIR)/lib/libglfw3.a"; \
	else \
		echo "Error: GLFW library not found after build!"; \
		exit 1; \
	fi

# Run client
run: $(CLIENT_EXE)
	./$(CLIENT_EXE)

# Phony targets
.PHONY: all clean run directories rebuild-glfw

