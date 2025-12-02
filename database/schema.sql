-- Millionaire Game Database Schema
-- Unified PostgreSQL Database Schema
-- Combines best elements from both design approaches

-- ============================================
-- 1. USERS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(10) DEFAULT 'user' CHECK (role IN ('user', 'admin')),
    is_banned BOOLEAN DEFAULT FALSE,
    ban_reason TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);

-- ============================================
-- 2. QUESTIONS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS questions (
    id SERIAL PRIMARY KEY,
    question_text TEXT NOT NULL,
    option_a TEXT NOT NULL,
    option_b TEXT NOT NULL,
    option_c TEXT NOT NULL,
    option_d TEXT NOT NULL,
    correct_answer INTEGER NOT NULL CHECK (correct_answer >= 0 AND correct_answer <= 3),
    level INTEGER NOT NULL CHECK (level >= 0 AND level <= 2),  -- 0=easy, 1=medium, 2=hard
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP,
    updated_by INTEGER REFERENCES users(id) ON DELETE SET NULL
);

-- ============================================
-- 3. GAME_SESSIONS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS game_sessions (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'active' CHECK (status IN ('active', 'won', 'lost', 'quit')),
    current_question_number INTEGER DEFAULT 1,
    current_level INTEGER DEFAULT 1,
    current_prize BIGINT DEFAULT 1000000,
    total_score INTEGER DEFAULT 0,
    final_prize BIGINT,
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP
);

-- ============================================
-- 4. GAME_QUESTIONS TABLE (Junction Table)
-- ============================================
CREATE TABLE IF NOT EXISTS game_questions (
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_order INTEGER NOT NULL CHECK (question_order >= 1 AND question_order <= 15),
    question_id INTEGER REFERENCES questions(id) ON DELETE CASCADE,
    PRIMARY KEY (game_id, question_order)
);

-- ============================================
-- 5. GAME_ANSWERS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS game_answers (
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_order INTEGER NOT NULL CHECK (question_order >= 1 AND question_order <= 15),
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    selected_option INTEGER CHECK (selected_option >= 0 AND selected_option <= 3),
    is_correct BOOLEAN NOT NULL,
    response_time_second INTEGER,
    PRIMARY KEY (game_id, question_order),
    FOREIGN KEY (game_id, question_order) REFERENCES game_questions(game_id, question_order) ON DELETE CASCADE
);

-- ============================================
-- 6. SAVED_GAMES TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS saved_games (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_number INTEGER NOT NULL,
    prize BIGINT NOT NULL,
    score INTEGER NOT NULL,
    used_lifelines TEXT, -- JSON array: ["5050", "PHONE", "AUDIENCE"]
    saved_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============================================
-- 7. FRIEND_REQUESTS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS friend_requests (
    id SERIAL PRIMARY KEY,
    from_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    to_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'declined')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(from_user_id, to_user_id),
    CHECK (from_user_id != to_user_id)
);

-- ============================================
-- 8. FRIENDSHIPS TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS friendships (
    id SERIAL PRIMARY KEY,
    user1_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    user2_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user1_id, user2_id),
    CHECK (user1_id < user2_id) -- Ensures single record per friendship pair
);

-- ============================================
-- 9. MESSAGES TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS messages (
    id SERIAL PRIMARY KEY,
    from_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    to_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE SET NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CHECK (from_user_id != to_user_id)
);

-- ============================================
-- 10. LEADERBOARD TABLE
-- ============================================
CREATE TABLE IF NOT EXISTS leaderboard (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE UNIQUE,
    final_question_number INTEGER CHECK (final_question_number >= 1 AND final_question_number <= 15),
    total_score BIGINT NOT NULL DEFAULT 0,
    highest_prize BIGINT NOT NULL DEFAULT 0,
    games_played INTEGER DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============================================
-- INDEXES FOR PERFORMANCE
-- ============================================

-- User lookups
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);

-- Question queries
CREATE INDEX IF NOT EXISTS idx_questions_level ON questions(level);
CREATE INDEX IF NOT EXISTS idx_questions_is_active ON questions(is_active);

-- Game session queries
CREATE INDEX IF NOT EXISTS idx_game_sessions_user_id ON game_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_game_sessions_status ON game_sessions(status);

-- Game questions/answers
CREATE INDEX IF NOT EXISTS idx_game_questions_game_id ON game_questions(game_id);
CREATE INDEX IF NOT EXISTS idx_game_answers_game_id ON game_answers(game_id);

-- Saved games
CREATE INDEX IF NOT EXISTS idx_saved_games_user_id ON saved_games(user_id);

-- Friends
CREATE INDEX IF NOT EXISTS idx_friendships_user1 ON friendships(user1_id);
CREATE INDEX IF NOT EXISTS idx_friendships_user2 ON friendships(user2_id);
CREATE INDEX IF NOT EXISTS idx_friend_requests_to_user ON friend_requests(to_user_id);

-- Messages
CREATE INDEX IF NOT EXISTS idx_messages_to_user ON messages(to_user_id);
CREATE INDEX IF NOT EXISTS idx_messages_from_user ON messages(from_user_id);

-- Leaderboard
CREATE INDEX IF NOT EXISTS idx_leaderboard_total_score ON leaderboard(total_score DESC);
CREATE INDEX IF NOT EXISTS idx_leaderboard_final_question ON leaderboard(final_question_number DESC);
