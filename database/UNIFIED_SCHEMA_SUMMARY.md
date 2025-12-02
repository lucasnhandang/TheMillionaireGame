# Unified Database Schema Summary

## Overview

This document summarizes the **unified database schema** that combines the best elements from both design approaches. The unified schema ensures all PROTOCOL.md requirements are met while maintaining data integrity and supporting all game features.

---

## Design Decisions

### Core Principle
- **Use teammate's core structure** for proven patterns (users, game_sessions, saved_games, friend_requests)
- **Add missing tables** from your design (game_questions, game_answers, messages)
- **Use correct field types** from teammate's design (INTEGER for correct_answer, level; BIGINT for prizes)
- **Include audit fields** from your design (isActive, updated_at, updated_by for questions)

---

## Unified Schema Structure

### 1. **users** Table
**Source**: Teammate's design (with ban functionality)

```sql
users {
    id (PK, SERIAL)
    username (VARCHAR(50), UNIQUE, NOT NULL)
    password_hash (VARCHAR(255), NOT NULL)
    role (VARCHAR(10), CHECK: 'user' or 'admin', DEFAULT 'user')
    is_banned (BOOLEAN, DEFAULT FALSE)
    ban_reason (TEXT)
    created_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    last_login (TIMESTAMP)
}
```

**Key Features**:
- ✅ Ban tracking for BAN_USER admin functionality
- ✅ Role-based access control (user/admin)
- ✅ Login tracking

---

