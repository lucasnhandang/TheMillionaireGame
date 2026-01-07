#!/bin/bash

echo "========================================"
echo "   DEBUG: ADD QUESTIONS"
echo "========================================"
echo ""

DB_NAME="millionaire_game"

# Check if database exists
echo "1. Checking if database exists..."
if sudo -u postgres psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME; then
    echo "   ✓ Database $DB_NAME exists"
else
    echo "   ✗ Database $DB_NAME does NOT exist!"
    echo "   Run: ./setup_database.sh"
    exit 1
fi

# Check current question count
echo ""
echo "2. Current question count BEFORE adding:"
sudo -u postgres psql $DB_NAME -c "SELECT COUNT(*) as count FROM questions;" 2>&1

# Check if questions table exists
echo ""
echo "3. Checking if questions table exists:"
sudo -u postgres psql $DB_NAME -c "\d questions" 2>&1 | head -20

# Try to add lifeline fields
echo ""
echo "4. Adding lifeline fields (if not exist):"
sudo -u postgres psql $DB_NAME -c "
ALTER TABLE questions 
ADD COLUMN IF NOT EXISTS lifeline_5050_info VARCHAR(10),
ADD COLUMN IF NOT EXISTS lifeline_call_info TEXT,
ADD COLUMN IF NOT EXISTS lifeline_ask_info VARCHAR(8);
" 2>&1

# Check which file exists
echo ""
echo "5. Checking which SQL file exists:"
if [ -f "add_questions_with_lifelines.sql" ]; then
    echo "   ✓ Found add_questions_with_lifelines.sql"
    SQL_FILE="add_questions_with_lifelines.sql"
elif [ -f "add_sample_questions.sql" ]; then
    echo "   ✓ Found add_sample_questions.sql"
    SQL_FILE="add_sample_questions.sql"
else
    echo "   ✗ No SQL file found!"
    exit 1
fi

# Try to add questions
echo ""
echo "6. Adding questions from $SQL_FILE..."
echo "   (This may take a moment...)"

# Show first few lines of SQL file
echo ""
echo "   First 5 lines of SQL file:"
head -5 $SQL_FILE

# Execute SQL file
sudo -u postgres psql $DB_NAME < $SQL_FILE 2>&1 | tee /tmp/add_questions_output.log

# Check result
echo ""
echo "7. Checking result..."
RESULT=$(sudo -u postgres psql $DB_NAME -t -c "SELECT COUNT(*) FROM questions;" 2>&1 | xargs)
echo "   Questions count AFTER adding: $RESULT"

if [ "$RESULT" -gt 0 ]; then
    echo "   ✓ Questions added successfully!"
    echo ""
    echo "8. Questions by level:"
    sudo -u postgres psql $DB_NAME -c "
    SELECT level, COUNT(*) as count 
    FROM questions 
    GROUP BY level 
    ORDER BY level;
    " 2>&1
else
    echo "   ✗ No questions found! Check error log above."
    echo ""
    echo "   Error output:"
    cat /tmp/add_questions_output.log | grep -i error
fi

echo ""
echo "========================================"

