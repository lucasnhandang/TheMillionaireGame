# Database Integration - Complete

## ✅ All Issues Fixed

### 1. **database.h** - Made `getLastError()` public
- Moved `getLastError()` from private to public section
- Now accessible from `server_core.cpp` for error reporting

### 2. **server/Makefile** - Added database module compilation
- Added `-I/usr/include/postgresql -I../database` to `CXXFLAGS`
- Added `-lpq` to `LDFLAGS` for PostgreSQL library
- Added `DATABASE_SRC` and `DATABASE_OBJ` variables
- Added compilation rule for `database.o` from `../database/database.cpp`
- Added `DATABASE_OBJ` to server executable dependencies

### 3. **server/server_core.cpp** - Database connection initialization
- Added `#include "../database/database.h"`
- Added `#include <thread>` and `#include <chrono>` (for existing code)
- Initialize database connection in `start()` method
- Disconnect database in `stop()` method
- Proper error handling with `getLastError()`

### 4. **server/request_handlers/auth_handlers.cpp** - Database integration
- Added `#include "../../database/database.h"`
- `handleLogin()`: Uses `Database::authenticateUser()`, `isUserBanned()`, `getUserRole()`
- `handleRegister()`: Uses `userExists()` and `registerUser()`

### 5. **server/auth_manager.cpp** - Database integration
- Added `#include "../database/database.h"`
- `isAdmin()`: Uses `Database::getUserRole()` to check admin status

### 6. **server/request_handlers/social_handlers.cpp** - Database integration
- Added `#include "../../database/database.h"` and `<sstream>`
- `handleLeaderboard()`: Uses `Database::getLeaderboard()` with proper JSON building
- `handleFriendStatus()`: Uses `Database::getFriendsList()` and `SessionManager::isUserOnline()`
- `handleAddFriend()`: Uses `userExists()`, `friendshipExists()`, `addFriendRequest()`
- `handleAcceptFriend()`: Uses `acceptFriendRequest()`
- `handleDeclineFriend()`: Uses `declineFriendRequest()`
- `handleFriendReqList()`: Uses `getFriendRequests()` with JSON building
- `handleDelFriend()`: Uses `friendshipExists()` and `deleteFriend()`
- `handleChat()`: Uses `userExists()` and `sendMessage()`

### 7. **server/request_handlers/user_handlers.cpp** - Database integration
- Added `#include "../../database/database.h"` and `<sstream>`
- `handleUserInfo()`: Uses `userExists()` and `getLeaderboard()` for user stats
- `handleViewHistory()`: Uses `getGameHistory()` with proper JSON building
- `handleChangePass()`: Uses `changePassword()`

### 8. **server/request_handlers/admin_handlers.cpp** - Database integration
- Added `#include "../../database/database.h"`, `<sstream>`, `<algorithm>`
- Added `extractOptions()` helper function to parse JSON options array
- `handleAddQues()`: Uses `addQuestion()` with options parsing
- `handleChangeQues()`: Uses `questionExists()`, `getQuestion()`, `updateQuestion()`
- `handleViewQues()`: Uses `getQuestions()` with proper JSON building
- `handleDelQues()`: Uses `questionExists()` and `deleteQuestion()`
- `handleBanUser()`: Uses `userExists()` and `banUser()`
- Updated level validation to 0-2 (was 1-15)

### 9. **database/schema.sql** - Updated level constraint
- Changed `level` constraint from `CHECK (level >= 1 AND level <= 15)` to `CHECK (level >= 0 AND level <= 2)`
- Fixed index definitions for `messages` table (changed `receiver_id`/`sender_id` to `to_user_id`/`from_user_id`)

### 10. **server/server.cpp** - Fixed config loading bug
- **Critical Bug Fix**: Config file was never loaded due to incorrect condition `if (config.port == 0)`
- Since default port is 8080, the condition was always false
- Fixed to always load config file first, then apply command line overrides
- Now `config.json` is properly loaded and used

