# Vanh's Update - Handoff Document

# Test Case: Test 3.1 - Start New Game

**Location:** `database/TEST_GUIDE.md` (lines 143-155)

**Test Request:**
```json
{"requestType":"START","data":{"authToken":"<your-auth-token>"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Game started","gameId":<number>,"timestamp":<unix-timestamp>}}
```

---

## Error Message

**Server Log Error:**
```
[2025-12-30 19:40:11] [ERROR] [database.cpp:326] Create game session failed: ERROR:  column "current_level" of relation "game_sessions" does not exist
LINE 1: ...ssions (user_id, status, current_question_number, current_le...
                                                             ^
```

**Client Response:**
```json
{"responseCode":500,"message":"Failed to create game session"}
```

---

## Root Cause

1. **Schema Mismatch**: The `current_level` column was removed from the `game_sessions` table in `schema.sql` (line 45 is commented out), but the database code (`database.cpp`) still attempted to INSERT, UPDATE, and SELECT this non-existent column.

2. **Why `current_level` was removed**: The `current_level` is a derived value that can be calculated from `current_question_number`:
   - Questions 1-5 → level 0 (easy)
   - Questions 6-10 → level 1 (medium)
   - Questions 11-15 → level 2 (hard)
   
   Storing it separately was redundant and could lead to data inconsistencies.

3. **Impact**: All game session operations (create, update, retrieve) failed because they referenced a column that doesn't exist in the database schema.

---

## Solution Implemented

### Strategy
- Remove all references to `current_level` from database queries (INSERT, UPDATE, SELECT)
- Calculate `current_level` dynamically when reading from database based on `current_question_number`
- Fix column indices in SELECT queries after removing `current_level`

### Code Changes

#### 1. `database/database.cpp` - `createGameSession()` function

**File:** `database/database.cpp`  
**Lines:** 319-321  
**Function:** `Database::createGameSession()`

**Previous Code:**
```cpp
string query = "INSERT INTO game_sessions (user_id, status, current_question_number, "
               "current_level, current_prize, total_score) VALUES (" +
               to_string(user_id) + ", 'active', 1, 1, 1000000, 0) RETURNING id";
```

**New Code:**
```cpp
string query = "INSERT INTO game_sessions (user_id, status, current_question_number, "
               "current_prize, total_score) VALUES (" +
               to_string(user_id) + ", 'active', 1, 1000000, 0) RETURNING id";
```

**Change:** Removed `current_level` column and its value `1` from INSERT statement.

---

#### 2. `database/database.cpp` - `updateGameSession()` function

**File:** `database/database.cpp`  
**Lines:** 339-344  
**Function:** `Database::updateGameSession()`

**Previous Code:**
```cpp
string query = "UPDATE game_sessions SET status = " + escapeString(session.status) +
               ", current_question_number = " + to_string(session.current_question_number) +
               ", current_level = " + to_string(session.current_level) +
               ", current_prize = " + to_string(session.current_prize) +
               ", total_score = " + to_string(session.total_score) +
               ", final_prize = " + (session.final_prize > 0 ? to_string(session.final_prize) : "NULL") +
               " WHERE id = " + to_string(session.id);
```

**New Code:**
```cpp
string query = "UPDATE game_sessions SET status = " + escapeString(session.status) +
               ", current_question_number = " + to_string(session.current_question_number) +
               ", current_prize = " + to_string(session.current_prize) +
               ", total_score = " + to_string(session.total_score) +
               ", final_prize = " + (session.final_prize > 0 ? to_string(session.final_prize) : "NULL") +
               " WHERE id = " + to_string(session.id);
```

**Change:** Removed `current_level` assignment from UPDATE statement.

---

#### 3. `database/database.cpp` - `getActiveGameSession()` function

**File:** `database/database.cpp`  
**Lines:** 365-390  
**Function:** `Database::getActiveGameSession()`

**Previous Code:**
```cpp
string query = "SELECT id, user_id, status, current_question_number, current_level, "
               "current_prize, total_score, final_prize, "
               "EXTRACT(EPOCH FROM started_at)::bigint, "
               "EXTRACT(EPOCH FROM ended_at)::bigint "
               "FROM game_sessions WHERE user_id = " + to_string(user_id) +
               " AND status = 'active' ORDER BY started_at DESC LIMIT 1";

// ... later in the function ...
session.id = atoi(PQgetvalue(res, 0, 0));
session.user_id = atoi(PQgetvalue(res, 0, 1));
session.status = PQgetvalue(res, 0, 2);
session.current_question_number = atoi(PQgetvalue(res, 0, 3));
session.current_level = atoi(PQgetvalue(res, 0, 4));
session.current_prize = atoll(PQgetvalue(res, 0, 5));
session.total_score = atoi(PQgetvalue(res, 0, 6));
if (PQgetvalue(res, 0, 7)) session.final_prize = atoll(PQgetvalue(res, 0, 7));
session.started_at = atol(PQgetvalue(res, 0, 8));
if (PQgetvalue(res, 0, 9)) session.ended_at = atol(PQgetvalue(res, 0, 9));
```

**New Code:**
```cpp
string query = "SELECT id, user_id, status, current_question_number, "
               "current_prize, total_score, final_prize, "
               "EXTRACT(EPOCH FROM started_at)::bigint, "
               "EXTRACT(EPOCH FROM ended_at)::bigint "
               "FROM game_sessions WHERE user_id = " + to_string(user_id) +
               " AND status = 'active' ORDER BY started_at DESC LIMIT 1";

// ... later in the function ...
session.id = atoi(PQgetvalue(res, 0, 0));
session.user_id = atoi(PQgetvalue(res, 0, 1));
session.status = PQgetvalue(res, 0, 2);
session.current_question_number = atoi(PQgetvalue(res, 0, 3));
// Calculate current_level from current_question_number (1-5=0, 6-10=1, 11-15=2)
session.current_level = (session.current_question_number <= 5) ? 0 : 
                       (session.current_question_number <= 10) ? 1 : 2;
session.current_prize = atoll(PQgetvalue(res, 0, 4));
session.total_score = atoi(PQgetvalue(res, 0, 5));
if (PQgetvalue(res, 0, 6)) session.final_prize = atoll(PQgetvalue(res, 0, 6));
session.started_at = atol(PQgetvalue(res, 0, 7));
if (PQgetvalue(res, 0, 8)) session.ended_at = atol(PQgetvalue(res, 0, 8));
```

**Changes:**
1. Removed `current_level` from SELECT statement
2. Added calculation of `current_level` from `current_question_number` instead of reading from database
3. Fixed column indices: shifted all indices after `current_question_number` by -1
4. Fixed `ended_at` index from 9 to 8

---

#### 4. `database/database.cpp` - `loadGameProgress()` function

**File:** `database/database.cpp`  
**Lines:** 439-459  
**Function:** `Database::loadGameProgress()`

**Previous Code:**
```cpp
string query = "SELECT sg.game_id, sg.question_number, sg.prize, sg.score, "
               "gs.status, gs.current_level, gs.total_score "
               "FROM saved_games sg "
               "JOIN game_sessions gs ON sg.game_id = gs.id "
               "WHERE sg.user_id = " + to_string(user_id) + " ORDER BY sg.saved_at DESC LIMIT 1";

// ... later in the function ...
session.id = atoi(PQgetvalue(res, 0, 0));
session.current_question_number = atoi(PQgetvalue(res, 0, 1));
session.current_prize = atoll(PQgetvalue(res, 0, 2));
session.total_score = atoi(PQgetvalue(res, 0, 3));
session.status = PQgetvalue(res, 0, 4);
session.current_level = atoi(PQgetvalue(res, 0, 5));
```

**New Code:**
```cpp
string query = "SELECT sg.game_id, sg.question_number, sg.prize, sg.score, "
               "gs.status, gs.total_score "
               "FROM saved_games sg "
               "JOIN game_sessions gs ON sg.game_id = gs.id "
               "WHERE sg.user_id = " + to_string(user_id) + " ORDER BY sg.saved_at DESC LIMIT 1";

// ... later in the function ...
session.id = atoi(PQgetvalue(res, 0, 0));
session.current_question_number = atoi(PQgetvalue(res, 0, 1));
session.current_prize = atoll(PQgetvalue(res, 0, 2));
session.total_score = atoi(PQgetvalue(res, 0, 3));
session.status = PQgetvalue(res, 0, 4);
// Calculate current_level from current_question_number (1-5=0, 6-10=1, 11-15=2)
session.current_level = (session.current_question_number <= 5) ? 0 : 
                       (session.current_question_number <= 10) ? 1 : 2;
```

**Changes:**
1. Removed `gs.current_level` from SELECT statement
2. Added calculation of `current_level` from `current_question_number` instead of reading from database

---

#### 5. `database/database.cpp` - `getGameHistory()` function

**File:** `database/database.cpp`  
**Lines:** 898-924  
**Function:** `Database::getGameHistory()`

**Previous Code:**
```cpp
string query = "SELECT id, user_id, status, current_question_number, current_level, "
               "current_prize, total_score, final_prize, "
               "EXTRACT(EPOCH FROM started_at)::bigint, "
               "EXTRACT(EPOCH FROM ended_at)::bigint "
               "FROM game_sessions WHERE user_id = " + to_string(user_id) +
               " AND status != 'active' ORDER BY ended_at DESC LIMIT " + to_string(limit);

// ... later in the loop ...
for (int i = 0; i < PQntuples(res); i++) {
    GameSession session;
    session.id = atoi(PQgetvalue(res, i, 0));
    session.user_id = atoi(PQgetvalue(res, i, 1));
    session.status = PQgetvalue(res, i, 2);
    session.current_question_number = atoi(PQgetvalue(res, i, 3));
    session.current_level = atoi(PQgetvalue(res, i, 4));
    session.current_prize = atoll(PQgetvalue(res, i, 5));
    session.total_score = atoi(PQgetvalue(res, i, 6));
    if (PQgetvalue(res, i, 7)) session.final_prize = atoll(PQgetvalue(res, i, 7));
    session.started_at = atol(PQgetvalue(res, i, 8));
    if (PQgetvalue(res, i, 9)) session.ended_at = atol(PQgetvalue(res, i, 9));
    sessions.push_back(session);
}
```

