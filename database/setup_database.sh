#!/bin/bash

# Script to setup PostgreSQL database for Millionaire Game
# Usage: ./setup_database.sh

echo "========================================"
echo "   SETUP DATABASE"
echo "========================================"
echo ""

# Check if PostgreSQL is installed
if ! command -v psql &> /dev/null; then
    echo "PostgreSQL chua duoc cai dat!"
    echo "Cai dat PostgreSQL:"
    echo "  sudo apt-get install postgresql libpq-dev"
    exit 1
fi

# Check if running as root or with sudo
if [ "$EUID" -eq 0 ]; then
    POSTGRES_USER="postgres"
    USE_SUDO=""
else
    POSTGRES_USER="postgres"
    USE_SUDO="sudo -u postgres"
fi

echo "Step 1: Kiem tra PostgreSQL service..."
if ! $USE_SUDO systemctl is-active --quiet postgresql; then
    echo "PostgreSQL chua chay. Dang khoi dong..."
    $USE_SUDO systemctl start postgresql
    if [ $? -ne 0 ]; then
        echo "Khong the khoi dong PostgreSQL!"
        exit 1
    fi
fi

echo "PostgreSQL da chay."
echo ""

# Create database
echo "Step 2: Tao database 'millionaire_game'..."
$USE_SUDO createdb millionaire_game 2>/dev/null
if [ $? -eq 0 ]; then
    echo "Database 'millionaire_game' da duoc tao."
elif [ $? -eq 1 ]; then
    echo "Database 'millionaire_game' da ton tai, bo qua..."
else
    echo "Loi khi tao database!"
    exit 1
fi
echo ""

# Create schema
echo "Step 3: Tao schema..."
if [ -f "schema.sql" ]; then
    $USE_SUDO psql millionaire_game < schema.sql
    if [ $? -eq 0 ]; then
        echo "Schema da duoc tao thanh cong!"
    else
        echo "Loi khi tao schema!"
        exit 1
    fi
else
    echo "Khong tim thay file schema.sql!"
    exit 1
fi
echo ""

# Add sample questions
echo "Step 4: Them cau hoi mau..."
if [ -f "add_sample_questions.sql" ]; then
    $USE_SUDO psql millionaire_game < add_sample_questions.sql
    if [ $? -eq 0 ]; then
        echo "Cau hoi mau da duoc them thanh cong!"
    else
        echo "Loi khi them cau hoi!"
        exit 1
    fi
else
    echo "Khong tim thay file add_sample_questions.sql!"
    exit 1
fi
echo ""

# Verify
echo "Step 5: Kiem tra..."
$USE_SUDO psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"

echo ""
echo "========================================"
echo "   SETUP HOAN TAT!"
echo "========================================"
echo ""
echo "Database: millionaire_game"
echo "User: postgres"
echo ""
echo "De ket noi:"
echo "  psql -U postgres millionaire_game"
echo ""