### 11. **Compilation Fixes**
- Fixed `make_unique` usage (C++14) → replaced with `unique_ptr<T>(new T(...))` for C++11 compatibility
  - Fixed in: `client_handler.cpp`, `stream_handler.cpp`, `logger.cpp`
- Fixed `std::unordered_map::operator[]` issue → changed to `emplace()` for `ClientSession` construction
- Fixed incomplete type errors → added `#include "session_manager.h"` to `auth_manager.cpp`
- Fixed PostgreSQL include paths for macOS Homebrew (versioned directories)
- Fixed PostgreSQL library paths for macOS (`-L/opt/homebrew/lib/postgresql@14`)
- Added explicit compilation rules for `request_handlers/` subdirectory files in Makefile

### 12. **Database Setup Improvements**
- Added PostgreSQL user creation steps for team consistency
- Added database reset/cleanup steps for starting fresh
- Fixed index creation errors in schema.sql

### 13. **Game Logic Modules Implementation**
- **QuestionManager** (`question_manager.h/cpp`): 
  - `getRandomQuestion()`: Gets random question from database for a level
  - `checkAnswer()`: Validates answer against question in database
  - `getCorrectAnswer()`: Returns correct answer index
- **ScoringSystem** (`scoring_system.h/cpp`):
  - `calculateQuestionScore()`: Calculates points based on time remaining and lifelines used
  - `calculateTotalScore()`: Sums up question scores
  - `getPrizeForLevel()`: Returns prize amount for question number (doubles each question)
  - `getSafeCheckpointPrize()`: Returns safe checkpoint prize (at questions 5, 10, 15)
- **LifelineManager** (`lifeline_manager.h/cpp`):
  - `use5050()`: Removes 2 incorrect answers, returns remaining options
  - `usePhone()`: Simulates friend suggestion (70% correct)
  - `useAudience()`: Generates audience poll percentages (correct answer gets 40-60%)
  - `isLifelineUsed()`: Checks if lifeline was already used
- **GameTimer** (`game_timer.h/cpp`):
  - `startQuestionTimer()`: Starts 15-second timer for question
  - `isTimeout()`: Checks if question timed out
  - `getRemainingTime()`: Returns remaining seconds
  - `stopTimer()`: Stops timer when question answered or game ends

### 14. **Game Handlers Integration**
- **handleStart()**: Uses QuestionManager to get first question, starts GameTimer
- **handleAnswer()**: Uses QuestionManager to check answer, ScoringSystem for points/prizes, GameTimer for timeout
- **handleLifeline()**: Uses LifelineManager to process lifeline requests
- **handleGiveUp()**: Uses ScoringSystem for final prize calculation
- Level progression: Questions 1-5 = level 0 (easy), 6-10 = level 1 (medium), 11-15 = level 2 (hard)

---

## 📋 Next Steps

### 1. Start PostgreSQL Server
```bash
# macOS
brew services start postgresql
# or
pg_ctl -D /usr/local/var/postgres start

# Linux
sudo systemctl start postgresql
# or
sudo service postgresql start
```

### 2. Create Database and Load Schema

**If starting fresh or resetting the database:**

```bash
cd "/path/to/your/repo"

# Terminate any active connections to the database
psql -U postgres -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"

# Drop existing database (if it exists)
psql -U postgres -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"

# Create fresh database
createdb -U postgres millionaire_game

# Load schema
psql -U postgres -d millionaire_game < database/schema.sql

# Load mock data (optional)
psql -U postgres -d millionaire_game < database/mock_data.sql
```

### 3. Create PostgreSQL User (For Team Consistency)
To ensure all teammates can use the same configuration, create a `postgres` user:

```bash
# Connect to PostgreSQL (using your default user)
psql -d postgres

# Create the postgres user
CREATE USER postgres WITH PASSWORD 'password' SUPERUSER CREATEDB CREATEROLE;

# Grant access to the database
GRANT ALL PRIVILEGES ON DATABASE millionaire_game TO postgres;

# Exit
\q
```

