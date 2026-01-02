#!/bin/bash

# Script to clean and rebuild server
# Usage: ./rebuild.sh

echo "========================================"
echo "   REBUILDING SERVER"
echo "========================================"
echo ""

# Step 1: Clean
echo "Step 1: Cleaning old build files..."
make clean
if [ $? -ne 0 ]; then
    echo "Warning: make clean failed, trying manual cleanup..."
    rm -rf obj bin
    find . -name "*.o" -type f -delete 2>/dev/null || true
fi

echo ""

# Step 2: Create directories
echo "Step 2: Creating build directories..."
mkdir -p obj bin

echo ""

# Step 3: Build
echo "Step 3: Building server..."
make

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "   BUILD SUCCESSFUL!"
    echo "========================================"
    echo ""
    echo "Server executable: bin/server"
    echo ""
    echo "To run server:"
    echo "  ./bin/server"
    echo ""
else
    echo ""
    echo "========================================"
    echo "   BUILD FAILED!"
    echo "========================================"
    echo ""
    echo "Please check the error messages above."
    echo ""
    exit 1
fi

