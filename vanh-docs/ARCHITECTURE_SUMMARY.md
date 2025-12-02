# Server Architecture Summary

## Overview

The Millionaire Game server is a **multi-threaded TCP server** built in C++11 that handles client connections, authentication, game logic, and social features. The architecture follows a **modular, handler-based design** with clear separation of concerns.

---

## Core Architecture

### 1. **Server Lifecycle Management**

**`ServerCore`** (`server_core.h/cpp`)
- Manages server socket creation, binding, and listening
- Accepts incoming TCP connections
- Spawns a new thread for each client connection
- Handles graceful shutdown (SIGINT/SIGTERM)
- Enforces maximum client limits

**Flow:**
```
server.cpp (main) 
  → ServerCore::start() [creates socket, binds, listens]
  → ServerCore::run() [accepts connections in loop]
  → ClientHandler::handleClient() [per-thread client processing]
```

---

### 2. **Client Connection Handling**

**`ClientHandler`** (`client_handler.h/cpp`)
- Each client connection runs in its own thread
- Uses `StreamHandler` for TCP stream I/O
- Main loop: read request → validate JSON → route → send response
- Sends CONNECTION notification on connect
- Handles disconnection cleanup

**`StreamHandler`** (`stream_handler.h/cpp`)
- Manages TCP socket communication
- Handles newline-delimited JSON messages
- Buffer management for partial messages
- Timeout handling for reads/writes
- Thread-safe message extraction

---

### 3. **Session Management**

**`SessionManager`** (`session_manager.h/cpp`) - **Singleton**
- Tracks all active client sessions
- Stores per-client state:
  - `auth_token`, `username`, `role` (user/admin)
  - `in_game`, `game_id`, `current_question_number`
  - `current_prize`, `total_score`, `used_lifelines`
  - Connection metadata (IP, timestamps)
- Manages online user list
- Thread-safe with mutex protection

**Key Data Structure:**
```cpp
struct ClientSession {
    unique_ptr<StreamHandler> handler;
    string client_ip;
    string auth_token;
    string username;
    string role;  // "user" or "admin"
    bool authenticated;
    bool in_game;
    int game_id;
    int current_question_number;
    int total_score;
    set<string> used_lifelines;
    // ... timestamps, etc.
};
```

---

### 4. **Authentication & Authorization**

**`AuthManager`** (`auth_manager.h/cpp`) - **Singleton**
- Generates 32-character hex authentication tokens
- Maps tokens to client file descriptors
- Validates tokens for authenticated requests
- Password strength validation (8+ chars, uppercase, lowercase, digit)
- Admin role checking (currently placeholder - needs database)

**Token Flow:**
1. Client sends LOGIN/REGISTER
2. Server validates credentials (placeholder - needs database)
3. Server generates token via `AuthManager::generateToken()`
4. Token registered: `token → client_fd` and `username → token`
5. All subsequent requests include `authToken` in data field
6. Server validates token before processing request

---

### 5. **Request Routing**

**`RequestRouter`** (`request_router.h/cpp`)
- Central dispatcher for all incoming requests
- Extracts `requestType` from JSON
- Routes to appropriate handler based on type
- Handles authentication check (except LOGIN, REGISTER, CONNECTION)

**Request Categories:**
- **Authentication**: LOGIN, REGISTER, LOGOUT
- **Game Actions**: START, ANSWER, LIFELINE, GIVE_UP, RESUME, LEAVE_GAME
- **Social**: LEADERBOARD, FRIEND_STATUS, ADD_FRIEND, ACCEPT_FRIEND, etc.
- **User Info**: USER_INFO, VIEW_HISTORY, CHANGE_PASS
- **Admin**: ADD_QUES, CHANGE_QUES, VIEW_QUES, DEL_QUES, BAN_USER
- **Connection**: PING, CONNECTION

---

### 6. **Request Handlers** (`request_handlers/`)

Organized by functional category:

