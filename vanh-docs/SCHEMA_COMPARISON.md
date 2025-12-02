# Database Schema Comparison

## Overview

This document compares **Your Design** (from `vanh-docs/relational-schema.md` and UML diagram) with **Teammate's Recommended Schema** (from `database/schema.sql`).

---

## Table-by-Table Comparison

### 1. USERS Table

#### Your Design:
```sql
USERS {
    user_id (PK)
    username
    password_hash
    created_at
    role
}
```

#### Teammate's Design:
```sql
users {
    id (PK)
    username (UNIQUE)
    password_hash
    role (CHECK: 'user' or 'admin')
    is_banned (BOOLEAN)
    ban_reason (TEXT)
    created_at
    last_login
}
```

**Differences:**
- ✅ **Your design**: Simpler, basic fields only
- ✅ **Teammate's design**: Adds `is_banned`, `ban_reason`, `last_login` (needed for BAN_USER functionality)
- ⚠️ **Missing in yours**: Ban tracking (required by PROTOCOL.md for BAN_USER request)
- ⚠️ **Missing in yours**: `last_login` tracking

**Recommendation**: **Use Teammate's design** - includes ban functionality required by admin handlers.

---

### 2. QUESTIONS Table

#### Your Design:
```sql
QUESTIONS {
    question_id (PK)
    question_text
    option_a, option_b, option_c, option_d
    correct_answer (STRING)
    difficulty (STRING)
    isActive (BOOLEAN)
    created_at
    updated_at
    updated_by
}
```

#### Teammate's Design:
```sql
questions {
    id (PK)
    question_text
    option_a, option_b, option_c, option_d
    correct_answer (INTEGER, CHECK: 0-3)
    level (INTEGER, CHECK: 1-15)
    created_at
}
```

**Differences:**
- ✅ **Your design**: More comprehensive with `updated_at`, `updated_by`, `isActive` (soft delete)
- ✅ **Your design**: `difficulty` as string (more flexible)
- ✅ **Teammate's design**: `correct_answer` as INTEGER (0-3) - matches PROTOCOL.md (answerIndex 0-3)
- ✅ **Teammate's design**: `level` as INTEGER (1-15) - matches game structure
- ⚠️ **Missing in teammate's**: `isActive` flag (soft delete for questions)
- ⚠️ **Missing in teammate's**: `updated_at`, `updated_by` (audit trail)

**Recommendation**: **Hybrid approach** - Use teammate's `correct_answer` (INTEGER) and `level` (INTEGER), but add your `isActive`, `updated_at`, `updated_by` fields.

---

### 3. GAMES / game_sessions Table

#### Your Design:
```sql
GAMES {
    game_id (PK)
    user_id (FK)
    started_at
    ended_at
    current_q_number
    current_amount
    status
    used_5050 (BOOLEAN)
    used_ask (BOOLEAN)
    used_phone (BOOLEAN)
}
```

#### Teammate's Design:
```sql
game_sessions {
    id (PK)
    user_id (FK)
    status (CHECK: 'active', 'won', 'lost', 'quit')
    current_question_number
    current_level
    current_prize (BIGINT)
    total_score (INTEGER)
    final_prize (BIGINT)
    started_at
    ended_at
}
```

**Differences:**
- ✅ **Your design**: Tracks lifelines in game table (`used_5050`, `used_ask`, `used_phone`)
- ✅ **Teammate's design**: Tracks `total_score` and `final_prize` (needed for leaderboard/scoring)
- ✅ **Teammate's design**: Has `current_level` (matches question level system)
- ⚠️ **Missing in teammate's**: Lifeline tracking (but teammate uses `saved_games.used_lifelines` JSON field)
- ⚠️ **Missing in yours**: `total_score` and `final_prize` (needed for game results)

**Recommendation**: **Use Teammate's design** but add lifeline tracking fields OR use `saved_games.used_lifelines` JSON approach.

---

### 4. GAME_QUESTIONS Junction Table

#### Your Design:
```sql
GAME_QUESTIONS {
    game_id (PK, FK)
    question_order (PK)
    question_id (FK)
}
```

#### Teammate's Design:
```sql
-- NOT PRESENT in teammate's schema
```

**Differences:**
- ✅ **Your design**: Has explicit junction table linking games to questions
- ❌ **Teammate's design**: **Missing this table entirely**
- ⚠️ **Problem**: Without this table, how do you track which questions were assigned to which game?

**Recommendation**: **Use Your Design** - This table is essential for:
- Tracking which questions were assigned to each game
- Supporting RESUME functionality (knowing which questions were already asked)
- Game history queries

---

### 5. GAME_ANSWERS Table

