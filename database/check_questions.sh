#!/bin/bash

# Script to check questions in database
# Usage: ./check_questions.sh

echo "========================================"
echo "   KIEM TRA CAU HOI TRONG DATABASE"
echo "========================================"
echo ""

# Check total questions
echo "Tong so cau hoi:"
sudo -u postgres psql millionaire_game -c "SELECT COUNT(*) as total FROM questions;"
echo ""

# Check questions per level
echo "So cau hoi moi level:"
sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"
echo ""

# Show sample questions
echo "Mau cau hoi (5 cau dau tien):"
sudo -u postgres psql millionaire_game -c "SELECT id, level, LEFT(question_text, 60) as question FROM questions ORDER BY level, id LIMIT 5;"
echo ""

