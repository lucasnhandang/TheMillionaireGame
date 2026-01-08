# Makefile for Qt-based Millionaire Game Client
# This project now uses Qt5 with CMake instead of ImGui/GLFW

# Detect OS
UNAME_S := $(shell uname -s)

# Build directory
BUILD_DIR = build

# Executable name (same on all platforms when built with CMake)
EXE = $(BUILD_DIR)/MillionaireGameClient

# Default target
all: $(EXE)

# Build with CMake
$(EXE):
	@echo "=== Building Qt Client with CMake ==="
	@if ! command -v qmake >/dev/null 2>&1 && ! command -v qmake5 >/dev/null 2>&1; then \
		echo ""; \
		echo "ERROR: Qt5 is not installed!"; \
		echo ""; \
		echo "Please install Qt5:"; \
		if [ "$(UNAME_S)" = "Darwin" ]; then \
			echo "  brew install qt@5"; \
			echo "  export PATH=\"/usr/local/opt/qt@5/bin:\$$PATH\""; \
		else \
			echo "  sudo apt-get update"; \
			echo "  sudo apt-get install -y qtbase5-dev qt5-qmake"; \
		fi; \
		echo ""; \
		echo "See BUILD_QT.md for detailed instructions."; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && $(MAKE)
	@echo ""
	@echo "=== Build Complete ==="
	@echo "Executable: $(EXE)"
	@echo ""
	@echo "Run with: make run"
	@echo "Or:       ./$(EXE)"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run the client
run: $(EXE)
	./$(EXE)

# Run in demo mode (no server)
demo: $(EXE)
	./$(EXE) --demo

# Rebuild everything
rebuild: clean all

# Help message
help:
	@echo "Qt-based Millionaire Game Client - Build System"
	@echo ""
	@echo "This project uses Qt5 with CMake (not ImGui/GLFW)"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the client"
	@echo "  make run          - Build and run with default server (localhost:8080)"
	@echo "  make demo         - Build and run in demo mode (no server)"
	@echo "  make clean        - Remove build directory"
	@echo "  make rebuild      - Clean and build from scratch"
	@echo ""
	@echo "Requirements:"
	@echo "  - Qt5 (qtbase5-dev on Linux, qt@5 on macOS)"
	@echo "  - CMake 3.16+"
	@echo ""
	@echo "See BUILD_QT.md for detailed setup instructions."

# Check Qt installation
check-qt:
	@echo "Checking Qt5 installation..."
	@if command -v qmake >/dev/null 2>&1; then \
		echo "✓ qmake found: $$(command -v qmake)"; \
		echo "  Version: $$(qmake -v | grep -o 'Qt version [0-9.]*')"; \
		echo "  Qt path: $$(qmake -query QT_INSTALL_PREFIX)"; \
	elif command -v qmake5 >/dev/null 2>&1; then \
		echo "✓ qmake5 found: $$(command -v qmake5)"; \
		echo "  Version: $$(qmake5 -v | grep -o 'Qt version [0-9.]*')"; \
		echo "  Qt path: $$(qmake5 -query QT_INSTALL_PREFIX)"; \
	else \
		echo "✗ Qt5 not found!"; \
		echo ""; \
		echo "Install with:"; \
		if [ "$(UNAME_S)" = "Darwin" ]; then \
			echo "  brew install qt@5"; \
		else \
			echo "  sudo apt-get install qtbase5-dev qt5-qmake"; \
		fi; \
	fi
	@echo ""
	@if command -v cmake >/dev/null 2>&1; then \
		echo "✓ CMake found: $$(cmake --version | head -n1)"; \
	else \
		echo "✗ CMake not found!"; \
		echo "  Install with: sudo apt-get install cmake"; \
	fi

.PHONY: all clean run demo rebuild help check-qt