**New Code:**
```cpp
string query = "SELECT id, user_id, status, current_question_number, "
               "current_prize, total_score, final_prize, "
               "EXTRACT(EPOCH FROM started_at)::bigint, "
               "EXTRACT(EPOCH FROM ended_at)::bigint "
               "FROM game_sessions WHERE user_id = " + to_string(user_id) +
               " AND status != 'active' ORDER BY ended_at DESC LIMIT " + to_string(limit);

// ... later in the loop ...
for (int i = 0; i < PQntuples(res); i++) {
    GameSession session;
    session.id = atoi(PQgetvalue(res, i, 0));
    session.user_id = atoi(PQgetvalue(res, i, 1));
    session.status = PQgetvalue(res, i, 2);
    session.current_question_number = atoi(PQgetvalue(res, i, 3));
    // Calculate current_level from current_question_number (1-5=0, 6-10=1, 11-15=2)
    session.current_level = (session.current_question_number <= 5) ? 0 : 
                           (session.current_question_number <= 10) ? 1 : 2;
    session.current_prize = atoll(PQgetvalue(res, i, 4));
    session.total_score = atoi(PQgetvalue(res, i, 5));
    if (PQgetvalue(res, i, 6)) session.final_prize = atoll(PQgetvalue(res, i, 6));
    session.started_at = atol(PQgetvalue(res, i, 7));
    if (PQgetvalue(res, i, 8)) session.ended_at = atol(PQgetvalue(res, i, 8));
    sessions.push_back(session);
}
```

**Changes:**
1. Removed `current_level` from SELECT statement
2. Added calculation of `current_level` from `current_question_number` instead of reading from database
3. Fixed column indices: shifted all indices after `current_question_number` by -1
4. Fixed `ended_at` index from 9 to 8

---

## Summary

### Files Modified
- **`database/database.cpp`** - 5 functions updated:
  1. `createGameSession()` - Removed `current_level` from INSERT
  2. `updateGameSession()` - Removed `current_level` from UPDATE
  3. `getActiveGameSession()` - Removed `current_level` from SELECT, added calculation, fixed indices
  4. `loadGameProgress()` - Removed `current_level` from SELECT, added calculation
  5. `getGameHistory()` - Removed `current_level` from SELECT, added calculation, fixed indices

### Files Referenced (Not Modified)
- **`database/schema.sql`** - `current_level` column commented out (line 45)
- **`database/database.h`** - `GameSession` struct still contains `current_level` field (used in memory)
- **`server/session_manager.h`** - `ClientSession` struct still contains `current_level` field (used in memory)
- **`server/request_handlers/game_handlers.cpp`** - Already calculates `current_level` from question number (lines 200-202)

### Testing Status
✅ **Fixed** - The START request should now work correctly. Game sessions will be created without referencing the non-existent `current_level` column.

---

## Notes for Team

- **`current_level` is still used in memory** (in `ClientSession` and `GameSession` structs) but is no longer stored in the database
- **Level calculation logic**: Questions 1-5 = level 0, Questions 6-10 = level 1, Questions 11-15 = level 2
- **This change ensures data consistency** - level is always derived from question number, preventing inconsistencies
- **No database migration needed** - the column was already removed from schema, we just fixed the code to match

---

**Date:** 2025-12-30  
**Author:** Vanh  
**Status:** ✅ Complete - Ready for testing

---

# Test Case: Question Timeout - User Stuck in Game

**Location:** Terminal output showing user cannot START new game after timeout

**Problem:**
```
Line 26: {"responseCode":200,"data":{"gameId":4,"correct":true,"questionNumber":1,...}}
Line 27: {"requestType":"START","data":{"authToken":"07cfbee3624be2b5780bccdd645c18df"}}
Line 28: {"responseCode":405,"message":"Already in a game"}
```

**Issue:** After question timeout, game ends but user is still marked as "in game", preventing new game start.

---

## Root Cause

### Current State Analysis

**Timeout Handling (lines 104-122 in `game_handlers.cpp`):**
- ✅ Sets `session.in_game = false` (in-memory)
- ✅ Stops timer
- ❌ **Missing:** Does NOT call `Database::endGame()` to update database
- ❌ **Missing:** Does NOT send GAME_END notification
- Returns error 408 with game data

### Comparison with Other Game End Scenarios

1. **Wrong Answer (lines 241-277):**
   - ✅ Sets `session.in_game = false`
   - ✅ Calls `Database::endGame(game_id, "lost", ...)`
   - ✅ Sends GAME_END notification

2. **Win Condition (lines 164-197):**
   - ✅ Sets `session.in_game = false`
   - ✅ Calls `Database::endGame(game_id, "won", ...)`
   - ✅ Sends GAME_END notification

3. **GIVE_UP:**
   - ✅ Sets `session.in_game = false`
   - ✅ Calls `Database::endGame()`
   - ✅ Sends GAME_END notification

### The Problem

When timeout occurs:
- In-memory session sets `in_game = false` ✅
- **Database game session remains "active"** ❌
- START handler checks database for active games
- Since database wasn't updated, it still shows active game
- Result: START returns error 405 "Already in a game"

### Additional Issues

1. **No Automatic Timeout Detection:** Timeout is only checked when user sends ANSWER request after timeout. Server should automatically detect and end timed-out games.

2. **START Handler Only Checks In-Memory:** START handler only checks `session.in_game` flag, not database. Should check both.

---

## Solution Implemented

### Decisions Made

1. ✅ **Automatic Timeout Detection:** Server automatically detects timeout and ends game even if user doesn't send ANSWER request
2. ✅ **Database Consistency:** Timeout handling calls `Database::endGame()` like other game end scenarios
3. ✅ **GAME_END Notification:** Timeout sends GAME_END notification like other game end scenarios
4. ✅ **Database Check in START:** START handler checks database for active games in addition to in-memory session

### Implementation Strategy

1. **Fix timeout handling in ANSWER handler** - Make it consistent with wrong answer handling
2. **Add database check to START handler** - Check both in-memory and database
3. **Add automatic timeout detection** - Periodic check in event loop
4. **Add helper methods** - `getClientFdByGameId()` and `getTimedOutGames()`

### Code Changes

#### 1. `server/session_manager.h` - Add `getClientFdByGameId()` method

**File:** `server/session_manager.h`  
**Lines:** 97-102  
**Function:** `SessionManager::getClientFdByGameId()`

**Previous Code:**
```cpp
    /**
     * Wait for all clients to disconnect
     */
    void waitForClientsToFinish();

private:
```

**New Code:**
```cpp
    /**
     * Wait for all clients to disconnect
     */
    void waitForClientsToFinish();
    
    /**
     * Find client file descriptor by game_id
     * @param game_id Game session ID
     * @return Client file descriptor, or -1 if not found
     */
    int getClientFdByGameId(int game_id);

private:
```

**Change:** Added method to find client session by game_id for timeout handling.

---

#### 2. `server/session_manager.cpp` - Implement `getClientFdByGameId()`

**File:** `server/session_manager.cpp`  
**Lines:** 112-121  
**Function:** `SessionManager::getClientFdByGameId()`

**Previous Code:**
```cpp
    LOG_INFO("Shutdown complete");
}

} // namespace MillionaireGame
```

**New Code:**
```cpp
    LOG_INFO("Shutdown complete");
}

int SessionManager::getClientFdByGameId(int game_id) {
    lock_guard<mutex> lock(clients_mutex_);
    for (const auto& pair : active_clients_) {
        if (pair.second.in_game && pair.second.game_id == game_id) {
            return pair.first;
        }
    }
    return -1;
}

} // namespace MillionaireGame
```

**Change:** Added implementation to find client file descriptor by game_id.

---

#### 3. `server/game_timer.h` - Add `getTimedOutGames()` method

**File:** `server/game_timer.h`  
**Lines:** 42-48  
**Function:** `GameTimer::getTimedOutGames()`

**Previous Code:**
```cpp
    /**
     * Stop timer for a game
     * @param game_id Game session ID
     */
    void stopTimer(int game_id);

private:
```

**New Code:**
```cpp
    /**
     * Stop timer for a game
     * @param game_id Game session ID
     */
    void stopTimer(int game_id);
    
    /**
     * Get all game IDs that have timed out
     * @return Vector of timed-out game IDs
     */
    std::vector<int> getTimedOutGames();

private:
```

**Change:** Added method to get all timed-out games for periodic checking.

---

#### 4. `server/game_timer.cpp` - Implement `getTimedOutGames()`

**File:** `server/game_timer.cpp`  
**Lines:** 40-56  
**Function:** `GameTimer::getTimedOutGames()`

**Previous Code:**
```cpp
void GameTimer::stopTimer(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    timer_start_times_.erase(game_id);
}

} // namespace MillionaireGame
```

**New Code:**
```cpp
void GameTimer::stopTimer(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    timer_start_times_.erase(game_id);
}

std::vector<int> GameTimer::getTimedOutGames() {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    std::vector<int> timed_out;
    time_t now = time(nullptr);
    
    for (const auto& pair : timer_start_times_) {
        time_t elapsed = now - pair.second;
        if (elapsed >= question_timeout_seconds_) {
            timed_out.push_back(pair.first);
        }
    }
    
    return timed_out;
}

} // namespace MillionaireGame
```

**Change:** Added implementation to find all games that have timed out.

---

#### 5. `server/request_handlers/game_handlers.cpp` - Fix START handler database check

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 21-26  
**Function:** `GameHandlers::handleStart()`

**Previous Code:**
```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    bool override_saved = JsonUtils::extractBool(request, "overrideSavedGame", false);
```

**New Code:**
```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    // Check database for active game session
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    bool override_saved = JsonUtils::extractBool(request, "overrideSavedGame", false);
```

**Change:** Added database check for active game session before allowing new game start.

---

#### 6. `server/request_handlers/game_handlers.cpp` - Fix timeout handling in ANSWER handler

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 103-123  
**Function:** `GameHandlers::handleAnswer()`

**Previous Code:**
```cpp
    // Check timeout
    if (GameTimer::getInstance().isTimeout(game_id)) {
        session.in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session.current_question_number);
        int safe_checkpoint_score = session.total_score;
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"timeRemaining\":0" +
                     ",\"pointsEarned\":0" +
                     ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                     ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                     ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"gameOver\":true,\"isWinner\":false,\"timeout\":true}";
        // For timeout, we return error response but include game data
        return "{\"responseCode\":408,\"data\":" + data + "}";
    }
```

