#!/bin/bash

echo "========================================"
echo "   KIEM TRA DEPENDENCIES"
echo "========================================"
echo ""

# Check libpq-dev
echo "1. Checking libpq-dev..."
if dpkg -l | grep -q libpq-dev; then
    echo "   ✓ libpq-dev is installed"
else
    echo "   ✗ libpq-dev is NOT installed"
    echo "   Install with: sudo apt-get install libpq-dev"
fi
echo ""

# Check PostgreSQL header file
echo "2. Checking libpq-fe.h..."
if [ -f "/usr/include/postgresql/libpq-fe.h" ]; then
    echo "   ✓ Found: /usr/include/postgresql/libpq-fe.h"
elif [ -f "/usr/include/libpq-fe.h" ]; then
    echo "   ✓ Found: /usr/include/libpq-fe.h"
else
    echo "   ✗ libpq-fe.h not found in standard locations"
    echo "   Searching..."
    FOUND=$(find /usr -name "libpq-fe.h" 2>/dev/null | head -1)
    if [ -n "$FOUND" ]; then
        echo "   ✓ Found: $FOUND"
    else
        echo "   ✗ libpq-fe.h not found anywhere"
        echo "   Install with: sudo apt-get install libpq-dev"
    fi
fi
echo ""

# Check pg_config
echo "3. Checking pg_config..."
if command -v pg_config &> /dev/null; then
    echo "   ✓ pg_config found"
    echo "   Include dir: $(pg_config --includedir)"
    echo "   Library dir: $(pg_config --libdir)"
else
    echo "   ✗ pg_config not found"
fi
echo ""

# Check PostgreSQL libraries
echo "4. Checking PostgreSQL libraries..."
if [ -f "/usr/lib/x86_64-linux-gnu/libpq.so" ] || [ -f "/usr/lib/libpq.so" ]; then
    echo "   ✓ libpq.so found"
else
    echo "   ✗ libpq.so not found"
fi
echo ""

echo "========================================"
echo "   KET LUAN"
echo "========================================"
echo ""
echo "Neu thieu dependencies, chay:"
echo "  sudo apt-get install libpq-dev"
echo ""

