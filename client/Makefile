# Makefile for Qt-based Millionaire Game Client
# This project now uses Qt5 with CMake instead of ImGui/GLFW

# Detect OS
UNAME_S := $(shell uname -s)

# Build directory
BUILD_DIR = build

# Executable name (same on all platforms when built with CMake)
EXE = $(BUILD_DIR)/MillionaireGameClient

# Auto-detect Qt5 path on macOS (for reference, actual detection happens in targets)
ifeq ($(UNAME_S),Darwin)
	QT5_APPLE_SILICON := $(shell test -f /opt/homebrew/opt/qt@5/bin/qmake && echo /opt/homebrew/opt/qt@5)
	QT5_INTEL := $(shell test -f /usr/local/opt/qt@5/bin/qmake && echo /usr/local/opt/qt@5)
	QT5_PREFIX := $(if $(QT5_APPLE_SILICON),$(QT5_APPLE_SILICON),$(QT5_INTEL))
endif

# Default target
all: $(EXE)

# Build with CMake
$(EXE):
	@echo "=== Building Qt Client with CMake ==="
	@QT5_FOUND=0; \
	QMAKE_PATH=""; \
	if command -v qmake >/dev/null 2>&1; then \
		QT5_FOUND=1; \
		QMAKE_PATH=$$(command -v qmake); \
	elif command -v qmake5 >/dev/null 2>&1; then \
		QT5_FOUND=1; \
		QMAKE_PATH=$$(command -v qmake5); \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		if [ -f "/opt/homebrew/opt/qt@5/bin/qmake" ]; then \
			QT5_FOUND=1; \
			QMAKE_PATH="/opt/homebrew/opt/qt@5/bin/qmake"; \
			export PATH="/opt/homebrew/opt/qt@5/bin:$$PATH"; \
			echo "Found Qt5 at: /opt/homebrew/opt/qt@5 (Apple Silicon)"; \
		elif [ -f "/usr/local/opt/qt@5/bin/qmake" ]; then \
			QT5_FOUND=1; \
			QMAKE_PATH="/usr/local/opt/qt@5/bin/qmake"; \
			export PATH="/usr/local/opt/qt@5/bin:$$PATH"; \
			echo "Found Qt5 at: /usr/local/opt/qt@5 (Intel Mac)"; \
		fi; \
	fi; \
	if [ $$QT5_FOUND -eq 0 ]; then \
		echo ""; \
		echo "ERROR: Qt5 is not installed!"; \
		echo ""; \
		echo "Please install Qt5:"; \
		if [ "$(UNAME_S)" = "Darwin" ]; then \
			echo "  brew install qt@5"; \
		else \
			echo "  sudo apt-get update"; \
			echo "  sudo apt-get install -y qtbase5-dev qt5-qmake"; \
		fi; \
		echo ""; \
		echo "See BUILD_QT.md for detailed instructions."; \
		exit 1; \
	fi; \
	if [ -n "$$QMAKE_PATH" ]; then \
		echo "Using qmake: $$QMAKE_PATH"; \
		$$QMAKE_PATH --version | head -n1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@QT5_CMAKE_PREFIX=""; \
	if [ "$(UNAME_S)" = "Darwin" ]; then \
		if [ -d "/opt/homebrew/opt/qt@5" ]; then \
			QT5_CMAKE_PREFIX="/opt/homebrew/opt/qt@5"; \
		elif [ -d "/usr/local/opt/qt@5" ]; then \
			QT5_CMAKE_PREFIX="/usr/local/opt/qt@5"; \
		fi; \
	fi; \
	if [ -n "$$QT5_CMAKE_PREFIX" ]; then \
		export PATH="$$QT5_CMAKE_PREFIX/bin:$$PATH"; \
		cd $(BUILD_DIR) && cmake -DCMAKE_PREFIX_PATH=$$QT5_CMAKE_PREFIX .. && $(MAKE); \
	else \
		cd $(BUILD_DIR) && cmake .. && $(MAKE); \
	fi
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
	@QT5_FOUND=0; \
	QMAKE_CMD=""; \
	if command -v qmake >/dev/null 2>&1; then \
		QT5_FOUND=1; \
		QMAKE_CMD=qmake; \
	elif command -v qmake5 >/dev/null 2>&1; then \
		QT5_FOUND=1; \
		QMAKE_CMD=qmake5; \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		if [ -f "/opt/homebrew/opt/qt@5/bin/qmake" ]; then \
			QT5_FOUND=1; \
			QMAKE_CMD="/opt/homebrew/opt/qt@5/bin/qmake"; \
			export PATH="/opt/homebrew/opt/qt@5/bin:$$PATH"; \
		elif [ -f "/usr/local/opt/qt@5/bin/qmake" ]; then \
			QT5_FOUND=1; \
			QMAKE_CMD="/usr/local/opt/qt@5/bin/qmake"; \
			export PATH="/usr/local/opt/qt@5/bin:$$PATH"; \
		fi; \
	fi; \
	if [ $$QT5_FOUND -eq 1 ]; then \
		echo "✓ qmake found: $$QMAKE_CMD"; \
		$$QMAKE_CMD --version | head -n1; \
		if $$QMAKE_CMD -query QT_INSTALL_PREFIX >/dev/null 2>&1; then \
			echo "  Qt path: $$($$QMAKE_CMD -query QT_INSTALL_PREFIX)"; \
		fi; \
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
