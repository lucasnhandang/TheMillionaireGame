-- Mock Data for Millionaire Game Database
-- Run this after schema.sql to populate test data

-- ============================================
-- USERS (password_hash is bcrypt hash of "password123")
-- ============================================
INSERT INTO users (username, password_hash, role, is_banned, created_at) VALUES
('admin1', '$2b$10$rQZ8K5K5K5K5K5K5K5K5K.5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K', 'admin', FALSE, CURRENT_TIMESTAMP),
('player1', '$2b$10$rQZ8K5K5K5K5K5K5K5K5K.5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K', 'user', FALSE, CURRENT_TIMESTAMP),
('player2', '$2b$10$rQZ8K5K5K5K5K5K5K5K5K.5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K', 'user', FALSE, CURRENT_TIMESTAMP),
('player3', '$2b$10$rQZ8K5K5K5K5K5K5K5K5K.5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K', 'user', FALSE, CURRENT_TIMESTAMP),
('banned_user', '$2b$10$rQZ8K5K5K5K5K5K5K5K5K.5K5K5K5K5K5K5K5K5K5K5K5K5K5K5K', 'user', TRUE, CURRENT_TIMESTAMP)
ON CONFLICT (username) DO NOTHING;

-- Note: In production, use proper bcrypt hashing. For testing, you can use:
-- python3 -c "import bcrypt; print(bcrypt.hashpw(b'password123', bcrypt.gensalt()).decode())"

-- ============================================
-- QUESTIONS (Sample questions for levels 0-2)
-- Level 0 = Easy, Level 1 = Medium, Level 2 = Hard
-- ============================================

-- Level 0 Questions (Easy)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level, is_active, created_at) VALUES
('What is the capital of France?', 'London', 'Paris', 'Berlin', 'Madrid', 1, 0, TRUE, CURRENT_TIMESTAMP),
('What is 2 + 2?', '3', '4', '5', '6', 1, 0, TRUE, CURRENT_TIMESTAMP),
('What is the largest planet in our solar system?', 'Earth', 'Mars', 'Jupiter', 'Saturn', 2, 0, TRUE, CURRENT_TIMESTAMP),
('How many continents are there?', '5', '6', '7', '8', 2, 0, TRUE, CURRENT_TIMESTAMP),
('What is the chemical symbol for water?', 'H2O', 'CO2', 'O2', 'NaCl', 0, 0, TRUE, CURRENT_TIMESTAMP),
('Who wrote "Romeo and Juliet"?', 'Charles Dickens', 'William Shakespeare', 'Jane Austen', 'Mark Twain', 1, 0, TRUE, CURRENT_TIMESTAMP),
('What is the speed of light in vacuum?', '299,792,458 m/s', '300,000,000 m/s', '299,000,000 m/s', '301,000,000 m/s', 0, 0, TRUE, CURRENT_TIMESTAMP),
('In which year did World War II end?', '1943', '1944', '1945', '1946', 2, 0, TRUE, CURRENT_TIMESTAMP),
('What is the smallest prime number?', '0', '1', '2', '3', 2, 0, TRUE, CURRENT_TIMESTAMP),
('Which ocean is the largest?', 'Atlantic', 'Indian', 'Arctic', 'Pacific', 3, 0, TRUE, CURRENT_TIMESTAMP),
('What is the chemical symbol for gold?', 'Go', 'Gd', 'Au', 'Ag', 2, 0, TRUE, CURRENT_TIMESTAMP),
('Who painted the Mona Lisa?', 'Vincent van Gogh', 'Leonardo da Vinci', 'Pablo Picasso', 'Michelangelo', 1, 0, TRUE, CURRENT_TIMESTAMP),
('What is the longest river in the world?', 'Amazon', 'Nile', 'Yangtze', 'Mississippi', 1, 0, TRUE, CURRENT_TIMESTAMP),
('In which country is Mount Everest located?', 'India', 'Nepal', 'China', 'Bhutan', 1, 0, TRUE, CURRENT_TIMESTAMP),
('What is the square root of 144?', '10', '11', '12', '13', 2, 0, TRUE, CURRENT_TIMESTAMP);

-- Level 1 Questions (Medium)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level, is_active, created_at) VALUES
('What is the capital of Australia?', 'Sydney', 'Melbourne', 'Canberra', 'Perth', 2, 1, TRUE, CURRENT_TIMESTAMP),
('Who discovered penicillin?', 'Louis Pasteur', 'Alexander Fleming', 'Marie Curie', 'Robert Koch', 1, 1, TRUE, CURRENT_TIMESTAMP),
('What is the hardest natural substance on Earth?', 'Gold', 'Iron', 'Diamond', 'Platinum', 2, 1, TRUE, CURRENT_TIMESTAMP),
('In which year did the Berlin Wall fall?', '1987', '1988', '1989', '1990', 2, 1, TRUE, CURRENT_TIMESTAMP),
('What is the molecular formula for glucose?', 'C6H12O6', 'C6H6O6', 'C12H22O11', 'CH4', 0, 1, TRUE, CURRENT_TIMESTAMP),
('Who wrote "1984"?', 'Aldous Huxley', 'George Orwell', 'Ray Bradbury', 'H.G. Wells', 1, 1, TRUE, CURRENT_TIMESTAMP),
('What is the speed of sound in air at room temperature?', '330 m/s', '343 m/s', '350 m/s', '360 m/s', 1, 1, TRUE, CURRENT_TIMESTAMP),
('In which year did the first man land on the moon?', '1967', '1968', '1969', '1970', 2, 1, TRUE, CURRENT_TIMESTAMP),
('What is the largest mammal in the world?', 'African Elephant', 'Blue Whale', 'Giraffe', 'Hippopotamus', 1, 1, TRUE, CURRENT_TIMESTAMP),
('What is the chemical symbol for silver?', 'Si', 'Sv', 'Ag', 'Au', 2, 1, TRUE, CURRENT_TIMESTAMP);

