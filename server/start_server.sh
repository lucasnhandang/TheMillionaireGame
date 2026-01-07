#!/bin/bash
# Script to build and start the Millionaire Game Server

cd "$(dirname "$0")"

echo "=== Millionaire Game Server ==="
echo ""

# Check if g++ is installed
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found. Please install build-essential:"
    echo "  sudo apt-get install build-essential g++"
    exit 1
fi

# Clean and build
echo "Building server..."
make clean
make

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Build successful!"
echo ""

# Check if config.json exists
if [ ! -f "config.json" ]; then
    echo "Warning: config.json not found. Creating from example..."
    if [ -f "config.json.example" ]; then
        cp config.json.example config.json
        echo "Created config.json from example"
    else
        echo "Warning: config.json.example not found. Using defaults."
    fi
fi

# Start server
echo "Starting server..."
echo "Press Ctrl+C to stop"
echo ""

./bin/server

