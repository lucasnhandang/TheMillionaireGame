#!/bin/bash

echo "========================================"
echo "   THEM CAU HOI VAO DATABASE"
echo "========================================"
echo ""

DB_NAME="millionaire_game"

# Check if database exists
if ! sudo -u postgres psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME; then
    echo "Database $DB_NAME chua ton tai!"
    echo "Chay script setup_database.sh truoc:"
    echo "  cd database"
    echo "  ./setup_database.sh"
    exit 1
fi

# Check if lifeline fields exist, if not add them
echo "1. Kiem tra lifeline fields..."
sudo -u postgres psql $DB_NAME -c "
ALTER TABLE questions 
ADD COLUMN IF NOT EXISTS lifeline_5050_info VARCHAR(10),
ADD COLUMN IF NOT EXISTS lifeline_call_info TEXT,
ADD COLUMN IF NOT EXISTS lifeline_ask_info VARCHAR(8);
" 2>/dev/null

echo "2. Dang them cau hoi tu file add_questions_with_lifelines.sql..."

if [ -f "add_questions_with_lifelines.sql" ]; then
    sudo -u postgres psql $DB_NAME < add_questions_with_lifelines.sql 2>&1 | grep -v "INSERT 0" | grep -v "^$" | grep -v "ALTER TABLE"
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "3. Kiem tra so luong cau hoi:"
        sudo -u postgres psql $DB_NAME -c "
        SELECT level, COUNT(*) as count 
        FROM questions 
        GROUP BY level 
        ORDER BY level;
        " 2>/dev/null
        
        echo ""
        echo "4. Tong so cau hoi:"
        sudo -u postgres psql $DB_NAME -c "SELECT COUNT(*) as total FROM questions;" 2>/dev/null
        
        echo ""
        echo "========================================"
        echo "Them cau hoi thanh cong!"
        echo "========================================"
    else
        echo "Loi khi them cau hoi!"
        exit 1
    fi
else
    echo "Khong tim thay file add_questions_with_lifelines.sql!"
    echo "Dang su dung file add_sample_questions.sql..."
    
    if [ -f "add_sample_questions.sql" ]; then
        sudo -u postgres psql $DB_NAME < add_sample_questions.sql 2>&1 | grep -v "INSERT 0" | grep -v "^$"
        
        # Fill lifeline fields for existing questions
        echo ""
        echo "Dang them lifeline data cho cac cau hoi..."
        sudo -u postgres psql $DB_NAME -f add_lifeline_fields.sql 2>/dev/null
        
        sudo -u postgres psql $DB_NAME -c "
        UPDATE questions
        SET lifeline_5050_info = (
            CASE 
                WHEN correct_answer = 0 THEN '1,2'
                WHEN correct_answer = 1 THEN '0,2'
                WHEN correct_answer = 2 THEN '0,1'
                WHEN correct_answer = 3 THEN '0,1'
                ELSE '0,1'
            END
        )
        WHERE lifeline_5050_info IS NULL;
        
        UPDATE questions
        SET lifeline_call_info = (
            'Ban cua toi nghi rang dap an dung la ' || 
            CASE correct_answer
                WHEN 0 THEN 'A'
                WHEN 1 THEN 'B'
                WHEN 2 THEN 'C'
                WHEN 3 THEN 'D'
            END || '. ' ||
            'Toi kha chac chan ve dieu nay.'
        )
        WHERE lifeline_call_info IS NULL;
        
        UPDATE questions
        SET lifeline_ask_info = (
            CASE correct_answer
                WHEN 0 THEN '70001005'
                WHEN 1 THEN '01070005'
                WHEN 2 THEN '01050700'
                WHEN 3 THEN '01050070'
                ELSE '25252525'
            END
        )
        WHERE lifeline_ask_info IS NULL;
        " 2>/dev/null
        
        echo "Hoan thanh!"
    else
        echo "Khong tim thay file add_sample_questions.sql!"
        exit 1
    fi
fi