-- Level 2 Questions (Hard)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level, is_active, created_at) VALUES
('Who composed "The Four Seasons"?', 'Johann Sebastian Bach', 'Antonio Vivaldi', 'Wolfgang Amadeus Mozart', 'Ludwig van Beethoven', 1, 2, TRUE, CURRENT_TIMESTAMP),
('What is the approximate age of the universe?', '10 billion years', '13.8 billion years', '15 billion years', '20 billion years', 1, 2, TRUE, CURRENT_TIMESTAMP),
('In which year was the first iPhone released?', '2005', '2006', '2007', '2008', 2, 2, TRUE, CURRENT_TIMESTAMP),
('What is the smallest country in the world by area?', 'Monaco', 'Vatican City', 'San Marino', 'Liechtenstein', 1, 2, TRUE, CURRENT_TIMESTAMP),
('Who won the Nobel Prize in Physics in 2020?', 'Roger Penrose, Reinhard Genzel, Andrea Ghez', 'Albert Einstein', 'Stephen Hawking', 'Niels Bohr', 0, 2, TRUE, CURRENT_TIMESTAMP),
('What is the Heisenberg Uncertainty Principle?', 'Energy cannot be created or destroyed', 'Position and momentum cannot be simultaneously measured precisely', 'Light behaves as both wave and particle', 'Matter and energy are equivalent', 1, 2, TRUE, CURRENT_TIMESTAMP);

-- ============================================
-- FRIENDSHIPS (Sample friendships)
-- ============================================
INSERT INTO friendships (user1_id, user2_id, created_at) VALUES
(2, 3, CURRENT_TIMESTAMP)  -- player1 and player2 are friends
ON CONFLICT (user1_id, user2_id) DO NOTHING;

-- ============================================
-- FRIEND REQUESTS (Sample pending requests)
-- ============================================
INSERT INTO friend_requests (from_user_id, to_user_id, status, created_at) VALUES
(2, 4, 'pending', CURRENT_TIMESTAMP),  -- player1 sent request to player3
(3, 4, 'pending', CURRENT_TIMESTAMP)   -- player2 sent request to player3
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

-- ============================================
-- LEADERBOARD (Sample leaderboard entries)
-- ============================================
INSERT INTO leaderboard (user_id, final_question_number, total_score, highest_prize, games_played, last_updated) VALUES
(2, 15, 450, 1000000000, 5, CURRENT_TIMESTAMP),  -- player1: won game, 450 points
(3, 10, 280, 10000000, 3, CURRENT_TIMESTAMP),    -- player2: reached question 10, 280 points
(4, 8, 200, 5000000, 2, CURRENT_TIMESTAMP)       -- player3: reached question 8, 200 points
ON CONFLICT (user_id) DO NOTHING;

-- ============================================
-- MESSAGES (Sample chat messages)
-- ============================================
INSERT INTO messages (from_user_id, to_user_id, content, created_at) VALUES
(2, 3, 'Hey! Want to play a game?', CURRENT_TIMESTAMP),
(3, 2, 'Sure! Let''s compete!', CURRENT_TIMESTAMP),
(2, 4, 'Good luck on your game!', CURRENT_TIMESTAMP);

-- ============================================
-- GAME SESSIONS (Sample completed games)
-- ============================================
INSERT INTO game_sessions (user_id, status, current_question_number, current_level, current_prize, total_score, final_prize, started_at, ended_at) VALUES
(2, 'won', 15, 15, 1000000000, 450, 1000000000, CURRENT_TIMESTAMP - INTERVAL '1 hour', CURRENT_TIMESTAMP - INTERVAL '30 minutes'),
(3, 'lost', 10, 10, 10000000, 280, 10000000, CURRENT_TIMESTAMP - INTERVAL '2 hours', CURRENT_TIMESTAMP - INTERVAL '1 hour 30 minutes'),
(4, 'lost', 8, 8, 5000000, 200, 5000000, CURRENT_TIMESTAMP - INTERVAL '3 hours', CURRENT_TIMESTAMP - INTERVAL '2 hours 30 minutes')
ON CONFLICT DO NOTHING;

-- Note: game_questions and game_answers would be populated during actual gameplay
-- These are just examples of the structure