### 2. **questions** Table
**Source**: Hybrid (teammate's structure + your audit fields)

```sql
questions {
    id (PK, SERIAL)
    question_text (TEXT, NOT NULL)
    option_a (TEXT, NOT NULL)
    option_b (TEXT, NOT NULL)
    option_c (TEXT, NOT NULL)
    option_d (TEXT, NOT NULL)
    correct_answer (INTEGER, CHECK: 0-3, NOT NULL)  -- Matches PROTOCOL.md answerIndex
    level (INTEGER, CHECK: 1-15, NOT NULL)           -- Matches game structure
    is_active (BOOLEAN, DEFAULT TRUE)                -- Soft delete
    created_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    updated_at (TIMESTAMP)                           -- Audit trail
    updated_by (INTEGER, FK → users.id)              -- Audit trail
}
```

**Key Features**:
- ✅ INTEGER `correct_answer` (0-3) matches PROTOCOL.md `answerIndex`
- ✅ INTEGER `level` (1-15) matches game question levels
- ✅ Soft delete with `is_active` flag
- ✅ Audit trail with `updated_at` and `updated_by`

---

### 3. **game_sessions** Table
**Source**: Teammate's design (includes score/prize tracking)

```sql
game_sessions {
    id (PK, SERIAL)
    user_id (FK → users.id, ON DELETE CASCADE)
    status (VARCHAR(20), CHECK: 'active', 'won', 'lost', 'quit', DEFAULT 'active')
    current_question_number (INTEGER, DEFAULT 1)
    current_level (INTEGER, DEFAULT 1)
    current_prize (BIGINT, DEFAULT 1000000)          -- Large prize values
    total_score (INTEGER, DEFAULT 0)                 -- Cumulative score
    final_prize (BIGINT)                             -- Final prize when game ends
    started_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    ended_at (TIMESTAMP)
}
```

**Key Features**:
- ✅ `total_score` for leaderboard ranking and VIEW_HISTORY
- ✅ `final_prize` for game results display
- ✅ BIGINT for prizes (supports 1 billion max prize)
- ✅ Status tracking (active/won/lost/quit)

---

### 4. **game_questions** Table
**Source**: Your design (essential junction table)

```sql
game_questions {
    game_id (PK, FK → game_sessions.id, ON DELETE CASCADE)
    question_order (PK, INTEGER)                    -- Question number (1-15)
    question_id (FK → questions.id, ON DELETE CASCADE)
}
```

**Key Features**:
- ✅ Tracks which questions were assigned to which game
- ✅ Required for RESUME functionality (knowing which questions were already asked)
- ✅ Supports game history queries
- ✅ Composite primary key (game_id, question_order)

---

### 5. **game_answers** Table
**Source**: Your design (essential for history)

```sql
game_answers {
    game_id (PK, FK → game_sessions.id, ON DELETE CASCADE)
    question_order (PK, FK → game_questions.question_order)
    submitted_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    selected_option (INTEGER, CHECK: 0-3)            -- answerIndex
    is_correct (BOOLEAN, NOT NULL)
    response_time_second (INTEGER)                  -- Time taken to answer
}
```

**Key Features**:
- ✅ Tracks all answers submitted during games
- ✅ Required for VIEW_HISTORY request
- ✅ Supports game replay and analytics
- ✅ `response_time_second` useful for scoring/timeout handling

---

### 6. **saved_games** Table
**Source**: Teammate's design (required for RESUME)

```sql
saved_games {
    id (PK, SERIAL)
    user_id (FK → users.id, ON DELETE CASCADE)
    game_id (FK → game_sessions.id, ON DELETE CASCADE)
    question_number (INTEGER, NOT NULL)
    prize (BIGINT, NOT NULL)
    score (INTEGER, NOT NULL)
    used_lifelines (TEXT)                           -- JSON array: ["5050", "PHONE", "AUDIENCE"]
    saved_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
}
```

**Key Features**:
- ✅ Required for RESUME functionality (PROTOCOL.md)
- ✅ Stores game state when connection is lost
- ✅ JSON array for used lifelines (flexible storage)

---

### 7. **friend_requests** Table
**Source**: Teammate's design (cleaner separation)

```sql
friend_requests {
    id (PK, SERIAL)
    from_user_id (FK → users.id, ON DELETE CASCADE)
    to_user_id (FK → users.id, ON DELETE CASCADE)
    status (VARCHAR(20), CHECK: 'pending', 'accepted', 'declined', DEFAULT 'pending')
    created_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    UNIQUE(from_user_id, to_user_id)
    CHECK (from_user_id != to_user_id)
}
```

**Key Features**:
- ✅ Separate table for pending requests
- ✅ Supports ACCEPT_FRIEND, DECLINE_FRIEND operations
- ✅ Prevents self-friend requests

---

### 8. **friendships** Table
**Source**: Teammate's design (accepted friendships only)

```sql
friendships {
    id (PK, SERIAL)
    user1_id (FK → users.id, ON DELETE CASCADE)
    user2_id (FK → users.id, ON DELETE CASCADE)
    created_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    UNIQUE(user1_id, user2_id)
    CHECK (user1_id < user2_id)                     -- Ensures single record per friendship
}
```

**Key Features**:
- ✅ Stores only accepted friendships
- ✅ Single record per friendship pair (user1_id < user2_id constraint)
- ✅ No duplicate data
- ✅ Bidirectional queries required: `WHERE user1_id = X OR user2_id = X`

---

### 9. **messages** Table
**Source**: Your design (required for CHAT)

```sql
messages {
    id (PK, SERIAL)
    sender_id (FK → users.id, ON DELETE CASCADE)
    receiver_id (FK → users.id, ON DELETE CASCADE)
    game_id (FK → game_sessions.id, ON DELETE SET NULL, NULLABLE)
    sent_at (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
    content (TEXT, NOT NULL)
    is_read (BOOLEAN, DEFAULT FALSE)
}
```

**Key Features**:
- ✅ Required for CHAT functionality (PROTOCOL.md)
- ✅ Optional `game_id` for game-related messages
- ✅ Read status tracking
- ✅ Supports offline message delivery

---

### 10. **leaderboard** Table
**Source**: Hybrid (teammate's structure + PROTOCOL.md requirements)

```sql
leaderboard {
    id (PK, SERIAL)
    user_id (FK → users.id, ON DELETE CASCADE, UNIQUE)
    final_question_number (INTEGER)                  -- Highest questions answered correctly (1-15)
    total_score (BIGINT, NOT NULL)                  -- Total score from best game
    highest_prize (BIGINT, NOT NULL)                -- Best prize achieved
    games_played (INTEGER, DEFAULT 0)
    last_updated (TIMESTAMP, DEFAULT CURRENT_TIMESTAMP)
}
```

**Key Features**:
- ✅ `final_question_number` for primary ranking (PROTOCOL.md line 604)
- ✅ `total_score` for secondary ranking (PROTOCOL.md line 604)
- ✅ Cached leaderboard data for performance
- ✅ Matches PROTOCOL.md ranking rules

---

## Key Changes from Original Designs

### From Your Design:
1. ✅ **Added**: `game_questions` junction table
2. ✅ **Added**: `game_answers` table
3. ✅ **Added**: `messages` table
4. ✅ **Added**: Audit fields to `questions` (`is_active`, `updated_at`, `updated_by`)

### From Teammate's Design:
1. ✅ **Added**: `saved_games` table
2. ✅ **Added**: `is_banned`, `ban_reason` to `users`
3. ✅ **Added**: `total_score`, `final_prize` to `game_sessions`
4. ✅ **Changed**: `correct_answer` from STRING to INTEGER (0-3)
5. ✅ **Changed**: `difficulty` to `level` as INTEGER (0-2: 0 = easy, 1 = medium, 2 = hard)
6. ✅ **Changed**: `current_amount` to `current_prize` as BIGINT
7. ✅ **Separated**: Friend requests into `friend_requests` and `friendships` tables

---

## Indexes for Performance

```sql
-- User lookups
CREATE INDEX idx_users_username ON users(username);

-- Question queries
CREATE INDEX idx_questions_level ON questions(level);
CREATE INDEX idx_questions_is_active ON questions(is_active);

-- Game session queries
CREATE INDEX idx_game_sessions_user_id ON game_sessions(user_id);
CREATE INDEX idx_game_sessions_status ON game_sessions(status);

-- Game questions/answers
CREATE INDEX idx_game_questions_game_id ON game_questions(game_id);
CREATE INDEX idx_game_answers_game_id ON game_answers(game_id);

-- Saved games
CREATE INDEX idx_saved_games_user_id ON saved_games(user_id);

-- Friends
CREATE INDEX idx_friendships_user1 ON friendships(user1_id);
CREATE INDEX idx_friendships_user2 ON friendships(user2_id);
CREATE INDEX idx_friend_requests_to_user ON friend_requests(to_user_id);

-- Leaderboard
CREATE INDEX idx_leaderboard_total_score ON leaderboard(total_score DESC);
CREATE INDEX idx_leaderboard_final_question ON leaderboard(final_question_number DESC);
```

---

## Relationships Summary

```
users (1) ──< (many) game_sessions
users (1) ──< (many) saved_games
users (1) ──< (many) messages (as sender)
users (1) ──< (many) messages (as receiver)
users (1) ──< (many) friend_requests (as from_user)
users (1) ──< (many) friend_requests (as to_user)
users (1) ──< (many) friendships (as user1)
users (1) ──< (many) friendships (as user2)
users (1) ──< (1) leaderboard

game_sessions (1) ──< (many) game_questions
game_sessions (1) ──< (many) game_answers
game_sessions (1) ──< (many) messages (optional)

questions (1) ──< (many) game_questions

game_questions (1) ──< (1) game_answers
```

---

## PROTOCOL.md Compliance

### ✅ All Required Features Supported:

1. **Authentication**: `users` table with role, ban tracking
2. **Game Sessions**: `game_sessions` with status, score, prize tracking
3. **Question Management**: `questions` with level, correct_answer (INTEGER)
4. **Game History**: `game_answers` table for VIEW_HISTORY
5. **Resume Functionality**: `saved_games` table
6. **Friend System**: `friend_requests` + `friendships` tables
7. **Chat**: `messages` table
8. **Leaderboard**: `leaderboard` table with proper ranking fields
9. **Admin Operations**: `users.is_banned`, `questions.is_active`

---

## Next Steps

1. ✅ Create unified `schema.sql` file with all tables
2. ✅ Implement `database.h/cpp` module using this schema
3. ✅ Update server handlers to use database methods
4. ✅ Test all PROTOCOL.md request types with database

---

## Summary

The unified schema successfully combines:
- **Teammate's proven structure** for core game functionality
- **Your additional tables** for complete feature coverage
- **Correct field types** matching PROTOCOL.md specifications
- **Audit trails** for data integrity

This ensures all 27 request types in PROTOCOL.md are fully supported with proper database persistence.