#### Your Design:
```sql
GAME_ANSWERS {
    game_id (PK, FK)
    question_order (PK)
    submitted_at
    selected_option (STRING)
    is_correct (BOOLEAN)
    response_time_second (INTEGER)
}
```

#### Teammate's Design:
```sql
-- NOT PRESENT in teammate's schema
```

**Differences:**
- ✅ **Your design**: Tracks all answers submitted during games
- ✅ **Your design**: Includes `response_time_second` (useful for scoring/timeout handling)
- ❌ **Teammate's design**: **Missing this table entirely**
- ⚠️ **Problem**: Without this table, you can't:
  - Track answer history for VIEW_HISTORY request
  - Replay game sessions
  - Analyze player performance

**Recommendation**: **Use Your Design** - This table is essential for game history and analytics.

---

### 6. saved_games Table

#### Your Design:
```sql
-- NOT PRESENT in your design
```

#### Teammate's Design:
```sql
saved_games {
    id (PK)
    user_id (FK)
    game_id (FK)
    question_number
    prize
    score
    used_lifelines (TEXT - JSON array)
    saved_at
}
```

**Differences:**
- ✅ **Teammate's design**: Dedicated table for auto-saved games (RESUME functionality)
- ❌ **Your design**: **Missing this table**
- ⚠️ **Problem**: Without this table, RESUME request cannot work (per PROTOCOL.md)

**Recommendation**: **Use Teammate's Design** - Required for RESUME functionality.

---

### 7. FRIENDSHIPS Table

#### Your Design:
```sql
FRIENDSHIPS {
    user_id (PK, FK)
    friend_user_id (PK, FK)
    status (STRING: 'pending', 'accepted', 'blocked')
    created_at
    accepted_at
}
```

#### Teammate's Design:
```sql
friendships {
    id (PK)
    user1_id (FK)
    user2_id (FK)
    created_at
    UNIQUE(user1_id, user2_id)
    CHECK (user1_id < user2_id)
}
```

**Differences:**
- ✅ **Your design**: Has `status` field (pending/accepted/blocked) - more flexible
- ✅ **Your design**: Has `accepted_at` timestamp
- ✅ **Teammate's design**: Uses `user1_id < user2_id` constraint to prevent duplicates
- ⚠️ **Teammate's design**: No status field - assumes all friendships are accepted
- ⚠️ **Teammate's design**: Uses separate `friend_requests` table for pending requests

**Recommendation**: **Hybrid approach** - Use teammate's `friend_requests` table for pending requests, and `friendships` table for accepted friendships (simpler than single table with status).

---

### 8. friend_requests Table

#### Your Design:
```sql
-- NOT PRESENT (uses FRIENDSHIPS.status instead)
```

#### Teammate's Design:
```sql
friend_requests {
    id (PK)
    from_user_id (FK)
    to_user_id (FK)
    status (CHECK: 'pending', 'accepted', 'declined')
    created_at
    UNIQUE(from_user_id, to_user_id)
    CHECK (from_user_id != to_user_id)
}
```

**Differences:**
- ✅ **Teammate's design**: Separate table for friend requests (cleaner separation)
- ✅ **Teammate's design**: Supports 'declined' status
- ❌ **Your design**: Uses single FRIENDSHIPS table with status field

**Recommendation**: **Use Teammate's Design** - Cleaner separation of concerns.

---

### 9. LEADERBOARD Table

#### Your Design:
```sql
LEADERBOARD {
    user_id (PK, FK)
    total_games_played
    total_correct_answers
    total_amount_earned
    best_amount
    last_played_at
}
```

#### Teammate's Design:
```sql
leaderboard {
    id (PK)
    user_id (FK, UNIQUE)
    total_score (BIGINT)
    highest_prize (BIGINT)
    games_played (INTEGER)
    last_updated
}
```

**Differences:**
- ✅ **Your design**: More detailed stats (`total_correct_answers`, `total_amount_earned`)
- ✅ **Your design**: `best_amount` field (matches PROTOCOL.md leaderboard ranking)
- ✅ **Teammate's design**: Simpler, focuses on `total_score` and `highest_prize`
- ⚠️ **Note**: PROTOCOL.md says leaderboard ranks by `finalQuestionNumber` (questions answered correctly) and `totalScore` - neither design perfectly matches this

**Recommendation**: **Hybrid approach** - Use teammate's structure but add fields needed for PROTOCOL.md ranking (finalQuestionNumber, totalScore from best game).

---

### 10. MESSAGES Table

#### Your Design:
```sql
MESSAGES {
    message_id (PK)
    sender_id (FK)
    receiver_id (FK)
    game_id (FK, nullable)
    sent_at
    content
    is_read (BOOLEAN)
}
```

