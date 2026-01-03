#!/bin/bash

echo "========================================"
echo "   CLEAN AND FIX QUESTIONS DATABASE"
echo "========================================"
echo ""

DB_NAME="millionaire_game"
DB_USER="postgres"

# Check if running as root or with sudo
if [ "$EUID" -eq 0 ]; then
    PSQL_CMD="sudo -u postgres psql"
else
    PSQL_CMD="psql -U $DB_USER"
fi

echo "1. Checking questions with missing data..."
$PSQL_CMD $DB_NAME -c "
SELECT COUNT(*) as questions_with_missing_data
FROM questions
WHERE question_text IS NULL OR question_text = '' 
   OR option_a IS NULL OR option_a = ''
   OR option_b IS NULL OR option_b = ''
   OR option_c IS NULL OR option_c = ''
   OR option_d IS NULL OR option_d = ''
   OR correct_answer IS NULL;
" 2>/dev/null

echo ""
echo "2. Checking questions with missing lifeline data..."
$PSQL_CMD $DB_NAME -c "
SELECT COUNT(*) as questions_with_missing_lifeline_data
FROM questions
WHERE lifeline_5050_info IS NULL 
   OR lifeline_call_info IS NULL 
   OR lifeline_ask_info IS NULL;
" 2>/dev/null

echo ""
read -p "Do you want to DELETE questions with missing critical data? (yes/no): " delete_confirm
if [ "$delete_confirm" = "yes" ]; then
    echo "Deleting questions with missing critical data..."
    $PSQL_CMD $DB_NAME -c "
    DELETE FROM questions
    WHERE question_text IS NULL OR question_text = '' 
       OR option_a IS NULL OR option_a = ''
       OR option_b IS NULL OR option_b = ''
       OR option_c IS NULL OR option_c = ''
       OR option_d IS NULL OR option_d = ''
       OR correct_answer IS NULL;
    " 2>/dev/null
    echo "Deleted!"
else
    echo "Skipped deletion."
fi

echo ""
read -p "Do you want to FILL missing lifeline fields? (yes/no): " fill_confirm
if [ "$fill_confirm" = "yes" ]; then
    echo "Filling missing lifeline fields..."
    
    # Fill 50:50 info
    echo "  - Filling lifeline_5050_info..."
    $PSQL_CMD $DB_NAME -c "
    UPDATE questions
    SET lifeline_5050_info = (
        CASE 
            WHEN correct_answer = 0 THEN '1,2'  -- Remove B and C
            WHEN correct_answer = 1 THEN '0,2'  -- Remove A and C
            WHEN correct_answer = 2 THEN '0,1'  -- Remove A and B
            WHEN correct_answer = 3 THEN '0,1'  -- Remove A and B
            ELSE '0,1'
        END
    )
    WHERE lifeline_5050_info IS NULL;
    " 2>/dev/null
    
    # Fill call info
    echo "  - Filling lifeline_call_info..."
    $PSQL_CMD $DB_NAME -c "
    UPDATE questions
    SET lifeline_call_info = (
        'Bạn của tôi nghĩ rằng đáp án đúng là ' || 
        CASE correct_answer
            WHEN 0 THEN 'A'
            WHEN 1 THEN 'B'
            WHEN 2 THEN 'C'
            WHEN 3 THEN 'D'
        END || '. ' ||
        'Tôi khá chắc chắn về điều này.'
    )
    WHERE lifeline_call_info IS NULL;
    " 2>/dev/null
    
    # Fill ask info
    echo "  - Filling lifeline_ask_info..."
    $PSQL_CMD $DB_NAME -c "
    UPDATE questions
    SET lifeline_ask_info = (
        CASE correct_answer
            WHEN 0 THEN '70001005'  -- 70% A, 1% B, 0% C, 5% D
            WHEN 1 THEN '01070005'  -- 1% A, 70% B, 0% C, 5% D
            WHEN 2 THEN '01050700'  -- 1% A, 5% B, 70% C, 0% D
            WHEN 3 THEN '01050070'  -- 1% A, 5% B, 0% C, 70% D
            ELSE '25252525'         -- Default: 25% each
        END
    )
    WHERE lifeline_ask_info IS NULL;
    " 2>/dev/null
    
    echo "Filled!"
else
    echo "Skipped filling."
fi

echo ""
echo "3. Final check - All questions status:"
$PSQL_CMD $DB_NAME -c "
SELECT 
    COUNT(*) as total_questions,
    COUNT(CASE WHEN question_text IS NOT NULL AND question_text != '' THEN 1 END) as with_question_text,
    COUNT(CASE WHEN option_a IS NOT NULL AND option_a != '' THEN 1 END) as with_option_a,
    COUNT(CASE WHEN option_b IS NOT NULL AND option_b != '' THEN 1 END) as with_option_b,
    COUNT(CASE WHEN option_c IS NOT NULL AND option_c != '' THEN 1 END) as with_option_c,
    COUNT(CASE WHEN option_d IS NOT NULL AND option_d != '' THEN 1 END) as with_option_d,
    COUNT(CASE WHEN correct_answer IS NOT NULL THEN 1 END) as with_correct_answer,
    COUNT(CASE WHEN lifeline_5050_info IS NOT NULL THEN 1 END) as with_lifeline_5050,
    COUNT(CASE WHEN lifeline_call_info IS NOT NULL THEN 1 END) as with_lifeline_call,
    COUNT(CASE WHEN lifeline_ask_info IS NOT NULL THEN 1 END) as with_lifeline_ask
FROM questions;
" 2>/dev/null

echo ""
echo "4. Questions by level:"
$PSQL_CMD $DB_NAME -c "
SELECT level, COUNT(*) as count
FROM questions
GROUP BY level
ORDER BY level;
" 2>/dev/null

echo ""
echo "========================================"
echo "Done!"
echo "========================================"

