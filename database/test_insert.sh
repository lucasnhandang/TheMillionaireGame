#!/bin/bash

echo "Testing database insert..."

DB_NAME="millionaire_game"

# Test 1: Check if table exists
echo "1. Checking if questions table exists:"
sudo -u postgres psql $DB_NAME -c "\d questions" 2>&1 | head -5

# Test 2: Try a simple insert
echo ""
echo "2. Trying a simple INSERT:"
sudo -u postgres psql $DB_NAME -c "
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) 
VALUES ('Test question?', 'A', 'B', 'C', 'D', 0, 1);
" 2>&1

# Test 3: Check if insert worked
echo ""
echo "3. Checking if test question was inserted:"
sudo -u postgres psql $DB_NAME -c "SELECT COUNT(*) FROM questions WHERE question_text = 'Test question?';" 2>&1

# Test 4: Try insert with lifeline fields
echo ""
echo "4. Trying INSERT with lifeline fields:"
sudo -u postgres psql $DB_NAME -c "
INSERT INTO questions (
    question_text, option_a, option_b, option_c, option_d, 
    correct_answer, level, 
    lifeline_5050_info, lifeline_call_info, lifeline_ask_info
) VALUES (
    'Test question 2?', 'A', 'B', 'C', 'D', 
    1, 1,
    '0,2', 'Test call info', '70001005'
);
" 2>&1

# Test 5: Final count
echo ""
echo "5. Total questions now:"
sudo -u postgres psql $DB_NAME -c "SELECT COUNT(*) as total FROM questions;" 2>&1

