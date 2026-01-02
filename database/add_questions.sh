#!/bin/bash

# Script to add questions from sample file
# Usage: ./add_questions.sh

echo "========================================"
echo "   THEM CAU HOI TU FILE SAMPLE"
echo "========================================"
echo ""

# Check current questions
echo "Kiem tra cau hoi hien co..."
sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;" 2>/dev/null

echo ""
echo "Dang them cau hoi tu file add_sample_questions.sql..."
echo ""

# Add questions
if [ -f "add_sample_questions.sql" ]; then
    sudo -u postgres psql millionaire_game < add_sample_questions.sql 2>&1 | grep -v "INSERT 0" | grep -v "^$"
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo ""
        echo "========================================"
        echo "   THEM CAU HOI THANH CONG!"
        echo "========================================"
        echo ""
        echo "Kiem tra ket qua:"
        sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"
    else
        echo ""
        echo "Co loi khi them cau hoi!"
        echo "Co the mot so cau hoi da ton tai."
    fi
else
    echo "Khong tim thay file add_sample_questions.sql!"
    exit 1
fi

echo ""
echo "De xem chi tiet cau hoi:"
echo "  psql -U postgres millionaire_game -c \"SELECT id, level, question_text FROM questions ORDER BY level;\""
echo ""