**`auth_handlers.h/cpp`**
- `handleLogin()` - Authenticate user, return token
- `handleRegister()` - Create new account
- `handleLogout()` - End session

**`game_handlers.h/cpp`**
- `handleStart()` - Start new game session
- `handleAnswer()` - Process answer submission
- `handleLifeline()` - Process lifeline usage (50/50, PHONE, AUDIENCE)
- `handleGiveUp()` - Player quits game
- `handleResume()` - Resume auto-saved game
- `handleLeaveGame()` - Leave game (auto-save)

**`social_handlers.h/cpp`**
- Friend management (ADD_FRIEND, ACCEPT_FRIEND, DEL_FRIEND, etc.)
- Leaderboard queries (global/friend)
- Friend status checking
- Chat messaging

**`user_handlers.h/cpp`**
- User profile info
- Game history viewing
- Password changes

**`admin_handlers.h/cpp`**
- Question CRUD operations (ADD_QUES, CHANGE_QUES, VIEW_QUES, DEL_QUES)
- User banning (BAN_USER)
- Admin role validation

**`connection_handlers.h/cpp`**
- CONNECTION notification
- PING/PONG for keepalive

---

### 7. **Supporting Modules**

**`JsonUtils`** (`json_utils.h/cpp`)
- Simple JSON parsing utilities
- Extract string/int/bool values from JSON
- Pattern-based extraction (not full JSON parser)

**`Logger`** (`logger.h/cpp`) - **Singleton**
- Thread-safe logging system
- Log levels: DEBUG, INFO, WARNING, ERROR
- Outputs to file and/or stdout
- Macros: `LOG_INFO()`, `LOG_ERROR()`, etc.

**`Config`** (`config.h/cpp`)
- Loads server configuration from JSON file
- Settings: port, log_file, max_clients, timeouts
- Command-line argument override support

**`GameStateManager`** (`game_state_manager.h/cpp`) - **Singleton**
- Currently placeholder implementation
- Generates unique game IDs
- TODO: Replace with database calls for game state persistence

---

## Communication Protocol

### Message Format
- **Protocol**: TCP sockets with newline (`\n`) delimiter
- **Format**: JSON messages
- **Request**: `{"requestType": "TYPE", "data": {...}}`
- **Response**: `{"responseCode": 200, "data": {...}}` or `{"responseCode": 401, "message": "..."}`

### Authentication Flow
1. Client connects → Server sends CONNECTION notification
2. Client sends LOGIN/REGISTER → Server validates → Returns `authToken`
3. All subsequent requests include `authToken` in `data` field
4. Server validates token before processing

### Error Handling
- All errors follow `ERROR_CODES.md` specification
- HTTP-like response codes (200, 400, 401, 402, etc.)
- Error responses: `{"responseCode": CODE, "message": "..."}`

---

## Current Implementation Status

### ✅ **Completed**
- **All 27 request types implemented** (routing and handlers)
- **All error codes from ERROR_CODES.md implemented**
- **All validations according to PROTOCOL.md**
- **Thread-safe session management**
- **Authentication token system**
- **Request routing and handler architecture**
- **JSON message parsing utilities**
- **Logging system**
- **Configuration management**

### ⚠️ **Placeholders (Need Database Integration)**
- **User authentication** - Currently stub functions
- **User registration** - Password validation works, but no DB storage
- **Admin role checking** - `isAdmin()` returns hardcoded values
- **Game state persistence** - Game state stored in memory only
- **Question retrieval** - No actual question database
- **Answer validation** - No question database to check against
- **Leaderboard queries** - Returns empty/mock data
- **Friend operations** - No database persistence
- **Game history** - No database storage
- **Admin question management** - No database persistence

**Key TODO Pattern:**
```cpp
// TODO: Replace with database call
bool authenticateUser(const string& username, const string& password) {
    // Stub implementation
    return true;  // Always succeeds
}
```

---