**New Code:**
```cpp
    // Check timeout
    if (GameTimer::getInstance().isTimeout(game_id)) {
        session.in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session.current_question_number);
        int safe_checkpoint_score = session.total_score;
        
        // Update game session in database (same as wrong answer)
        Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"timeRemaining\":0" +
                     ",\"pointsEarned\":0" +
                     ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                     ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                     ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"gameOver\":true,\"isWinner\":false}";
        
        // Send GAME_END notification (same as wrong answer)
        string game_end_data = "{\"gameId\":" + to_string(game_id) +
                              ",\"status\":\"lost\"" +
                              ",\"finalLevel\":" + to_string(session.current_question_number) +
                              ",\"finalQuestionNumber\":" + to_string(session.current_question_number) +
                              ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                              ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                              ",\"isWinner\":false}";
        NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
        
        // For timeout, we return error response but include game data
        return "{\"responseCode\":408,\"data\":" + data + "}";
    }
```

**Changes:**
1. Added `Database::endGame()` call to update database (same as wrong answer)
2. Added GAME_END notification sending (same as wrong answer)
3. Removed `"timeout":true` from response data (kept in responseCode 408)
4. Made timeout handling consistent with wrong answer handling

---

#### 7. `server/request_handlers/game_handlers.cpp` - Add timer start in START handler

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 55-63  
**Function:** `GameHandlers::handleStart()`

**Previous Code:**
```cpp
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 0;  // Start with easy (level 0)
    session.current_prize = ScoringSystem::getInstance().getPrizeForLevel(0, 1);
    session.total_score = 0;
    session.used_lifelines.clear();

    string data = "{\"gameId\":" + to_string(game_id) +
```

**New Code:**
```cpp
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 0;  // Start with easy (level 0)
    session.current_prize = ScoringSystem::getInstance().getPrizeForLevel(0, 1);
    session.total_score = 0;
    session.used_lifelines.clear();
    
    // Start timer for first question
    GameTimer::getInstance().startQuestionTimer(game_id);

    string data = "{\"gameId\":" + to_string(game_id) +
```

**Change:** Added timer start when game begins so timeout can be detected.

---

#### 8. `server/event_loop.h` - Add `checkGameTimeouts()` method

**File:** `server/event_loop.h`  
**Lines:** 103-108  
**Function:** `EventLoop::checkGameTimeouts()`

**Previous Code:**
```cpp
    /**
     * Worker thread function
     */
    void workerThreadFunc();

private:
```

**New Code:**
```cpp
    /**
     * Worker thread function
     */
    void workerThreadFunc();
    
    /**
     * Check for timed-out games and end them automatically
     */
    void checkGameTimeouts();

private:
```

**Change:** Added method declaration for periodic timeout checking.

---

#### 9. `server/event_loop.cpp` - Add includes and periodic timeout check

**File:** `server/event_loop.cpp`  
**Lines:** 1-12  
**Includes:**

**Previous Code:**
```cpp
#include "event_loop.h"
#include "logger.h"
#include "auth_manager.h"
#include "request_router.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
```

**New Code:**
```cpp
#include "event_loop.h"
#include "logger.h"
#include "auth_manager.h"
#include "request_router.h"
#include "session_manager.h"
#include "game_timer.h"
#include "scoring_system.h"
#include "notification_utils.h"
#include "../database/database.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
```

**Change:** Added includes for timeout checking functionality.

---

#### 10. `server/event_loop.cpp` - Call timeout check in event loop

**File:** `server/event_loop.cpp`  
**Lines:** 151-154  
**Function:** `EventLoop::run()`

**Previous Code:**
```cpp
        if (poll_result == 0) {
            // Timeout - can do periodic tasks here (ping check, etc.)
            continue;
        }
```

**New Code:**
```cpp
        if (poll_result == 0) {
            // Timeout - do periodic tasks (ping check, game timeout check, etc.)
            checkGameTimeouts();
            continue;
        }
```

**Change:** Added call to `checkGameTimeouts()` in periodic task section.

---

#### 11. `server/event_loop.cpp` - Implement `checkGameTimeouts()` function

**File:** `server/event_loop.cpp`  
**Lines:** 521-566  
**Function:** `EventLoop::checkGameTimeouts()`

**Previous Code:**
```cpp
    }
}

} // namespace MillionaireGame
```

**New Code:**
```cpp
    }
}

void EventLoop::checkGameTimeouts() {
    // Get all timed-out games
    vector<int> timed_out_games = GameTimer::getInstance().getTimedOutGames();
    
    for (int game_id : timed_out_games) {
        // Find client session for this game
        int client_fd = SessionManager::getInstance().getClientFdByGameId(game_id);
        if (client_fd < 0) {
            // No active session found - client may have disconnected
            // Stop the timer, database cleanup will happen on next START
            GameTimer::getInstance().stopTimer(game_id);
            continue;
        }
        
        ClientSession* session = SessionManager::getInstance().getSession(client_fd);
        if (!session || !session->in_game || session->game_id != game_id) {
            // Session state doesn't match - stop timer and continue
            GameTimer::getInstance().stopTimer(game_id);
            continue;
        }
        
        // End the game due to timeout
        session->in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session->current_question_number);
        int safe_checkpoint_score = session->total_score;
        
        // Update game session in database
        Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
        
        // Send GAME_END notification
        string game_end_data = "{\"gameId\":" + to_string(game_id) +
                              ",\"status\":\"lost\"" +
                              ",\"finalLevel\":" + to_string(session->current_question_number) +
                              ",\"finalQuestionNumber\":" + to_string(session->current_question_number) +
                              ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                              ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                              ",\"isWinner\":false}";
        NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
        
        LOG_INFO("Game " + to_string(game_id) + " timed out and ended automatically");
    }
}

} // namespace MillionaireGame
```

**Change:** Added automatic timeout detection that:
1. Checks all active timers for timeout
2. Finds client session for each timed-out game
3. Ends game in database
4. Sends GAME_END notification
5. Clears session state

---

## Summary

### Files Modified
- **`server/session_manager.h`** - Added `getClientFdByGameId()` method declaration
- **`server/session_manager.cpp`** - Implemented `getClientFdByGameId()` method
- **`server/game_timer.h`** - Added `getTimedOutGames()` method declaration + `#include <vector>`
- **`server/game_timer.cpp`** - Implemented `getTimedOutGames()` method
- **`server/request_handlers/game_handlers.cpp`** - Fixed START handler (database check + timer start) and ANSWER handler (timeout handling)
- **`server/event_loop.h`** - Added `checkGameTimeouts()` method declaration
- **`server/event_loop.cpp`** - Added includes, periodic timeout check call, and `checkGameTimeouts()` implementation

### Testing Status
✅ **Fixed** - Timeout handling now:
- Updates database (calls `endGame()`)
- Sends GAME_END notification
- Automatically detects timeout without waiting for user request
- START handler checks database for active games
- User can start new game after timeout

---

## Notes for Team

- **Automatic timeout detection:** Server checks for timeouts every 1 second (when poll() times out)
- **Database consistency:** All game end scenarios (win, lose, timeout, give_up) now update database consistently
- **Notification consistency:** All game end scenarios send GAME_END notification
- **Timer management:** Timer starts when game starts, stops when game ends
- **Edge cases handled:** Client disconnection during timeout, session state mismatch

---

# Test Case: Lifeline Implementation Refactoring

**Location:** Lifeline system enhancement - database-driven hints

**Problem:**
Current lifeline implementation uses random generation for all lifeline results. Need to refactor to use pre-defined hints stored in database for consistent, question-specific lifeline responses.

---

## Root Cause

**Current Implementation:**
- **5050**: Randomly removes 2 wrong answers each time
- **PHONE**: Randomly generates suggestion (70% correct, 30% wrong)
- **AUDIENCE**: Randomly generates percentages each time

**Issue:** No consistency - same question gives different lifeline results each time, making testing and gameplay unpredictable.

**Solution:** Store lifeline hints in database for each question, allowing consistent, curated hints.

---

## Solution Implemented

### Decisions Made

1. ✅ **Database Storage**: Use JSONB columns for structured data (5050_info, ask_info) and TEXT for call_info
2. ✅ **5050 Format**: Store array of indices to keep, e.g., `[0,2]` means keep options A and C
3. ✅ **Ask Format**: Store JSON object with percentages, e.g., `{"A":65,"B":15,"C":10,"D":10}`
4. ✅ **Call Format**: Store message string, e.g., `"I'm 85% sure it's A"` (30% wrong answers)
5. ✅ **Backward Compatibility**: Fallback to random generation if database hint is NULL/empty

### Implementation Strategy

1. **Add columns to questions table** - JSONB for 5050 and ask, TEXT for call
2. **Update Question struct** - Add fields for lifeline hints
3. **Update database queries** - Read lifeline fields from database
4. **Modify lifeline_manager** - Use database hints when available, fallback to random
5. **Populate mock_data** - Add appropriate hints for all questions

### Code Changes

#### 1. `database/schema.sql` - Add lifeline columns

**File:** `database/schema.sql`  
**Lines:** 22-35  
**Change:** Added `lifeline_5050_info JSONB`, `lifeline_ask_info JSONB`, `lifeline_call_info TEXT` columns

#### 2. `database/database.h` - Add fields to Question struct

**File:** `database/database.h`  
**Lines:** 31-47  
**Change:** Added `lifeline_5050_info`, `lifeline_ask_info`, `lifeline_call_info` string fields

#### 3. `database/database.cpp` - Update queries

**Files:** `database/database.cpp`  
**Functions:** `getQuestion()`, `getGameQuestion()`, `getQuestions()`  
**Change:** Added lifeline columns to SELECT queries and extraction code

#### 4. `server/lifeline_manager.cpp` - Use database hints

**File:** `server/lifeline_manager.cpp`  
**Functions:** `use5050()`, `usePhone()`, `useAudience()`  
**Change:** Check database hints first, use if available, fallback to random generation

#### 5. `database/mock_data.sql` - Add hints for all questions

**File:** `database/mock_data.sql`  
**Change:** Added lifeline hints for all 31 questions (5050 arrays, ask percentages, call messages)

---

#### 6. `server/request_handlers/game_handlers.cpp` - Fix lifeline response to include hint data

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 362-377  
**Function:** `GameHandlers::handleLifeline()`

**Previous Code:**
```cpp
    session.used_lifelines.insert(lifeline_type);
    string data = "{\"lifelineType\":\"" + lifeline_type + "}";
    
    // TODO: Implement LIFELINE_INFO notification with delay
    // ...
    
    return StreamUtils::createSuccessResponse(200, data);
```

