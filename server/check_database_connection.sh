#!/bin/bash

echo "========================================"
echo "   KIEM TRA DATABASE CONNECTION"
echo "========================================"
echo ""

# Check PostgreSQL service
echo "1. Checking PostgreSQL service..."
if systemctl is-active --quiet postgresql; then
    echo "   ✓ PostgreSQL is running"
else
    echo "   ✗ PostgreSQL is NOT running"
    echo "   Start with: sudo systemctl start postgresql"
fi
echo ""

# Check database exists
echo "2. Checking database 'millionaire_game'..."
if sudo -u postgres psql -lqt | cut -d \| -f 1 | grep -qw millionaire_game; then
    echo "   ✓ Database 'millionaire_game' exists"
else
    echo "   ✗ Database 'millionaire_game' does NOT exist"
    echo "   Create with: sudo -u postgres createdb millionaire_game"
fi
echo ""

# Test connection
echo "3. Testing database connection..."
if sudo -u postgres psql millionaire_game -c "SELECT 1;" > /dev/null 2>&1; then
    echo "   ✓ Can connect to database"
else
    echo "   ✗ Cannot connect to database"
fi
echo ""

# Check questions
echo "4. Checking questions in database..."
COUNT=$(sudo -u postgres psql millionaire_game -t -c "SELECT COUNT(*) FROM questions;" 2>/dev/null | tr -d ' ')
if [ -n "$COUNT" ] && [ "$COUNT" -gt 0 ]; then
    echo "   ✓ Found $COUNT questions in database"
    echo ""
    echo "   Questions by level:"
    sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;" 2>/dev/null
else
    echo "   ✗ No questions found in database"
    echo "   Add questions with: cd database && sudo -u postgres psql millionaire_game < add_sample_questions.sql"
fi
echo ""

echo "========================================"
echo "   KET LUAN"
echo "========================================"
echo ""
echo "Neu co loi, kiem tra:"
echo "  1. PostgreSQL da chay: sudo systemctl status postgresql"
echo "  2. Database da duoc tao: sudo -u postgres psql -l | grep millionaire_game"
echo "  3. Co questions trong database: sudo -u postgres psql millionaire_game -c 'SELECT COUNT(*) FROM questions;'"
echo ""

