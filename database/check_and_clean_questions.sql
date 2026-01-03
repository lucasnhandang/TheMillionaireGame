-- Script to check and clean questions in database
-- 1. Check questions with missing/null data
-- 2. Delete questions with empty/null critical fields
-- 3. Fill missing lifeline fields with default values

-- ============================================
-- STEP 1: Check questions with missing data
-- ============================================
SELECT '=== QUESTIONS WITH MISSING DATA ===' as info;

-- Questions with NULL or empty question_text
SELECT id, level, 
       CASE WHEN question_text IS NULL OR question_text = '' THEN 'MISSING' ELSE 'OK' END as question_text_status,
       CASE WHEN option_a IS NULL OR option_a = '' THEN 'MISSING' ELSE 'OK' END as option_a_status,
       CASE WHEN option_b IS NULL OR option_b = '' THEN 'MISSING' ELSE 'OK' END as option_b_status,
       CASE WHEN option_c IS NULL OR option_c = '' THEN 'MISSING' ELSE 'OK' END as option_c_status,
       CASE WHEN option_d IS NULL OR option_d = '' THEN 'MISSING' ELSE 'OK' END as option_d_status,
       CASE WHEN correct_answer IS NULL THEN 'MISSING' ELSE 'OK' END as correct_answer_status
FROM questions
WHERE question_text IS NULL OR question_text = '' 
   OR option_a IS NULL OR option_a = ''
   OR option_b IS NULL OR option_b = ''
   OR option_c IS NULL OR option_c = ''
   OR option_d IS NULL OR option_d = ''
   OR correct_answer IS NULL;

-- Count questions with missing data
SELECT COUNT(*) as questions_with_missing_data
FROM questions
WHERE question_text IS NULL OR question_text = '' 
   OR option_a IS NULL OR option_a = ''
   OR option_b IS NULL OR option_b = ''
   OR option_c IS NULL OR option_c = ''
   OR option_d IS NULL OR option_d = ''
   OR correct_answer IS NULL;

-- ============================================
-- STEP 2: Check lifeline fields
-- ============================================
SELECT '=== QUESTIONS WITH MISSING LIFELINE DATA ===' as info;

SELECT id, level,
       CASE WHEN lifeline_5050_info IS NULL THEN 'NULL' ELSE 'OK' END as lifeline_5050_status,
       CASE WHEN lifeline_call_info IS NULL THEN 'NULL' ELSE 'OK' END as lifeline_call_status,
       CASE WHEN lifeline_ask_info IS NULL THEN 'NULL' ELSE 'OK' END as lifeline_ask_status
FROM questions
WHERE lifeline_5050_info IS NULL 
   OR lifeline_call_info IS NULL 
   OR lifeline_ask_info IS NULL;

SELECT COUNT(*) as questions_with_missing_lifeline_data
FROM questions
WHERE lifeline_5050_info IS NULL 
   OR lifeline_call_info IS NULL 
   OR lifeline_ask_info IS NULL;

-- ============================================
-- STEP 3: DELETE questions with critical missing data
-- ============================================
-- WARNING: This will delete questions permanently!
-- Uncomment the following lines to execute:

/*
DELETE FROM questions
WHERE question_text IS NULL OR question_text = '' 
   OR option_a IS NULL OR option_a = ''
   OR option_b IS NULL OR option_b = ''
   OR option_c IS NULL OR option_c = ''
   OR option_d IS NULL OR option_d = ''
   OR correct_answer IS NULL;
*/

-- ============================================
-- STEP 4: FILL missing lifeline fields with defaults
-- ============================================
-- This will fill NULL lifeline fields with default values
-- Uncomment to execute:

/*
-- Fill 50:50 info (remove 2 random wrong answers)
-- Format: "0,1" means remove options A and B
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

-- Fill call info (phone a friend suggestion)
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

-- Fill ask info (audience poll percentages)
-- Format: 8 digits representing percentages for A,B,C,D
-- Example: "10088002" = 10% A, 8% B, 80% C, 2% D
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
*/

-- ============================================
-- STEP 5: Final check - all questions should be complete
-- ============================================
SELECT '=== FINAL CHECK: ALL QUESTIONS ===' as info;

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

-- Questions by level
SELECT level, COUNT(*) as count
FROM questions
GROUP BY level
ORDER BY level;