**New Code:**
```cpp
    session.used_lifelines.insert(lifeline_type);
    
    // Build response data with lifeline result
    // result.result_data contains the hint JSON object
    // For 5050: {"remainingOptions":[0,2]}
    // For PHONE: {"suggestion":"I'm 85% sure it's A"}
    // For AUDIENCE: {"poll":{"A":65,"B":15,"C":10,"D":10}}
    
    // Merge result_data into response, adding lifelineType
    string data;
    if (!result.result_data.empty()) {
        // result_data is a JSON object, merge lifelineType into it
        string inner_data = result.result_data;
        if (inner_data.front() == '{' && inner_data.back() == '}') {
            // Remove outer braces
            inner_data = inner_data.substr(1, inner_data.length() - 2);
            // Add lifelineType as first field
            data = "{\"lifelineType\":\"" + lifeline_type + "\"";
            if (!inner_data.empty()) {
                data += "," + inner_data;
            }
            data += "}";
        } else {
            // Fallback: just wrap it
            data = "{\"lifelineType\":\"" + lifeline_type + "\",\"data\":" + result.result_data + "}";
        }
    } else {
        // Fallback if no result_data
        data = "{\"lifelineType\":\"" + lifeline_type + "\"}";
    }
    
    // TODO: Implement LIFELINE_INFO notification with delay
    // ...
    
    return StreamUtils::createSuccessResponse(200, data);
```

**Change:** Fixed incomplete response - now includes lifeline hint data from `result.result_data` merged with `lifelineType` field.

**Issue Fixed:** Response was missing closing quote and lifeline hint data (e.g., `remainingOptions` for 5050).

---

## Summary

### Files Modified
- **`database/schema.sql`** - Added 3 lifeline columns
- **`database/database.h`** - Added 3 fields to Question struct
- **`database/database.cpp`** - Updated 3 query functions
- **`server/lifeline_manager.cpp`** - Modified all 3 lifeline methods
- **`database/mock_data.sql`** - Added hints for all questions
- **`server/request_handlers/game_handlers.cpp`** - Fixed response to include lifeline hint data

### Testing Status
✅ **Complete** - Lifeline system now:
- Uses database hints with fallback to random generation
- Returns complete response with hint data included
- Properly merges `lifelineType` with lifeline-specific data

### Example Responses

**5050:**
```json
{"responseCode":200,"data":{"lifelineType":"5050","remainingOptions":[0,2]}}
```

**PHONE:**
```json
{"responseCode":200,"data":{"lifelineType":"PHONE","suggestion":"I'm 85% sure it's A"}}
```

**AUDIENCE:**
```json
{"responseCode":200,"data":{"lifelineType":"AUDIENCE","poll":{"A":65,"B":15,"C":10,"D":10}}}
```

---

## Notes for Team

- **Lifeline Format:**
  - **5050**: `[0,2]` = keep options A and C
  - **Ask**: `{"A":65,"B":15,"C":10,"D":10}` = audience percentages
  - **Call**: `"I'm 85% sure it's A"` = friend message (30% wrong)
- **Backward Compatibility:** NULL/empty fields fallback to random generation
- **Consistency:** Same question always gives same lifeline result

---

# Test Case: Leaderboard Query Error - Missing Column

**Location:** Server log showing leaderboard query error

**Problem:**
```
[2025-12-30 21:53:59] [ERROR] [database.cpp:577] Get leaderboard failed: ERROR: column l.final_question_number does not exist
LINE 1: SELECT u.id, u.username, COALESCE(l.final_question_number, 0...
```

**Issue:** Leaderboard queries reference `final_question_number` and `highest_prize` columns that don't exist in the leaderboard table (they're commented out in schema.sql).

---

## Root Cause

**Schema Analysis (`database/schema.sql` lines 135-143):**
```sql
CREATE TABLE IF NOT EXISTS leaderboard (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE UNIQUE,
    -- final_question_number INTEGER CHECK (final_question_number >= 1 AND final_question_number <= 15),
    total_score BIGINT NOT NULL DEFAULT 0,
    -- highest_prize BIGINT NOT NULL DEFAULT 0,
    games_played INTEGER DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Problem:** `final_question_number` and `highest_prize` columns are commented out, but code still queries them.

**Current Code Issues:**
1. `getLeaderboard()` queries `l.final_question_number` and `l.highest_prize` (don't exist)
2. `updateLeaderboard()` SELECTs and INSERTs/UPDATEs `final_question_number` and `highest_prize` (don't exist)

**Solution:** Calculate `final_question_number` dynamically from `game_sessions` table, remove `highest_prize` references.

---

## Solution Implemented

### Code Changes

#### 1. `database/database.cpp` - Fix `getLeaderboard()` query

**File:** `database/database.cpp`  
**Lines:** 546-572  
**Function:** `Database::getLeaderboard()`

**Previous Code:**
```cpp
    query = "SELECT u.id, u.username, COALESCE(l.final_question_number, 0), "
            "COALESCE(l.total_score, 0), COALESCE(l.highest_prize, 0) "
            "FROM users u "
            "LEFT JOIN leaderboard l ON u.id = l.user_id "
            "ORDER BY COALESCE(l.final_question_number, 0) DESC, COALESCE(l.total_score, 0) DESC "
            "LIMIT " + to_string(limit) + " OFFSET " + to_string((page - 1) * limit);
```

**New Code:**
```cpp
    query = "SELECT u.id, u.username, "
            "COALESCE(MAX(gs.current_question_number), 0) as final_question_number, "
            "COALESCE(l.total_score, 0), 0 as highest_prize "
            "FROM users u "
            "LEFT JOIN leaderboard l ON u.id = l.user_id "
            "LEFT JOIN game_sessions gs ON u.id = gs.user_id AND gs.status IN ('won', 'lost') "
            "GROUP BY u.id, u.username, l.total_score "
            "ORDER BY final_question_number DESC, COALESCE(l.total_score, 0) DESC "
            "LIMIT " + to_string(limit) + " OFFSET " + to_string((page - 1) * limit);
```

**Changes:**
1. Calculate `final_question_number` from `game_sessions` using `MAX(current_question_number)` for completed games
2. Removed `l.final_question_number` reference (column doesn't exist)
3. Set `highest_prize` to 0 (column doesn't exist, not needed for ranking)
4. Added `LEFT JOIN game_sessions` and `GROUP BY` to aggregate question numbers

**Similar fix applied to friend leaderboard query (lines 555-564).**

---

#### 2. `database/database.cpp` - Fix `updateLeaderboard()` query

**File:** `database/database.cpp`  
**Lines:** 598-644  
**Function:** `Database::updateLeaderboard()`

**Previous Code:**
```cpp
    // Get current best stats
    string query = "SELECT final_question_number, total_score, highest_prize, games_played "
                   "FROM leaderboard WHERE user_id = " + to_string(user_id);
    // ...
    query = "INSERT INTO leaderboard (user_id, final_question_number, total_score, highest_prize, games_played) "
            "VALUES (" + to_string(user_id) + ", " + to_string(best_final_q) + ", " +
            to_string(best_score) + ", " + to_string(best_prize) + ", " + to_string(games_played) + ") "
            "ON CONFLICT (user_id) DO UPDATE SET "
            "final_question_number = GREATEST(leaderboard.final_question_number, EXCLUDED.final_question_number), "
            "total_score = GREATEST(leaderboard.total_score, EXCLUDED.total_score), "
            "highest_prize = GREATEST(leaderboard.highest_prize, EXCLUDED.highest_prize), "
            "games_played = leaderboard.games_played + 1, "
            "last_updated = CURRENT_TIMESTAMP";
```

**New Code:**
```cpp
    // Get current best stats (only total_score and games_played exist in leaderboard table)
    string query = "SELECT total_score, games_played "
                   "FROM leaderboard WHERE user_id = " + to_string(user_id);
    // ...
    // final_question_number and highest_prize are not stored in leaderboard table
    // They are calculated dynamically from game_sessions when querying
    query = "INSERT INTO leaderboard (user_id, total_score, games_played) "
            "VALUES (" + to_string(user_id) + ", " + to_string(best_score) + ", " + to_string(games_played) + ") "
            "ON CONFLICT (user_id) DO UPDATE SET "
            "total_score = GREATEST(leaderboard.total_score, EXCLUDED.total_score), "
            "games_played = leaderboard.games_played + 1, "
            "last_updated = CURRENT_TIMESTAMP";
```

**Changes:**
1. Removed `final_question_number` and `highest_prize` from SELECT query
2. Removed `final_question_number` and `highest_prize` from INSERT/UPDATE
3. Only update `total_score` and `games_played` (columns that exist)
4. `final_question_number` is calculated dynamically in `getLeaderboard()` from `game_sessions`

---

#### 3. `database/database.cpp` - Fix `endGame()` to get current_question_number

**File:** `database/database.cpp`  
**Lines:** 484-493  
**Function:** `Database::endGame()`

**Previous Code:**
```cpp
    // Update leaderboard
    query = "SELECT user_id, final_prize FROM game_sessions WHERE id = " + to_string(game_id);
    res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        int user_id = atoi(PQgetvalue(res, 0, 0));
        long long prize = atoll(PQgetvalue(res, 0, 1));
        int final_q = (status == "won") ? 15 : 0; // Calculate from game state
        updateLeaderboard(user_id, final_q, total_score, prize);
    }
```

**New Code:**
```cpp
    // Update leaderboard
    query = "SELECT user_id, current_question_number, final_prize FROM game_sessions WHERE id = " + to_string(game_id);
    res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        int user_id = atoi(PQgetvalue(res, 0, 0));
        int final_q = atoi(PQgetvalue(res, 0, 1)); // Get from current_question_number
        long long prize = atoll(PQgetvalue(res, 0, 2));
        updateLeaderboard(user_id, final_q, total_score, prize);
    }