#### Teammate's Design:
```sql
-- NOT PRESENT in teammate's schema
```

**Differences:**
- ✅ **Your design**: Complete chat/messaging system
- ❌ **Teammate's design**: **Missing this table entirely**
- ⚠️ **Problem**: PROTOCOL.md includes CHAT request - needs message storage

**Recommendation**: **Use Your Design** - Required for CHAT functionality.

---

## Summary of Key Differences

### ✅ **Your Design Advantages:**
1. **GAME_QUESTIONS junction table** - Essential for tracking game-question assignments
2. **GAME_ANSWERS table** - Essential for game history and analytics
3. **MESSAGES table** - Required for CHAT functionality
4. **More detailed leaderboard stats** - Better analytics
5. **Questions table** - Better audit trail (`updated_at`, `updated_by`, `isActive`)

### ✅ **Teammate's Design Advantages:**
1. **users.is_banned, ban_reason** - Required for BAN_USER functionality
2. **saved_games table** - Required for RESUME functionality
3. **friend_requests table** - Cleaner separation of pending vs accepted friendships
4. **questions.correct_answer as INTEGER** - Matches PROTOCOL.md (answerIndex 0-3)
5. **questions.level as INTEGER** - Matches game structure (1-15)
6. **game_sessions.total_score, final_prize** - Required for game results

### ❌ **Critical Missing Tables:**

**In Your Design:**
- `saved_games` - **CRITICAL** for RESUME functionality

**In Teammate's Design:**
- `GAME_QUESTIONS` - **CRITICAL** for tracking which questions belong to which game
- `GAME_ANSWERS` - **CRITICAL** for VIEW_HISTORY request
- `MESSAGES` - **CRITICAL** for CHAT request

---

## Recommended Final Schema

### Core Tables (Use Teammate's Design):
1. ✅ `users` - Use teammate's (includes ban fields)
2. ✅ `questions` - Use teammate's structure but add `isActive`, `updated_at`, `updated_by`
3. ✅ `game_sessions` - Use teammate's (includes score/prize fields)
4. ✅ `saved_games` - Use teammate's (required for RESUME)
5. ✅ `friend_requests` - Use teammate's (cleaner separation)
6. ✅ `friendships` - Use teammate's (for accepted friendships)

### Additional Tables (From Your Design):
7. ✅ `game_questions` - **ADD THIS** (junction table for game-question assignments)
8. ✅ `game_answers` - **ADD THIS** (for VIEW_HISTORY and analytics)
9. ✅ `messages` - **ADD THIS** (for CHAT functionality)

### Leaderboard (Hybrid):
10. ✅ `leaderboard` - Use teammate's structure but add fields for PROTOCOL.md ranking:
    - `final_question_number` (INTEGER) - Highest questions answered correctly
    - `total_score` (BIGINT) - Total score from best game
    - Keep existing: `highest_prize`, `games_played`, `last_updated`

---

## Field Type Corrections Needed

### questions.correct_answer
- **Your design**: STRING
- **Teammate's design**: INTEGER (0-3) ✅
- **Recommendation**: Use INTEGER - matches PROTOCOL.md `answerIndex` (0-3)

### questions.difficulty vs level
- **Your design**: `difficulty` (STRING)
- **Teammate's design**: `level` (INTEGER, 1-15) ✅
- **Recommendation**: Use `level` (INTEGER) - matches game structure

### game_sessions.current_amount vs current_prize
- **Your design**: `current_amount` (INTEGER)
- **Teammate's design**: `current_prize` (BIGINT) ✅
- **Recommendation**: Use `current_prize` (BIGINT) - prizes can be very large (1 billion)

---

## Action Items

1. ✅ **Add `saved_games` table** from teammate's design
2. ✅ **Add `game_questions` table** from your design
3. ✅ **Add `game_answers` table** from your design
4. ✅ **Add `messages` table** from your design
5. ✅ **Update `users` table** to include `is_banned`, `ban_reason`, `last_login`
6. ✅ **Update `questions` table** to use INTEGER for `correct_answer` and `level`
7. ✅ **Update `questions` table** to add `isActive`, `updated_at`, `updated_by`
8. ✅ **Update `game_sessions` table** to include `total_score`, `final_prize`
9. ✅ **Update `leaderboard` table** to include `final_question_number` and `total_score`

---

## Conclusion

**Best Approach**: **Hybrid schema** combining:
- Teammate's core structure (users, game_sessions, saved_games, friend_requests)
- Your additional tables (game_questions, game_answers, messages)
- Field type corrections from teammate's design (INTEGER for correct_answer, level)
- Additional fields from your design (isActive, updated_at, updated_by for questions)

This ensures all PROTOCOL.md requirements are met while maintaining data integrity and supporting all features.

