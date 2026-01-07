# Server Integration Guide

## Overview

This document provides instructions for integrating the database and game logic modules into the server codebase.

## Code Structure

The server codebase has been refactored into modular components:

```
server/
├── server_core.h/cpp          # Server lifecycle management (start, stop, accept connections)
├── session_manager.h/cpp      # Client session management
├── auth_manager.h/cpp         # Authentication & authorization (tokens, password validation)
├── request_router.h/cpp       # Request routing to appropriate handlers
├── request_handlers/          # Request handlers by category
│   ├── auth_handlers.h/cpp    # LOGIN, REGISTER, LOGOUT
│   ├── game_handlers.h/cpp    # START, ANSWER, LIFELINE, GIVE_UP, RESUME, LEAVE_GAME
│   ├── social_handlers.h/cpp  # LEADERBOARD, FRIEND_STATUS, ADD_FRIEND, etc.
│   ├── user_handlers.h/cpp    # USER_INFO, VIEW_HISTORY, CHANGE_PASS
│   ├── admin_handlers.h/cpp   # ADD_QUES, CHANGE_QUES, VIEW_QUES, DEL_QUES, BAN_USER
│   └── connection_handlers.h/cpp # PING, CONNECTION
├── client_handler.h/cpp        # Client connection handler (per-thread)
├── json_utils.h/cpp          # JSON parsing utilities
├── game_state_manager.h/cpp   # Game state management (placeholder for game logic)
├── server.cpp                # Main entry point
└── PROTOCOL_IMPLEMENTATION_STATUS.md # Implementation status tracking
```

### Module Responsibilities

- **ServerCore**: Manages socket creation, binding, listening, and connection acceptance
- **SessionManager**: Tracks all active client sessions and online users
- **AuthManager**: Handles token generation, validation, and password checking
- **RequestRouter**: Routes incoming requests to appropriate handlers based on requestType
- **ClientHandler**: Processes individual client connections in separate threads
- **RequestHandlers**: Process specific request types (organized by category)
- **GameStateManager**: Manages game state operations (to be integrated with game logic)

### Implementation Status

✅ **All protocol request types implemented** (27 request types)
✅ **All error codes from ERROR_CODES.md implemented** (with placeholders for database checks)
✅ **All validations according to PROTOCOL.md implemented**

See `PROTOCOL_IMPLEMENTATION_STATUS.md` for detailed status of each request type and error code.

## Database Integration

### Prerequisites

1. **Install PostgreSQL Client Library:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libpq-dev
   
   # macOS
   brew install postgresql
   ```

2. **Update Makefile:**
   Add PostgreSQL library to compilation flags:
   ```makefile
   CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g -I/usr/include/postgresql
   LDFLAGS = -pthread -lpq
   ```

### Database Module Structure

Create `database.h` and `database.cpp` with the following interface:

```cpp
class Database {
public:
    static Database& getInstance();
    
    // Connection
    bool connect(const std::string& connection_string);
    bool isConnected() const;
    
    // User operations
    bool authenticateUser(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    std::string getUserRole(const std::string& username);
    bool changePassword(const std::string& username, const std::string& old_password, const std::string& new_password);
    bool banUser(const std::string& username, const std::string& reason);
    bool userExists(const std::string& username);
    
    // Game operations
    int createGameSession(const std::string& username);
    bool updateGameSession(const GameSession& session);
    GameSession getActiveGameSession(const std::string& username);
    bool saveGameProgress(const std::string& username, int game_id, int question_number, int prize, int score);
    GameSession loadGameProgress(const std::string& username);
    bool endGame(int game_id, const std::string& status, int total_score, int final_prize);
    
    // Leaderboard
    std::vector<LeaderboardEntry> getLeaderboard(const std::string& type, int page, int limit);
    
    // Friends
    std::vector<std::string> getFriendsList(const std::string& username);
    bool addFriendRequest(const std::string& from_user, const std::string& to_user);
    bool acceptFriendRequest(const std::string& from_user, const std::string& to_user);
    bool declineFriendRequest(const std::string& from_user, const std::string& to_user);
    bool deleteFriend(const std::string& user1, const std::string& user2);
    std::vector<FriendRequest> getFriendRequests(const std::string& username);
    
    // Game history
    std::vector<GameSession> getGameHistory(const std::string& username, int limit = 20);
    
    // Admin operations
    int addQuestion(const Question& question);
    bool updateQuestion(int question_id, const Question& question);
    bool deleteQuestion(int question_id);
    Question getQuestion(int question_id);
    std::vector<Question> getQuestions(int level, int page, int limit);
    bool questionExists(int question_id);
};
```

### Configuration

Add database connection settings to `config.h`:

```cpp
struct ServerConfig {
    // ... existing fields ...
    std::string db_host;
    int db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
};
```

Update `config.json.example`:

```json
{
  "port": 8080,
  "log_file": "server.log",
  "log_level": "INFO",
  "max_clients": 100,
  "ping_timeout_seconds": 60,
  "connection_timeout_seconds": 300,
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "game_user",
  "db_password": "password"
}
```

### Integration Steps

1. **Initialize Database Connection:**
   In `GameServer::start()`, add:
   ```cpp
   string conn_string = "host=" + config_.db_host + 
                       " port=" + to_string(config_.db_port) +
                       " dbname=" + config_.db_name +
                       " user=" + config_.db_user +
                       " password=" + config_.db_password;
   