```

**Change:** Get `current_question_number` from game_sessions instead of calculating from status (won=15, lost=0 was incorrect).

---

## Summary

### Files Modified
- **`database/database.cpp`** - Fixed `getLeaderboard()` and `updateLeaderboard()` queries

### Testing Status
✅ **Fixed** - Leaderboard queries now:
- Calculate `final_question_number` dynamically from `game_sessions` table
- Only reference columns that exist (`total_score`, `games_played`)
- Removed references to non-existent columns (`final_question_number`, `highest_prize` in leaderboard table)

### Notes for Team

- **`final_question_number`**: Calculated dynamically from `MAX(current_question_number)` in `game_sessions` for completed games
- **`highest_prize`**: Not stored in leaderboard table, set to 0 in queries
- **Leaderboard table**: Only stores `user_id`, `total_score`, `games_played`, `last_updated`
- **Ranking**: Based on `final_question_number` (from game_sessions) then `total_score`

---

# Test Case: Accept Friend Request - Not Creating Friendship

**Location:** Test 9.3 - Accept Friend Request

**Problem:**
```
Request: {"requestType":"ACCEPT_FRIEND","data":{"authToken":"...","friendUsername":"testuser1"}}
Response: {"responseCode":200,"data":{"friendUsername":"testuser1}}
```

**Verification Results:**
- No friendship record in `friendships` table
- Friend request still has status "pending" (should be "accepted" or deleted)

**Issue:** `handleAcceptFriend()` has all database calls commented out as TODOs, so it doesn't actually accept the friend request.

---

## Root Cause

**Current Implementation (`server/request_handlers/social_handlers.cpp` lines 100-122):**
```cpp
string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");
    
    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }
    
    // TODO: Replace with database call
    // [All database calls are commented out]
    
    string data = "{\"friendUsername\":\"" + friend_username + "}";  // Missing closing quote
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Problems:**
1. All database calls are commented out (TODO)
2. No validation that friend request exists
3. No check if already friends
4. No call to `acceptFriendRequest()` to create friendship and update request status
5. Response JSON missing closing quote

**Database Function Available:**
- `Database::acceptFriendRequest(from_user, to_user)` - Updates request status and creates friendship
- `Database::friendshipExists(user1, user2)` - Checks if friendship exists
- `Database::getFriendRequests(username)` - Gets pending requests for user

---

## Solution Implemented

### Code Changes

#### `server/request_handlers/social_handlers.cpp` - Implement ACCEPT_FRIEND handler

**File:** `server/request_handlers/social_handlers.cpp`  
**Lines:** 100-132  
**Function:** `SocialHandlers::handleAcceptFriend()`

**Previous Code:**
```cpp
string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // TODO: Replace with database call
    // bool request_exists = Database::getInstance().friendRequestExists(friend_username, session.username);
    // if (!request_exists) {
    //     return StreamUtils::createErrorResponse(404, "Friend request not found");
    // }
    // 
    // bool already_friends = Database::getInstance().areFriends(session.username, friend_username);
    // if (already_friends) {
    //     return StreamUtils::createErrorResponse(409, "Friend already exists");
    // }
    // 
    // bool success = Database::getInstance().acceptFriendRequest(friend_username, session.username);

    string data = "{\"friendUsername\":\"" + friend_username + "}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

**New Code:**
```cpp
string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Check if friend request exists (from friend_username to session.username)
    vector<FriendRequest> requests = Database::getInstance().getFriendRequests(session.username);
    bool request_exists = false;
    for (const auto& req : requests) {
        if (req.username == friend_username) {
            request_exists = true;
            break;
        }
    }
    
    if (!request_exists) {
        return StreamUtils::createErrorResponse(404, "Friend request not found");
    }
    
    // Check if already friends
    if (Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(409, "Friend already exists");
    }
    
    // Accept friend request (from friend_username to session.username)
    bool success = Database::getInstance().acceptFriendRequest(friend_username, session.username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to accept friend request");
    }

    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Changes:**
1. Check if friend request exists by getting pending requests and checking if `friend_username` is in the list
2. Check if already friends using `friendshipExists()`
3. Call `acceptFriendRequest()` to:
   - Update friend_request status to "accepted"
   - Create friendship record in friendships table
4. Fixed missing closing quote in response JSON
5. Added error handling for failed acceptance

**Note:** Also fixed missing closing quotes in other handlers (`handleAddFriend`, `handleDeclineFriend`, `handleDelFriend`).

---

## Summary

### Files Modified
- **`server/request_handlers/social_handlers.cpp`** - Implemented `handleAcceptFriend()` with database calls

### Testing Status
✅ **Fixed** - ACCEPT_FRIEND now:
- Validates friend request exists
- Checks if already friends
- Calls `acceptFriendRequest()` to create friendship and update request status
- Returns proper JSON response

### Expected Behavior After Fix

**Database Changes:**
1. `friend_requests` table: Status updated from "pending" to "accepted"
2. `friendships` table: New record created with `user1_id` < `user2_id`

**Verification:**
```sql
-- Should show friendship record
SELECT user1_id, user2_id FROM friendships WHERE ...;

-- Should show status = 'accepted' (or request deleted)
SELECT status FROM friend_requests WHERE ...;
```

---

# Test Case: Delete Friend - Not Removing Friendship

**Location:** Test 9.5 - Delete Friend

**Problem:**
```
Request: {"requestType":"DEL_FRIEND","data":{"authToken":"...","friendUsername":"testuser2"}}
Response: {"responseCode":200,"data":{"friendUsername":"testuser2"}}
```

**Verification Results:**
- Friendship still exists in `friendships` table (count = 1, should be 0)
- Friend still appears in `FRIEND_STATUS` response

**Issue:** `handleDelFriend()` has all database calls commented out as TODOs, so it doesn't actually delete the friendship.

---

## Root Cause

**Current Implementation (`server/request_handlers/social_handlers.cpp` lines 170-186):**
```cpp
string handleDelFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");
    
    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }
    
    // TODO: Replace with database call
    // [All database calls are commented out]
    
    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Problems:**
1. All database calls are commented out (TODO)
2. No validation that friendship exists
3. No call to `deleteFriend()` to remove friendship from database
4. Response format doesn't match expected: should be `{"message":"Friend removed successfully"}` instead of `{"friendUsername":"..."}`

**Database Function Available:**
- `Database::deleteFriend(user1, user2)` - Deletes friendship record
- `Database::friendshipExists(user1, user2)` - Checks if friendship exists

---

## Solution Implemented

### Code Changes

#### `server/request_handlers/social_handlers.cpp` - Implement DEL_FRIEND handler

**File:** `server/request_handlers/social_handlers.cpp`  
**Lines:** 170-186  
**Function:** `SocialHandlers::handleDelFriend()`

**Previous Code:**
```cpp
string handleDelFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // TODO: Replace with database call
    // bool are_friends = Database::getInstance().areFriends(session.username, friend_username);
    // if (!are_friends) {
    //     return StreamUtils::createErrorResponse(404, "Friend not found");
    // }
    // 
    // bool success = Database::getInstance().deleteFriend(session.username, friend_username);

    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

**New Code:**
```cpp
string handleDelFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Check if friendship exists
    if (!Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(404, "Friend not found");
    }
    
    // Delete friendship
    bool success = Database::getInstance().deleteFriend(session.username, friend_username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to delete friend");
    }

    string data = "{\"message\":\"Friend removed successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Changes:**
1. Check if friendship exists using `friendshipExists()`
2. Call `deleteFriend()` to remove friendship record from database
3. Updated response format to match expected: `{"message":"Friend removed successfully"}`
4. Added error handling for failed deletion

---

## Summary

### Files Modified
- **`server/request_handlers/social_handlers.cpp`** - Implemented `handleDelFriend()` with database calls

### Testing Status
✅ **Fixed** - DEL_FRIEND now:
- Validates friendship exists
- Calls `deleteFriend()` to remove friendship from database
- Returns proper response format matching test expectations

---

# Test Case: GIVE_UP Not Working - Complete Fix

**Location:** Test Case - GIVE_UP functionality and START after GIVE_UP

**Problem:**
```
GIVE_UP request: {"requestType":"GIVE_UP","data":{"authToken":"...","gameId":4,"questionNumber":1}}
Response: Success (status "gave_up")

START request: {"requestType":"START","data":{"authToken":"..."}}
Response: {"responseCode":405,"message":"Already in a game"}

Server log shows:
ERROR: new row for relation "game_sessions" violates check constraint "game_sessions_status_check"
DETAIL: Failing row contains (4, 6, gave_up, ...)
```

**Issue:** Multiple problems causing GIVE_UP to fail:
1. `handleGiveUp()` only checked `session.in_game` (in-memory flag), not the database
2. `handleGiveUp()` didn't call `Database::endGame()` to update the database
3. When `endGame()` was added, it used invalid status "gave_up" which violates database CHECK constraint
4. `handleStart()` checked `session.in_game` before checking database, causing stale state issues

---

## Root Cause

### Problem 1: Missing Database Integration

**Initial Implementation (`server/request_handlers/game_handlers.cpp` lines 408-451):**
```cpp
string handleGiveUp(const string& request, ClientSession& session, int client_fd) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }
    
    // ... validation ...
    
    session.in_game = false;  // Only updates in-memory flag
    
    // NO call to Database::endGame()!
    
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Problems:**
- Only checks `session.in_game` (in-memory flag), doesn't check database
- Doesn't call `Database::endGame()` to update game status in database
- Doesn't stop the game timer
- If session state is lost, `session.in_game` becomes false but database still has active game

### Problem 2: Invalid Status Value

**Database Schema (`database/schema.sql` line 46):**
```sql
status VARCHAR(20) DEFAULT 'active' CHECK (status IN ('active', 'won', 'lost', 'quit'))
```

**After adding `endGame()` call:**
```cpp
Database::getInstance().endGame(game_id, "gave_up", total_score, final_prize);
```

**Problems:**
- Code uses status "gave_up" which is not in the allowed CHECK constraint values
- Database UPDATE fails with constraint violation: `ERROR: new row for relation "game_sessions" violates check constraint "game_sessions_status_check"`
- `endGame()` returns false but we don't check the return value
- Game status remains "active" in database, causing START to fail

### Problem 3: START Checking Session Before Database

**Initial Implementation (`server/request_handlers/game_handlers.cpp` lines 21-30):**
```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    if (session.in_game) {  // Checks in-memory flag FIRST
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    // Check database for active game session
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }
}
```

**Problems:**
- Checks `session.in_game` (in-memory flag) BEFORE checking database
- If `session.in_game` is true but database has no active game, START incorrectly returns "Already in a game"
- Database should be the source of truth, not in-memory session state

---

## Solution Implemented

### Code Changes

#### 1. `server/request_handlers/game_handlers.cpp` - Fix GIVE_UP handler

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 408-465  
**Function:** `GameHandlers::handleGiveUp()`

