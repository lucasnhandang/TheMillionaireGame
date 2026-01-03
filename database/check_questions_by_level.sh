#!/bin/bash

echo "========================================"
echo "   KIEM TRA CAU HOI THEO LEVEL"
echo "========================================"
echo ""

echo "1. Tong so cau hoi:"
sudo -u postgres psql millionaire_game -c "SELECT COUNT(*) as total FROM questions;" 2>/dev/null

echo ""
echo "2. So cau hoi moi level:"
sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;" 2>/dev/null

echo ""
echo "3. Chi tiet cau hoi level 1 (5 cau dau tien):"
sudo -u postgres psql millionaire_game -c "SELECT id, level, LEFT(question_text, 50) as question FROM questions WHERE level = 1 LIMIT 5;" 2>/dev/null

echo ""
echo "4. Test query getRandomQuestion (level 1):"
sudo -u postgres psql millionaire_game -c "SELECT id, question_text, option_a, option_b, option_c, option_d, correct_answer, level FROM questions WHERE level = 1 ORDER BY RANDOM() LIMIT 1;" 2>/dev/null

echo ""
echo "========================================"
echo ""


