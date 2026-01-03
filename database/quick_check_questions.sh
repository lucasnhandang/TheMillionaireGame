#!/bin/bash

echo "========================================"
echo "   QUICK CHECK QUESTIONS DATABASE"
echo "========================================"
echo ""

DB_NAME="millionaire_game"

echo "1. Total questions:"
sudo -u postgres psql $DB_NAME -c "SELECT COUNT(*) as total FROM questions;" 2>/dev/null

echo ""
echo "2. Questions with missing critical data:"
sudo -u postgres psql $DB_NAME -c "
SELECT id, level, 
       CASE WHEN question_text IS NULL OR question_text = '' THEN 'MISSING' ELSE 'OK' END as question_text,
       CASE WHEN option_a IS NULL OR option_a = '' THEN 'MISSING' ELSE 'OK' END as option_a,
       CASE WHEN option_b IS NULL OR option_b = '' THEN 'MISSING' ELSE 'OK' END as option_b,
       CASE WHEN option_c IS NULL OR option_c = '' THEN 'MISSING' ELSE 'OK' END as option_c,
       CASE WHEN option_d IS NULL OR option_d = '' THEN 'MISSING' ELSE 'OK' END as option_d,
       CASE WHEN correct_answer IS NULL THEN 'MISSING' ELSE 'OK' END as correct_answer
FROM questions
WHERE question_text IS NULL OR question_text = '' 
   OR option_a IS NULL OR option_a = ''
   OR option_b IS NULL OR option_b = ''
   OR option_c IS NULL OR option_c = ''
   OR option_d IS NULL OR option_d = ''
   OR correct_answer IS NULL
LIMIT 10;
" 2>/dev/null

echo ""
echo "3. Questions with missing lifeline data:"
sudo -u postgres psql $DB_NAME -c "
SELECT COUNT(*) as count
FROM questions
WHERE lifeline_5050_info IS NULL 
   OR lifeline_call_info IS NULL 
   OR lifeline_ask_info IS NULL;
" 2>/dev/null

echo ""
echo "4. Questions by level:"
sudo -u postgres psql $DB_NAME -c "
SELECT level, COUNT(*) as count
FROM questions
GROUP BY level
ORDER BY level;
" 2>/dev/null

echo ""
echo "========================================"