   if (!Database::getInstance().connect(conn_string)) {
       LOG_ERROR("Failed to connect to database");
       return false;
   }
   ```

2. **Replace Stub Functions:**
   Find all `TODO: Replace with database call` comments in `server.cpp` and replace with actual database calls.

   Example:
   ```cpp
   // Before:
   bool login_success = authenticateUser(username, password);
   
   // After:
   bool login_success = Database::getInstance().authenticateUser(username, password);
   User user = Database::getInstance().getUser(username);
   if (user.is_banned) {
       return StreamUtils::createErrorResponse(403, "Account is banned");
   }
   string user_role = Database::getInstance().getUserRole(username);
   ```

3. **Update Authentication:**
   - `handleLogin()`: Use `Database::getInstance().authenticateUser()`
   - `handleRegister()`: Use `Database::getInstance().registerUser()`
   - `isAdmin()`: Use `Database::getInstance().getUserRole()`

4. **Update Game Handlers:**
   - `handleStart()`: Use `Database::getInstance().createGameSession()`
   - `handleAnswer()`: Use `Database::getInstance().updateGameSession()`
   - `handleResume()`: Use `Database::getInstance().loadGameProgress()`
   - `handleGiveUp()`: Use `Database::getInstance().endGame()`

## Game Logic Integration

### Required Modules

1. **QuestionManager** (`game_question.h/cpp`)
   - `Question getRandomQuestion(int level)`
   - `bool checkAnswer(int question_id, int answer_index)`
   - `int getCorrectAnswer(int question_id)`

2. **GameStateManager** (`game_state.h/cpp`)
   - `GameSession startNewGame(const std::string& username, bool override_saved)`
   - `AnswerResult processAnswer(int game_id, int question_number, int answer_index, int time_remaining)`
   - `LifelineResult useLifeline(int game_id, const std::string& lifeline_type)`
   - `GameEndResult giveUp(int game_id)`
   - `void autoSaveGame(int game_id)`

3. **ScoringSystem** (`scoring.h/cpp`)
   - `int calculateQuestionScore(int time_remaining, int lifelines_used)`
   - `int calculateTotalScore(const std::vector<int>& question_scores)`
   - `int getPrizeForLevel(int level)`
   - `int getSafeCheckpointPrize(int question_number)`

4. **LifelineManager** (`lifeline.h/cpp`)
   - `LifelineResult use5050(int game_id, int question_id)`
   - `LifelineResult usePhone(int game_id, int question_id)`
   - `LifelineResult useAudience(int game_id, int question_id)`
   - `bool isLifelineUsed(int game_id, const std::string& lifeline_type)`

5. **GameTimer** (`game_timer.h/cpp`)
   - `void startQuestionTimer(int game_id)`
   - `bool isTimeout(int game_id)`
   - `int getRemainingTime(int game_id)`
   - `void stopTimer(int game_id)`

### Integration Steps

1. **Include Headers:**
   Add to `server.cpp`:
   ```cpp
   #include "game_question.h"
   #include "game_state.h"
   #include "scoring.h"
   #include "lifeline.h"
   #include "game_timer.h"
   ```

2. **Update START Handler:**
   ```cpp
   // Generate game ID and create session
   int game_id = Database::getInstance().createGameSession(username);
   
   // Get random question for level 1
   Question question = QuestionManager::getInstance().getRandomQuestion(1);
   
   // Start timer
   GameTimer::getInstance().startQuestionTimer(game_id);
   
   // Send notifications
   sendGameStartNotification(session.handler.get(), game_id);
   sendQuestionInfoNotification(session.handler.get(), game_id, question);
   ```

3. **Update ANSWER Handler:**
   ```cpp
   // Check timeout
   if (GameTimer::getInstance().isTimeout(game_id)) {
       return handleQuestionTimeout(session);
   }
   
   // Process answer
   int time_remaining = GameTimer::getInstance().getRemainingTime(game_id);
   AnswerResult result = GameStateManager::getInstance().processAnswer(
       game_id, question_number, answer_index, time_remaining);
   
   // Update database
   Database::getInstance().updateGameSession(result.game_session);
   ```

4. **Update LIFELINE Handler:**
   ```cpp
   LifelineResult result = LifelineManager::getInstance().useLifeline(
       game_id, lifeline_type);
   
   // Apply delay
   this_thread::sleep_for(chrono::seconds(result.delay_seconds));
   
   // Send notification
   sendLifelineInfoNotification(session.handler.get(), result);
   ```

5. **Add Notification Functions:**
   Create helper functions to send notifications:
   ```cpp
   void sendGameStartNotification(StreamHandler* handler, int game_id);
   void sendQuestionInfoNotification(StreamHandler* handler, int game_id, const Question& question);
   void sendLifelineInfoNotification(StreamHandler* handler, const LifelineResult& result);
   void sendGameEndNotification(StreamHandler* handler, const GameEndResult& result);
   ```

## Testing

1. **Unit Tests:**
   Test each handler independently with mock database/game logic.

2. **Integration Tests:**
   Test full request/response flow with real database.

3. **End-to-End Tests:**
   Test complete game flow from START to GAME_END.

## Notes

- All database calls should be wrapped in try-catch blocks for error handling.
- Use proper error codes from ERROR_CODES.md.
- Log all database operations for debugging.
- Ensure thread-safety when accessing shared resources.

