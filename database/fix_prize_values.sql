-- Fix incorrect prize values in database
-- The old scoring_system.cpp was calculating wrong values (up to 1 billion)
-- The correct prize ladder max is $1,000,000

-- Prize Ladder Reference:
-- Q1: $100, Q2: $200, Q3: $300, Q4: $500, Q5: $1,000 (checkpoint)
-- Q6: $2,000, Q7: $4,000, Q8: $8,000, Q9: $16,000, Q10: $32,000 (checkpoint)
-- Q11: $64,000, Q12: $125,000, Q13: $250,000, Q14: $500,000, Q15: $1,000,000 (checkpoint)

-- Step 1: View current incorrect data
SELECT id, user_id, status, current_question_number, final_prize 
FROM game_sessions 
WHERE final_prize > 1000000
ORDER BY final_prize DESC;

-- Step 2: Fix final_prize based on current_question_number
-- Using the correct checkpoint values

-- For games that ended at Q15 (winners), set to $1,000,000
UPDATE game_sessions 
SET final_prize = 1000000 
WHERE current_question_number = 15 AND status = 'won' AND final_prize > 1000000;

-- For games that ended at Q10-Q14 (checkpoint Q10), set to $32,000
UPDATE game_sessions 
SET final_prize = 32000 
WHERE current_question_number >= 10 AND current_question_number < 15 
  AND status IN ('lost', 'quit') AND final_prize > 1000000;

-- For games that ended at Q5-Q9 (checkpoint Q5), set to $1,000
UPDATE game_sessions 
SET final_prize = 1000 
WHERE current_question_number >= 5 AND current_question_number < 10 
  AND status IN ('lost', 'quit') AND final_prize > 1000000;

-- For games that ended before Q5 (no checkpoint), set to $0
UPDATE game_sessions 
SET final_prize = 0 
WHERE current_question_number < 5 
  AND status IN ('lost', 'quit') AND final_prize > 1000000;

-- Step 3: Cap any remaining values above $1,000,000 (fallback)
UPDATE game_sessions 
SET final_prize = 1000000 
WHERE final_prize > 1000000;

-- Step 4: Verify the fix
SELECT id, user_id, status, current_question_number, final_prize 
FROM game_sessions 
WHERE final_prize IS NOT NULL
ORDER BY final_prize DESC
LIMIT 20;
