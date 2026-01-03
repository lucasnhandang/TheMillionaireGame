-- Add lifeline info fields to questions table
ALTER TABLE questions 
ADD COLUMN IF NOT EXISTS lifeline_5050_info VARCHAR(10), -- e.g., "1,2" means remove options A and B
ADD COLUMN IF NOT EXISTS lifeline_call_info TEXT, -- Phone a friend suggestion text
ADD COLUMN IF NOT EXISTS lifeline_ask_info VARCHAR(8); -- e.g., "10088002" means 10% A, 8% B, 80% C, 2% D

-- Add comment for documentation
COMMENT ON COLUMN questions.lifeline_5050_info IS 'Comma-separated option indices (0-3) to remove for 50:50 lifeline';
COMMENT ON COLUMN questions.lifeline_call_info IS 'Phone a friend suggestion text with answer at the end';
COMMENT ON COLUMN questions.lifeline_ask_info IS '8-digit string representing audience poll percentages for A,B,C,D';