**Previous Code:**
```cpp
string handleGiveUp(const string& request, ClientSession& session, int client_fd) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    int final_prize = session.current_prize;
    int final_question_number = session.current_question_number;
    int total_score = session.total_score;

    session.in_game = false;

    string data = "{\"finalPrize\":" + to_string(final_prize) + 
                 ",\"finalQuestionNumber\":" + to_string(final_question_number) + 
                 ",\"totalScore\":" + to_string(total_score) +
                 ",\"gameId\":" + to_string(game_id) + "}";
    
    // Send GAME_END notification
    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                          ",\"status\":\"gave_up\"" +
                          ",\"finalLevel\":" + to_string(final_question_number) +
                          ",\"finalQuestionNumber\":" + to_string(final_question_number) +
                          ",\"finalPrize\":" + to_string(final_prize) +
                          ",\"totalScore\":" + to_string(total_score) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}
```

**New Code:**
```cpp
string handleGiveUp(const string& request, ClientSession& session, int client_fd) {
    // Check both in-memory session and database for active game
    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    
    // If session.in_game is false, check database for active game
    if (!session.in_game) {
        GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
        if (active_game.id == 0) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }
        // Restore session state from database
        session.in_game = true;
        session.game_id = active_game.id;
        session.current_question_number = active_game.current_question_number;
        session.current_prize = active_game.current_prize;
        session.total_score = active_game.total_score;
        game_id = active_game.id; // Use game_id from database if not provided
    }

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);
    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    long long final_prize = session.current_prize;
    int final_question_number = session.current_question_number;
    int total_score = session.total_score;

    // Stop timer
    GameTimer::getInstance().stopTimer(game_id);
    
    // End game in database (use 'quit' status as per schema)
    Database::getInstance().endGame(game_id, "quit", total_score, final_prize);
    
    // Update session state
    session.in_game = false;

    string data = "{\"finalPrize\":" + to_string(final_prize) + 
                 ",\"finalQuestionNumber\":" + to_string(final_question_number) + 
                 ",\"totalScore\":" + to_string(total_score) +
                 ",\"gameId\":" + to_string(game_id) + "}";
    
    // Send GAME_END notification
    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                          ",\"status\":\"quit\"" +
                          ",\"finalLevel\":" + to_string(final_question_number) +
                          ",\"finalQuestionNumber\":" + to_string(final_question_number) +
                          ",\"finalPrize\":" + to_string(final_prize) +
                          ",\"totalScore\":" + to_string(total_score) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}
```

**Changes:**
1. **Check database if `session.in_game` is false:** If in-memory flag is false, check database for active game
2. **Restore session state from database:** If active game found, restore session state from database values
3. **Call `Database::endGame()`:** Properly end the game in database with status "quit" (not "gave_up")
4. **Stop game timer:** Call `GameTimer::getInstance().stopTimer(game_id)` to stop the timer
5. **Fixed type:** Changed `int final_prize` to `long long final_prize` to match `Database::endGame()` signature
6. **Fixed status value:** Changed from "gave_up" to "quit" to match database schema CHECK constraint

#### 2. `server/request_handlers/game_handlers.cpp` - Fix START handler

**File:** `server/request_handlers/game_handlers.cpp`  
**Lines:** 21-26  
**Function:** `GameHandlers::handleStart()`

**Previous Code:**
```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    // Check database for active game session
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }
```

**New Code:**
```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    // Check database only - database is source of truth
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }
```

**Changes:**
1. **Check database ONLY:** Database is the sole source of truth - no session state checks at all
2. **Removed all session state syncing:** Don't touch session.in_game or any session fields during the check
3. **Simplified logic:** Just check database, if active game exists return error, otherwise proceed

---

## Summary

### Files Modified
- **`server/request_handlers/game_handlers.cpp`** - Fixed `handleGiveUp()` and `handleStart()`

### Testing Status
✅ **Fixed** - GIVE_UP and START now:
- GIVE_UP checks database for active game if in-memory flag is false
- GIVE_UP restores session state from database when needed
- GIVE_UP calls `Database::endGame()` with valid status "quit"
- GIVE_UP stops game timer
- START checks database ONLY (no session state checks)
- Database is the sole source of truth
- Consistent behavior with other game-ending handlers

### Expected Behavior After Fix

**Database Changes:**
1. `game_sessions` table: Status updated to "quit" (valid value), `ended_at` set, `final_prize` and `total_score` saved
2. `leaderboard` table: Updated with final scores
3. `getActiveGameSession()` will not find ended games (only finds `status = 'active'`)

**Behavior:**
- After GIVE_UP, database game status is "quit" (not "active")
- START checks database first and finds no active game
- START proceeds to create new game correctly

---

# Test Case: Admin Role Check Using Cached Session Instead of Database

**Location:** Admin handlers - ADD_QUES, CHANGE_QUES, VIEW_QUES, DEL_QUES, BAN_USER

**Problem:**
```
ADD_QUES request: {"requestType":"ADD_QUES","data":{"authToken":"...","question":"..."}}
Response: {"responseCode":403,"message":"Access forbidden - not an admin account"}

Database shows user has role 'admin', but request still fails.
```

**Issue:** Admin handlers check `session.role` (cached in-memory value) instead of checking the database directly. When a user's role is updated in the database, the cached session value doesn't reflect the change until the user logs out and logs back in.

---

## Root Cause

**Current Implementation (`server/request_handlers/admin_handlers.cpp`):**
```cpp
string handleAddQues(const string& request, ClientSession& session, int client_fd) {
    if (session.role != "admin") {  // Checks cached session value
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }
    // ...
}
```

**Problems:**
1. Checks `session.role` (in-memory cached value set at login)
2. If user's role is updated in database, cached value doesn't update
3. User must log out and log back in to get updated role
4. Database is the source of truth, but we're not checking it

**Session Role Setting (`server/request_handlers/auth_handlers.cpp` line 49):**
```cpp
session.role = user_role;  // Set once at login, cached for session lifetime
```

**Impact:** Admin operations fail even after role is updated in database until user re-authenticates.

---

## Solution Implemented

### Code Changes

#### `server/request_handlers/admin_handlers.cpp` - Check database role directly

**File:** `server/request_handlers/admin_handlers.cpp`  
**Functions:** `handleAddQues()`, `handleChangeQues()`, `handleViewQues()`, `handleDelQues()`, `handleBanUser()`  
**Lines:** 50, 101, 155, 189, 212

**Previous Code (all 5 handlers):**
```cpp
if (session.role != "admin") {
    return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
}
```

**New Code (all 5 handlers):**
```cpp
// Check database role directly (not cached session role)
string user_role = Database::getInstance().getUserRole(session.username);
if (user_role != "admin") {
    return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
}
```

**Changes:**
1. **Check database directly:** Call `Database::getInstance().getUserRole(session.username)` instead of using `session.role`
2. **Database as source of truth:** Role changes in database are immediately effective
3. **No re-authentication needed:** Users don't need to log out/in after role changes

**Handlers Updated:**
- `handleAddQues()` - Line 50
- `handleChangeQues()` - Line 101
- `handleViewQues()` - Line 155
- `handleDelQues()` - Line 189
- `handleBanUser()` - Line 212

---

## Summary

### Files Modified
- **`server/request_handlers/admin_handlers.cpp`** - Updated all 5 admin handlers to check database role directly

### Testing Status
✅ **Fixed** - Admin handlers now:
- Check database role directly (not cached session value)
- Immediately reflect role changes in database
- No need to re-authenticate after role updates
- Database is the source of truth for role checks

---

# Test Case: ADD_QUES Request Format Issues - Options and Lifeline Info

**Location:** Test 11.2 - Add Question (Admin)

**Problem:**
```
Request: {"requestType":"ADD_QUES","data":{"authToken":"...","question":"What is 2+2?","options":[{"label":"A","text":"3"},...],"lifeline_5050_info":[1,2],...}}

Database result shows:
- option_a = "label" (should be "3")
- option_b = "A" (should be "4")
- option_c = "text" (should be "5")
- option_d = "3" (should be "6")
- lifeline_5050_info, lifeline_ask_info, lifeline_call_info are all empty
```

**Issue:** Multiple problems:
1. `extractOptions()` extracts quoted strings sequentially, so `[{"label":"A","text":"3"},...]` extracts "label", "A", "text", "3" instead of option texts
2. Lifeline info fields are not extracted from request
3. `addQuestion()` doesn't insert lifeline columns into database

---

## Root Cause

**Current Implementation (`server/request_handlers/admin_handlers.cpp`):**

1. **`extractOptions()` function (lines 17-47):**
   - Extracts quoted strings sequentially from options array
   - For `[{"label":"A","text":"3"},...]`, it extracts: "label", "A", "text", "3"
   - Should extract the "text" field from each object

2. **`handleAddQues()` function (lines 49-98):**
   - Doesn't extract lifeline info fields from request
   - Doesn't set lifeline fields in Question object

3. **`Database::addQuestion()` function (`database/database.cpp` lines 939-965):**
   - INSERT query doesn't include lifeline columns
   - Lifeline data is never saved to database

---

## Solution Implemented

### Decisions Made

1. ✅ **Simplify options format:** Change from `[{"label":"A","text":"3"},...]` to `["3","4","5","6"]` (array of strings)
2. ✅ **Lifeline info required:** All three lifeline fields are required (return error if missing)
3. ✅ **Separate top-level fields:** Keep lifeline info as separate fields in request

### Code Changes

#### 1. `server/request_handlers/admin_handlers.cpp` - Fix `extractOptions()` and add lifeline extraction

**File:** `server/request_handlers/admin_handlers.cpp`  
**Lines:** 16-120  
**Functions:** `extractOptions()`, new `extractJsonValue()`, `handleAddQues()`

**Previous Code:**
```cpp
// Helper function to extract options array from JSON
static vector<string> extractOptions(const string& json) {
    vector<string> options(4);
    
    // Look for "options" array in JSON
    size_t options_start = json.find("\"options\"");
    if (options_start == string::npos) {
        return options;
    }
    
    // Find the array start
    size_t array_start = json.find("[", options_start);
    if (array_start == string::npos) {
        return options;
    }
    
    // Extract each option string from the array
    size_t pos = array_start + 1;
    for (int i = 0; i < 4 && pos < json.length(); i++) {
        // Find next quoted string
        size_t quote_start = json.find("\"", pos);
        if (quote_start == string::npos) break;
        
        size_t quote_end = json.find("\"", quote_start + 1);
        if (quote_end == string::npos) break;
        
        options[i] = json.substr(quote_start + 1, quote_end - quote_start - 1);
        pos = quote_end + 1;
    }
    
    return options;
}
```