**Note:** If you're on macOS and get "role does not exist" error, use your macOS username instead:
```bash
psql -U $(whoami) -d postgres
```

### 4. Update config.json
Create `server/config.json` from `server/config.json.example` and use the standard database credentials:
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
  "db_user": "postgres",
  "db_password": "password"
}
```

### 5. Build Server
```bash
cd server
make clean
make
```

### 6. Test Database Connection
```bash
lsof -ti :8080 | xargs kill -9
./bin/server
# Should see: "Database connected successfully" in logs
```

---

## 🔧 Build Requirements

- **PostgreSQL**: Installed and running
- **libpq-dev** (Linux) or **postgresql** (macOS): PostgreSQL client library
- **C++11 compiler**: g++ or clang++
- **pthread**: For threading support

---

## ⚠️ Known Limitations

1. **Password Hashing**: Currently uses simple hash function. **Replace with bcrypt before production**.

2. **Options Parsing**: The `extractOptions()` function in `admin_handlers.cpp` is a simple parser. For complex JSON, consider using a proper JSON library.

3. **Game Logic**: ✅ **COMPLETED** - All game logic modules implemented and integrated:
   - QuestionManager: Question retrieval and answer validation
   - ScoringSystem: Score calculation and prize determination
   - LifelineManager: All three lifelines (50/50, Phone, Audience)
   - GameTimer: Question timeout tracking

4. **Level Validation**: Updated to 0-2 (easy/medium/hard). Level is determined by question number:
   - Questions 1-5: Level 0 (easy)
   - Questions 6-10: Level 1 (medium)
   - Questions 11-15: Level 2 (hard)

---

## ✅ Integration Status

- ✅ Database schema created
- ✅ Database module implemented
- ✅ Makefile updated
- ✅ Server initialization updated
- ✅ All handler TODOs replaced with database calls
- ✅ All includes added
- ✅ Error handling implemented
- ✅ Config loading bug fixed
- ✅ All compilation errors resolved
- ✅ PostgreSQL user setup documented
- ✅ Database reset procedures documented
- ✅ **Game logic modules implemented** (QuestionManager, ScoringSystem, LifelineManager, GameTimer)
- ✅ **Game handlers integrated** with game logic modules

**Ready for testing!**

---

## 🐛 Bugs Fixed During Integration

### Config Loading Bug (Critical)
- **Issue**: Config file (`config.json`) was never loaded, server always used defaults
- **Root Cause**: Logic checked `if (config.port == 0)` but default is 8080
- **Fix**: Always load config file first, then apply command line overrides
- **Files Changed**: `server/server.cpp`

### Compilation Issues
- **C++11 Compatibility**: Replaced `make_unique` (C++14) with `unique_ptr` constructor
- **Include Paths**: Fixed PostgreSQL headers for macOS Homebrew versioned directories
- **Library Paths**: Added versioned PostgreSQL library directory for macOS
- **Makefile**: Added explicit rules for subdirectory source files

### Database Setup
- **User Creation**: Added steps to create `postgres` user for team consistency
- **Schema Errors**: Fixed index definitions to match actual table column names
- **Reset Procedure**: Added commands to clean/reset database for fresh start

---

## 🖥️ Server Management

### How to Restart the Server

#### Step 1: Stop the Current Server

```bash
# Option A: If running in terminal, press Ctrl+C

# Option B: Kill by PID
kill $(lsof -ti :8080)

# Option C: Kill by port (force)
lsof -ti :8080 | xargs kill -9
```

#### Step 2: Rebuild (if code changed)

```bash
cd server
make clean
make
```

#### Step 3: Start Server

```bash
cd server
./bin/server
```

**Or with custom config:**
```bash
./bin/server -c config.json
```

**Or with port override:**
```bash
./bin/server -p 8080
```