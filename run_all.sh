#!/bin/bash
# Script to run both server and client together

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Who Wants to be a Millionaire - Full Setup ===${NC}"
echo ""

# Function to check command
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}Error: $1 not found${NC}"
        return 1
    fi
    return 0
}

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

if ! check_command g++; then
    echo "Installing g++..."
    sudo apt-get install -y build-essential g++ make
fi

if ! check_command python3; then
    echo "Installing python3..."
    sudo apt-get install -y python3 python3-pip
fi

python3 -c "import tkinter" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Installing python3-tk..."
    sudo apt-get install -y python3-tk
fi

echo -e "${GREEN}Dependencies OK${NC}"
echo ""

# Build server
echo -e "${YELLOW}Building server...${NC}"
cd server

# Create config.json if not exists
if [ ! -f "config.json" ]; then
    if [ -f "config.json.example" ]; then
        cp config.json.example config.json
        echo "Created config.json from example"
    fi
fi

make clean
make

if [ $? -ne 0 ]; then
    echo -e "${RED}Server build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Server build successful!${NC}"
echo ""

# Start server in background
echo -e "${YELLOW}Starting server in background...${NC}"
./bin/server > server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Check if server started
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}Server failed to start! Check server.log${NC}"
    tail -n 20 server.log
    exit 1
fi

echo -e "${GREEN}Server started successfully!${NC}"
echo ""

# Check if port is listening
if command -v netstat &> /dev/null; then
    if netstat -tuln 2>/dev/null | grep -q ":8080"; then
        echo -e "${GREEN}Server is listening on port 8080${NC}"
    fi
elif command -v ss &> /dev/null; then
    if ss -tuln 2>/dev/null | grep -q ":8080"; then
        echo -e "${GREEN}Server is listening on port 8080${NC}"
    fi
fi

echo ""

# Start client
echo -e "${YELLOW}Starting client...${NC}"
echo -e "${BLUE}Close client window to stop both server and client${NC}"
echo ""

cd ../client
python3 main.py

# Cleanup
echo ""
echo -e "${YELLOW}Shutting down server...${NC}"
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "${GREEN}Done!${NC}"