## Database Integration Points

Based on `server/docs/INTEGRATION.md`, the following need database integration:

### 1. **Authentication Handlers**
- `AuthHandlers::handleLogin()` → `Database::authenticateUser()`
- `AuthHandlers::handleRegister()` → `Database::registerUser()`
- `AuthManager::isAdmin()` → `Database::getUserRole()`

### 2. **Game Handlers**
- `GameHandlers::handleStart()` → `Database::createGameSession()`
- `GameHandlers::handleAnswer()` → `Database::updateGameSession()`
- `GameHandlers::handleResume()` → `Database::loadGameProgress()`
- `GameHandlers::handleGiveUp()` → `Database::endGame()`
- Question retrieval → `Database::getQuestion()` or `QuestionManager`

### 3. **Social Handlers**
- Friend operations → `Database::addFriendRequest()`, `Database::getFriendsList()`, etc.
- Leaderboard → `Database::getLeaderboard()`

### 4. **User Handlers**
- `UserHandlers::handleViewHistory()` → `Database::getGameHistory()`
- `UserHandlers::handleChangePass()` → `Database::changePassword()`

### 5. **Admin Handlers**
- Question CRUD → `Database::addQuestion()`, `Database::updateQuestion()`, etc.
- `AdminHandlers::handleBanUser()` → `Database::banUser()`

---

## Thread Safety

- **SessionManager**: Mutex-protected (`clients_mutex_`)
- **AuthManager**: Mutex-protected (`tokens_mutex_`)
- **Logger**: Mutex-protected (`log_mutex_`)
- **StreamHandler**: Per-client instance (no shared state)
- **RequestRouter**: Stateless (no shared state)

---

## Configuration

**`config.json`** structure:
```json
{
  "port": 8080,
  "log_file": "server.log",
  "log_level": "INFO",
  "max_clients": 100,
  "ping_timeout_seconds": 60,
  "connection_timeout_seconds": 300
}
```

**Database config** (to be added):
```json
{
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "game_user",
  "db_password": "password"
}
```

---

## Build System

**Makefile** (`server/Makefile`)
- Compiles all source files to `obj/` directory
- Links into `bin/server` executable
- Includes all handler modules
- Uses C++11 standard, pthread for threading
- **Note**: Database library (`-lpq`) not yet added

---

## Key Design Patterns

1. **Singleton Pattern**: `SessionManager`, `AuthManager`, `Logger`, `GameStateManager`
2. **Handler Pattern**: Request handlers organized by category
3. **Router Pattern**: Central dispatcher routes requests to handlers
4. **Thread-per-Connection**: Each client gets its own thread
5. **RAII**: Smart pointers for resource management

---

## Next Steps for Database Integration

1. **Create Database Module** (`database.h/cpp`)
   - Implement singleton Database class
   - Add PostgreSQL connection using `libpq`
   - Implement all methods specified in `server/docs/INTEGRATION.md`

2. **Update Configuration**
   - Add database connection fields to `config.h`
   - Update `config.json.example`

3. **Update Makefile**
   - Add `-lpq` to LDFLAGS
   - Add `database.cpp` to build

4. **Replace Placeholders**
   - Search for `TODO: Replace with database call`
   - Replace stub functions with `Database::getInstance().method()`

5. **Initialize Database**
   - Call `Database::getInstance().connect()` in `ServerCore::start()`

6. **Test Integration**
   - Test authentication flow
   - Test game session creation
   - Test all handlers with database

---

## Summary

The server architecture is **well-structured and modular**, with clear separation between:
- **Network layer** (ServerCore, StreamHandler, ClientHandler)
- **Session management** (SessionManager)
- **Authentication** (AuthManager)
- **Request processing** (RequestRouter + Handlers)
- **Business logic** (Game handlers, Social handlers, etc.)

The **main gap** is database integration - all handlers are implemented but use placeholder functions. Once the database module is integrated, the server will be fully functional.