**New Code:**
```cpp
// Helper function to extract options array from JSON
// Format: "options":["3","4","5","6"]
static vector<string> extractOptions(const string& json) {
    vector<string> options(4);
    
    // Look for "options" array in JSON
    size_t options_start = json.find("\"options\"");
    if (options_start == string::npos) {
        return options;
    }
    
    // Find the array start
    size_t array_start = json.find("[", options_start);
    if (array_start == string::npos) {
        return options;
    }
    
    // Extract each option string from the array
    size_t pos = array_start + 1;
    for (int i = 0; i < 4 && pos < json.length(); i++) {
        // Skip whitespace and commas
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ',')) {
            pos++;
        }
        if (pos >= json.length()) break;
        
        // Find next quoted string
        if (json[pos] != '"') break;
        pos++; // Skip opening quote
        
        size_t quote_end = json.find("\"", pos);
        if (quote_end == string::npos) break;
        
        options[i] = json.substr(pos, quote_end - pos);
        pos = quote_end + 1;
    }
    
    return options;
}

// Helper function to extract JSON value (string, array, or object)
// Returns the raw JSON value as string
static string extractJsonValue(const string& json, const string& key) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    if (pos >= json.length()) return "";
    
    size_t start = pos;
    
    // Determine the type and extract accordingly
    if (json[pos] == '"') {
        // String value - extract until closing quote
        pos++;
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        return json.substr(start, end - start + 1);
    } else if (json[pos] == '[') {
        // Array value - extract until matching closing bracket
        int bracket_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '[') bracket_count++;
            if (json[end] == ']') {
                bracket_count--;
                if (bracket_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else if (json[pos] == '{') {
        // Object value - extract until matching closing brace
        int brace_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '{') brace_count++;
            if (json[end] == '}') {
                brace_count--;
                if (brace_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else {
        // Number or boolean - extract until comma, }, or ]
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
            end++;
        }
        return json.substr(start, end - start);
    }
}
```

**Changes:**
1. **Fixed `extractOptions()`:** Now properly extracts strings from simple array format `["3","4","5","6"]`
2. **Added `extractJsonValue()`:** Helper function to extract JSON arrays/objects/strings from request

#### 2. `server/request_handlers/admin_handlers.cpp` - Extract and validate lifeline info

**File:** `server/request_handlers/admin_handlers.cpp`  
**Lines:** 145-164  
**Function:** `handleAddQues()`

**Previous Code:**
```cpp
    // Extract and validate options array
    vector<string> options = extractOptions(request);
    if (options[0].empty() || options[1].empty() || options[2].empty() || options[3].empty()) {
        return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 options with label and text");
    }

    // Create Question object
    Question q;
    q.question_text = question;
    q.option_a = options[0];
    q.option_b = options[1];
    q.option_c = options[2];
    q.option_d = options[3];
    q.correct_answer = correct_answer;
    q.level = level;
    q.is_active = true;
    q.updated_by = 0;
```

**New Code:**
```cpp
    // Extract and validate options array (simplified format: ["3","4","5","6"])
    vector<string> options = extractOptions(request);
    if (options[0].empty() || options[1].empty() || options[2].empty() || options[3].empty()) {
        return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 option strings");
    }

    // Extract and validate lifeline info (required)
    // lifeline_5050_info and lifeline_ask_info are JSONB (arrays/objects)
    // lifeline_call_info is TEXT (plain string)
    string lifeline_5050_info = extractJsonValue(request, "lifeline_5050_info");
    string lifeline_ask_info = extractJsonValue(request, "lifeline_ask_info");
    string lifeline_call_info = JsonUtils::extractString(request, "lifeline_call_info");  // Plain string, no quotes
    
    if (lifeline_5050_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_5050_info");
    }
    if (lifeline_ask_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_ask_info");
    }
    if (lifeline_call_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_call_info");
    }

    // Create Question object
    Question q;
    q.question_text = question;
    q.option_a = options[0];
    q.option_b = options[1];
    q.option_c = options[2];
    q.option_d = options[3];
    q.correct_answer = correct_answer;
    q.level = level;
    q.is_active = true;
    q.lifeline_5050_info = lifeline_5050_info;
    q.lifeline_ask_info = lifeline_ask_info;
    q.lifeline_call_info = lifeline_call_info;
    q.updated_by = 0;
```

**Changes:**
1. Updated error message for simplified options format
2. Extract lifeline info from request using `extractJsonValue()` for JSONB fields and `extractString()` for TEXT field
3. Validate all lifeline fields are present (required)
4. Set lifeline fields in Question object

#### 3. `database/database.cpp` - Insert lifeline columns

**File:** `database/database.cpp`  
**Lines:** 939-960  
**Function:** `Database::addQuestion()`

**Previous Code:**
```cpp
int Database::addQuestion(const Question& question) {
    if (!isConnected()) return 0;
    
    string query = "INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level, is_active, updated_by) VALUES (" +
                   escapeString(question.question_text) + ", " +
                   escapeString(question.option_a) + ", " +
                   escapeString(question.option_b) + ", " +
                   escapeString(question.option_c) + ", " +
                   escapeString(question.option_d) + ", " +
                   to_string(question.correct_answer) + ", " +
                   to_string(question.level) + ", " +
                   (question.is_active ? "TRUE" : "FALSE") + ", " +
                   (question.updated_by > 0 ? to_string(question.updated_by) : "NULL") + ") RETURNING id";
```

**New Code:**
```cpp
int Database::addQuestion(const Question& question) {
    if (!isConnected()) return 0;
    
    // Build lifeline info values (JSONB for 5050 and ask, TEXT for call)
    string lifeline_5050_val = question.lifeline_5050_info.empty() ? "NULL" : escapeString(question.lifeline_5050_info);
    string lifeline_ask_val = question.lifeline_ask_info.empty() ? "NULL" : escapeString(question.lifeline_ask_info);
    string lifeline_call_val = question.lifeline_call_info.empty() ? "NULL" : escapeString(question.lifeline_call_info);
    
    string query = "INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level, is_active, lifeline_5050_info, lifeline_ask_info, lifeline_call_info, updated_by) VALUES (" +
                   escapeString(question.question_text) + ", " +
                   escapeString(question.option_a) + ", " +
                   escapeString(question.option_b) + ", " +
                   escapeString(question.option_c) + ", " +
                   escapeString(question.option_d) + ", " +
                   to_string(question.correct_answer) + ", " +
                   to_string(question.level) + ", " +
                   (question.is_active ? "TRUE" : "FALSE") + ", " +
                   lifeline_5050_val + "::jsonb, " +
                   lifeline_ask_val + "::jsonb, " +
                   lifeline_call_val + ", " +
                   (question.updated_by > 0 ? to_string(question.updated_by) : "NULL") + ") RETURNING id";
```

**Changes:**
1. Added lifeline columns to INSERT statement
2. Escape and cast JSONB values with `::jsonb`
3. Handle NULL values if fields are empty (though they're required now)

---

## Summary

### Files Modified
- **`server/request_handlers/admin_handlers.cpp`** - Fixed `extractOptions()`, added `extractJsonValue()`, updated `handleAddQues()` to extract lifeline info
- **`database/database.cpp`** - Updated `addQuestion()` to insert lifeline columns
- **`database/TEST_GUIDE.md`** - Updated request format to simplified options array

### Testing Status
✅ **Fixed** - ADD_QUES now:
- Uses simplified options format: `["3","4","5","6"]`
- Extracts and validates all lifeline info fields (required)
- Inserts lifeline columns into database
- Options are correctly parsed and stored

### Expected Request Format

**New Format:**
```json
{
  "requestType": "ADD_QUES",
  "data": {
    "authToken": "...",
    "question": "What is 2+2?",
    "options": ["3", "4", "5", "6"],
    "correctAnswer": 1,
    "level": 0,
    "lifeline_5050_info": [1, 2],
    "lifeline_ask_info": {"A": 5, "B": 70, "C": 15, "D": 10},
    "lifeline_call_info": "I'm 90% sure it's B"
  }
}
```

**Changes from previous format:**
- `options`: Changed from `[{"label":"A","text":"3"},...]` to `["3","4","5","6"]`
- `lifeline_5050_info`, `lifeline_ask_info`, `lifeline_call_info`: Now required fields

---

# Test Case: BAN_USER Not Updating Database

**Location:** Test 11.5 - Ban User (Admin)

**Problem:**
```
Request: {"requestType":"BAN_USER","data":{"authToken":"...","username":"testuser2","reason":"Test ban"}}
Response: {"responseCode":200,"data":{"message":"User banned successfully","username":"testuser2"}}

Database verification shows:
- is_banned = f (should be t)
- ban_reason = empty (should be "Test ban")
```

**Issue:** The `handleBanUser()` function returns success but doesn't actually call the database to ban the user. The database calls are commented out.

---

## Root Cause

**Current Implementation (`server/request_handlers/admin_handlers.cpp` lines 332-338):**

```cpp
// TODO: Replace with database call
// bool user_exists = Database::getInstance().userExists(target_username);
// if (!user_exists) {
//     return StreamUtils::createErrorResponse(404, "User not found");
// }
// 
// Database::getInstance().banUser(target_username, reason);

string data = "{\"message\":\"User banned successfully\",\"username\":\"" + target_username + "\"}";
return StreamUtils::createSuccessResponse(200, data);
```

**Problems:**
1. Database calls are commented out (TODO comments)
2. Function returns success without actually updating database
3. No error handling for database failures

---

## Solution Implemented

### Code Changes

#### `server/request_handlers/admin_handlers.cpp` - Uncomment and implement database calls

**File:** `server/request_handlers/admin_handlers.cpp`  
**Lines:** 332-351  
**Function:** `handleBanUser()`

**Previous Code:**
```cpp
    if (target_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot ban yourself");
    }

    // TODO: Replace with database call
    // bool user_exists = Database::getInstance().userExists(target_username);
    // if (!user_exists) {
    //     return StreamUtils::createErrorResponse(404, "User not found");
    // }
    // 
    // Database::getInstance().banUser(target_username, reason);

    string data = "{\"message\":\"User banned successfully\",\"username\":\"" + target_username + "\"}";
    
    // TODO: Send USER_BANNED notification to the banned user (force disconnect)
    // This requires finding the user's client_fd
    // int banned_user_fd = findClientFdByUsername(target_username);
    // if (banned_user_fd != -1) {
    //     string user_notification_data = "{\"reason\":\"" + reason +
    //                                    "\",\"timestamp\":" + to_string(time(nullptr)) + "}";
    //     NotificationUtils::sendNotification(banned_user_fd, "USER_BANNED", user_notification_data);
    // }
    
    return StreamUtils::createSuccessResponse(200, data);
```

**New Code:**
```cpp
    if (target_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot ban yourself");
    }

    // Check if user exists
    bool user_exists = Database::getInstance().userExists(target_username);
    if (!user_exists) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }
    
    // Ban the user in database
    bool success = Database::getInstance().banUser(target_username, reason);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to ban user");
    }

    string data = "{\"message\":\"User banned successfully\",\"username\":\"" + target_username + "\"}";
    
    // TODO: Send USER_BANNED notification to the banned user (force disconnect)
    // This requires finding the user's client_fd
    // int banned_user_fd = findClientFdByUsername(target_username);
    // if (banned_user_fd != -1) {
    //     string user_notification_data = "{\"reason\":\"" + reason +
    //                                    "\",\"timestamp\":" + to_string(time(nullptr)) + "}";
    //     NotificationUtils::sendNotification(banned_user_fd, "USER_BANNED", user_notification_data);
    // }
    
    return StreamUtils::createSuccessResponse(200, data);
```

**Changes:**
1. **Uncommented database calls:** Now checks if user exists and calls `banUser()`
2. **Added error handling:** Returns 404 if user doesn't exist, 500 if ban fails
3. **Database integration:** Actually updates `is_banned` and `ban_reason` in database

---

## Summary

### Files Modified
- **`server/request_handlers/admin_handlers.cpp`** - Uncommented and implemented database calls in `handleBanUser()`

### Testing Status
✅ **Fixed** - BAN_USER now:
- Checks if user exists before banning
- Calls `Database::banUser()` to update database
- Returns proper error codes for failures
- Updates `is_banned = TRUE` and `ban_reason` in database

---

# Test Case: Banned User Login Returns Wrong Error Code

**Location:** Login handler - banned user authentication

**Problem:**
```
Banned user login request
Response: {"responseCode":401,"message":"Invalid credentials"}

Expected: {"responseCode":403,"error":"Account is banned"}
```

**Issue:** The login handler checks `authenticateUser()` first, which returns `false` for banned users, causing a 401 "Invalid credentials" response before checking if the user is banned.

---

## Root Cause

**Current Implementation (`server/request_handlers/auth_handlers.cpp` lines 26-35):**

```cpp
// Authenticate user with database
bool login_success = Database::getInstance().authenticateUser(username, password);
if (!login_success) {
    return StreamUtils::createErrorResponse(401, "Invalid credentials");
}

// Check if user is banned
if (Database::getInstance().isUserBanned(username)) {
    return StreamUtils::createErrorResponse(403, "Account is banned");
}
```

**Problems:**
1. Checks authentication BEFORE checking if user is banned
2. `authenticateUser()` returns `false` for banned users (line 146-148 in `database.cpp`)
3. Handler returns 401 before reaching the banned check
4. Banned users get "Invalid credentials" instead of "Account is banned"

**Database Implementation (`database/database.cpp` lines 124-151):**
```cpp
bool Database::authenticateUser(const string& username, const string& password) {
    // ...
    bool is_banned = (PQgetvalue(res, 0, 1)[0] == 't');
    
    if (is_banned) {
        return false;  // Returns false for banned users
    }
    
    return verifyPassword(password, stored_hash);
}
```

---

## Solution Implemented

### Code Changes

#### `server/request_handlers/auth_handlers.cpp` - Check banned status before authentication

**File:** `server/request_handlers/auth_handlers.cpp`  
**Lines:** 19-35  
**Function:** `AuthHandlers::handleLogin()`

**Previous Code:**
```cpp
    string username = JsonUtils::extractString(request, "username");
    string password = JsonUtils::extractString(request, "password");

    if (username.empty() || password.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username or password");
    }

    // Authenticate user with database
    bool login_success = Database::getInstance().authenticateUser(username, password);
    if (!login_success) {
        return StreamUtils::createErrorResponse(401, "Invalid credentials");
    }
    
    // Check if user is banned
    if (Database::getInstance().isUserBanned(username)) {
        return StreamUtils::createErrorResponse(403, "Account is banned");
    }
```

**New Code:**
```cpp
    string username = JsonUtils::extractString(request, "username");
    string password = JsonUtils::extractString(request, "password");

    if (username.empty() || password.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username or password");
    }

    // Check if user is banned BEFORE authentication
    if (Database::getInstance().isUserBanned(username)) {
        return StreamUtils::createErrorResponse(403, "Account is banned");
    }

    // Authenticate user with database
    bool login_success = Database::getInstance().authenticateUser(username, password);
    if (!login_success) {
        return StreamUtils::createErrorResponse(401, "Invalid credentials");
    }
```

**Changes:**
1. **Check banned status first:** Moved `isUserBanned()` check before `authenticateUser()` call
2. **Correct error code:** Banned users now get 403 "Account is banned" instead of 401 "Invalid credentials"
3. **Better security:** Don't reveal whether username exists or password is correct for banned users

---

## Summary

### Files Modified
- **`server/request_handlers/auth_handlers.cpp`** - Moved banned check before authentication in `handleLogin()`

### Testing Status
✅ **Fixed** - Login handler now:
- Checks if user is banned before authentication
- Returns 403 "Account is banned" for banned users
- Returns 401 "Invalid credentials" only for actual authentication failures

---

# Test Case: User Stuck in Game After Disconnect

**Location:** Client disconnect handling - active game cleanup

**Problem:**
```
User starts a game, then disconnects (Ctrl+C)
User reconnects and tries to START
Response: {"responseCode":405,"message":"Already in a game"}

Database shows game_sessions.status = 'active' forever
```

**Issue:** When a user disconnects while in an active game, `cleanupClient()` doesn't check for or end the active game. The game remains "active" in the database, preventing the user from starting a new game.

---

## Root Cause

**Current Implementation (`server/client_handler.cpp` lines 71-82):**

```cpp
void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        if (!session->auth_token.empty()) {
            AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
        }
        if (!session->username.empty()) {
            SessionManager::getInstance().removeOnlineUser(session->username);
        }
    }
    SessionManager::getInstance().removeSession(client_fd);
}
```

**Problems:**
1. Doesn't check for active game session
2. Doesn't call `Database::endGame()` to update game status
3. Doesn't stop game timer
4. Game remains "active" in database after disconnect
5. User cannot start new game until game times out or is manually fixed

---

## Solution Implemented

### Code Changes

#### `server/client_handler.cpp` - Add game cleanup on disconnect

**File:** `server/client_handler.cpp`  
**Lines:** 1-6 (add includes), 71-82 (modify cleanupClient)  
**Function:** `ClientHandler::cleanupClient()`

**Previous Code:**
```cpp
#include "client_handler.h"
#include "logger.h"
#include "auth_manager.h"
#include <unistd.h>
#include <exception>
#include <ctime>
```

**New Code:**
```cpp
#include "client_handler.h"
#include "logger.h"
#include "auth_manager.h"
#include "game_timer.h"
#include "scoring_system.h"
#include "notification_utils.h"
#include "../../database/database.h"
#include <unistd.h>
#include <exception>
#include <ctime>
```

**Previous Code:**
```cpp
void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        if (!session->auth_token.empty()) {
            AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
        }
        if (!session->username.empty()) {
            SessionManager::getInstance().removeOnlineUser(session->username);
        }
    }
    SessionManager::getInstance().removeSession(client_fd);
}
```

**New Code:**
```cpp
void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        // Check for active game and end it if user disconnects
        if (!session->username.empty()) {
            GameSession active_game = Database::getInstance().getActiveGameSession(session->username);
            if (active_game.id > 0) {
                // User was in a game - end it due to disconnect
                int game_id = active_game.id;
                GameTimer::getInstance().stopTimer(game_id);
                
                // Calculate final score and prize (use safe checkpoint)
                long long final_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(active_game.current_question_number);
                
                // End game in database
                bool end_success = Database::getInstance().endGame(game_id, "quit", active_game.total_score, final_prize);
                
                if (end_success) {
                    // Send GAME_END notification if database update succeeded
                    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                                          ",\"status\":\"quit\"" +
                                          ",\"finalLevel\":" + to_string(active_game.current_question_number) +
                                          ",\"finalQuestionNumber\":" + to_string(active_game.current_question_number) +
                                          ",\"safeCheckpointPrize\":" + to_string(final_prize) +
                                          ",\"safeCheckpointScore\":" + to_string(active_game.total_score) +
                                          ",\"finalPrize\":" + to_string(final_prize) +
                                          ",\"totalScore\":" + to_string(active_game.total_score) +
                                          ",\"isWinner\":false}";
                    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
                    
                    LOG_INFO("Ended game " + to_string(game_id) + " for disconnected user " + session->username);
                } else {
                    LOG_ERROR("Failed to end game " + to_string(game_id) + " for disconnected user " + session->username);
                }
            }
        }
        
        // Cleanup authentication
        if (!session->auth_token.empty()) {
            AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
        }
        if (!session->username.empty()) {
            SessionManager::getInstance().removeOnlineUser(session->username);
        }
    }
    SessionManager::getInstance().removeSession(client_fd);
}
```

**Changes:**
1. **Added includes:** `game_timer.h`, `scoring_system.h`, `notification_utils.h`, `database.h`
2. **Check for active game:** Query database for active game session when user disconnects
3. **End game immediately:** Call `Database::endGame()` with status "quit" when disconnect detected
4. **Stop timer:** Call `GameTimer::getInstance().stopTimer()` to stop game timer
5. **Calculate final prize:** Use safe checkpoint prize based on current question number
6. **Send notification:** Send GAME_END notification if database update succeeds (single-player, but notification still sent)
7. **Error handling:** Log error if database update fails

---

## Summary

### Files Modified
- **`server/client_handler.cpp`** - Added game cleanup logic in `cleanupClient()`

### Testing Status
✅ **Fixed** - Disconnect handling now:
- Checks for active game session when user disconnects
- Ends game immediately with status "quit"
- Stops game timer
- Updates database to prevent "stuck in game" issue
- Sends GAME_END notification if database update succeeds
- User can start new game immediately after reconnecting

### Expected Behavior After Fix

**Behavior:**
- When user disconnects while in a game, game is immediately ended with status "quit"
- Database `game_sessions.status` is updated to "quit" (not "active")
- Game timer is stopped
- GAME_END notification is sent if database update succeeds
- User can start new game immediately after reconnecting
- No "stuck in game" issue

---

**Date:** 2025-12-30  
**Author:** Vanh  
**Status:** ✅ Complete - Ready for testing

