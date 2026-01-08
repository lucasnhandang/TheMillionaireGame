# Who Wants to Be a Millionaire - Technical Documentation
## Complete Implementation Guide & Source of Truth

---

## Table of Contents
1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Technology Stack](#3-technology-stack)
4. [Communication Protocol](#4-communication-protocol)
5. [Directory Structure](#5-directory-structure)
6. [Core Components Deep Dive](#6-core-components-deep-dive)
7. [Database Schema](#7-database-schema)
8. [Request Flow](#8-request-flow)
9. [Where to Find & Modify Code](#9-where-to-find--modify-code)
10. [Development Workflow](#10-development-workflow)
11. [Testing Strategy](#11-testing-strategy)
12. [Common Scenarios](#12-common-scenarios)

---

## 1. Project Overview

### 1.1 What is This Project?
A multiplayer **"Who Wants to Be a Millionaire"** game implementation using C++ with:
- **TCP Socket Communication** (NOT WebSocket - using WebSocket will result in minus points)
- **Client-Server Architecture** with persistent state
- **PostgreSQL Database** for data persistence
- **ImGui-based GUI** for the client interface
- **27+ Protocol Request Types** covering authentication, game logic, social features, and admin operations

### 1.2 Key Features
✅ **Authentication System**: User registration, login, password management, ban functionality  
✅ **Game Mechanics**: 15-question game with progressive difficulty, 3 lifelines (50/50, Phone, Audience)  
✅ **Scoring System**: Time-based scoring (max 30 points per question), lifeline penalties (-5 points)  
✅ **Safe Checkpoints**: Questions 5, 10, 15 guarantee minimum prize on failure  
✅ **Auto-Save/Resume**: Game state persists across disconnections  
✅ **Social Features**: Friend system, chat, leaderboards (global & friends)  
✅ **Admin Panel**: Question management (CRUD operations), user banning  
✅ **Real-time Notifications**: Server pushes game events to clients  

### 1.3 Current Status (All Tests Passed ✅)
- ✅ Database integration complete and tested
- ✅ All 27 protocol request types implemented
- ✅ Authentication, game logic, social features, admin operations working
- ✅ Error code compliance (400-422, 500-502)
- ✅ Client GUI with ImGui + GLFW + OpenGL
- ✅ TCP socket communication with JSON protocol

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT LAYER                             │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  ImGui GUI (main.cpp)                                     │  │
│  │  - Render game UI, menus, leaderboards                   │  │
│  │  - Handle user input                                      │  │
│  │  - Display question, lifelines, timer                     │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Protocol Handler (protocol_handler.cpp)                  │  │
│  │  - Serialize/deserialize JSON messages                    │  │
│  │  - Build requests, parse responses                        │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Socket Client (socket_client.cpp)                        │  │
│  │  - TCP socket connection                                  │  │
│  │  - Send/receive messages (newline-delimited JSON)         │  │
│  │  - Background receive thread                              │  │
│  └────────────────┬─────────────────────────────────────────┘  │
└───────────────────┼──────────────────────────────────────────────┘
                    │ TCP Socket (localhost:8080)
                    │ JSON over newline-delimited stream
┌───────────────────▼──────────────────────────────────────────────┐
│                         SERVER LAYER                              │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Server Core (server_core.cpp)                            │  │
│  │  - TCP server socket (epoll/select for multiple clients)  │  │
│  │  - Accept connections                                     │  │
│  │  - Create ClientHandler threads                           │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Client Handler (client_handler.cpp)                      │  │
│  │  - Per-client thread                                      │  │
│  │  - Stream handler for newline-delimited JSON              │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Request Router (request_router.cpp)                      │  │
│  │  - Route requestType to appropriate handler               │  │
│  │  - Validate authentication (except LOGIN/REGISTER)        │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Request Handlers (request_handlers/*.cpp)                │  │
│  │  - auth_handlers: LOGIN, REGISTER, LOGOUT, CHANGE_PASS   │  │
│  │  - game_handlers: START, ANSWER, LIFELINE, GIVE_UP, etc. │  │
│  │  - social_handlers: Friends, Chat, Leaderboard           │  │
│  │  - user_handlers: USER_INFO, VIEW_HISTORY                │  │
│  │  - admin_handlers: Question CRUD, BAN_USER               │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  Business Logic Modules                                   │  │
│  │  - SessionManager: Track client sessions                  │  │
│  │  - AuthManager: Token generation/validation               │  │
│  │  - GameStateManager: Game ID generation                   │  │
│  │  - QuestionManager: Random question selection             │  │
│  │  - ScoringSystem: Point calculation                       │  │
│  │  - LifelineManager: Lifeline logic (50/50, Phone, Ask)    │  │
│  │  - GameTimer: 30-second countdown per question            │  │
│  └────────────────┬─────────────────────────────────────────┘  │
└───────────────────┼──────────────────────────────────────────────┘
                    │
┌───────────────────▼──────────────────────────────────────────────┐
│                       DATABASE LAYER                              │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Database Module (database/database.cpp)                  │  │
│  │  - PostgreSQL connection (libpq)                          │  │
│  │  - CRUD operations for all tables                         │  │
│  │  - Password hashing (bcrypt-style)                        │  │
│  │  - Transaction management                                 │  │
│  └────────────────┬─────────────────────────────────────────┘  │
│                   │                                              │
│  ┌────────────────▼─────────────────────────────────────────┐  │
│  │  PostgreSQL Database (millionaire_game)                   │  │
│  │  Tables: users, questions, game_sessions, game_questions, │  │
│  │          game_answers, saved_games, friendships,          │  │
│  │          friend_requests, messages, leaderboard           │  │
│  └──────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘
```

### 2.2 Communication Model
- **Protocol**: JSON over TCP sockets
- **Message Delimiter**: Newline character (`\n`)
- **Request Format**: `{"requestType": "TYPE", "data": {...}}\n`
- **Response Format**: `{"responseCode": 200, "data": {...}}\n` or `{"responseCode": 401, "message": "Error"}\n`
- **Authentication**: Token-based (32-char hex token) after successful LOGIN
- **Threading**: Server uses one thread per client connection

### 2.3 Data Flow Example (START Game)
```
1. Client (GUI):          User clicks "Start Game"
2. Client (Protocol):     Build JSON: {"requestType":"START","data":{"authToken":"..."}}
3. Client (Socket):       Send to server over TCP (with \n delimiter)
4. Server (Client Handler): Receive complete JSON message
5. Server (Router):       Extract requestType="START", validate authToken
6. Server (Game Handler): handleStart() → Create game session in DB
7. Database:              INSERT INTO game_sessions (...) → returns gameId
8. Server (Game Handler): Assign 15 random questions to game
9. Database:              INSERT INTO game_questions (game_id, question_order, question_id)
10. Server (Notification): Send GAME_START notification
11. Server (Notification): Send QUESTION_INFO notification with first question
12. Client (Socket):      Receive GAME_START, then QUESTION_INFO
13. Client (Protocol):    Parse JSON responses
14. Client (GUI):         Display question, start 30-second timer
```

---

## 3. Technology Stack

### 3.1 Server Stack
| Component | Technology | Purpose |
|-----------|-----------|---------|
| Language | C++11 | Core server logic |
| Networking | POSIX TCP Sockets | Client-server communication |
| Threading | pthread | Per-client threads |
| Concurrency | epoll/select | Efficient I/O multiplexing |
| JSON | Manual parsing | Protocol message handling |
| Database | PostgreSQL (libpq) | Data persistence |
| Password | SHA-256 + salt | User authentication |
| Logging | Custom logger | Server logs to file |

### 3.2 Client Stack
| Component | Technology | Purpose |
|-----------|-----------|---------|
| Language | C++11 | Client logic |
| GUI | ImGui 1.89+ | User interface |
| Graphics | OpenGL 3+ | Rendering backend |
| Windowing | GLFW 3.3+ | Window management |
| Networking | TCP Sockets | Server communication |
| JSON | Manual parsing | Protocol messages |
| Images | stb_image.h | Load lifeline icons |

### 3.3 Database Stack
| Component | Technology | Purpose |
|-----------|-----------|---------|
| RDBMS | PostgreSQL 14+ | Data storage |
| Schema | 10 tables | Users, games, questions, social |
| Connection | libpq (C API) | Database connectivity |
| Indexing | B-tree indexes | Query optimization |

### 3.4 Build System
| Component | Technology | Purpose |
|-----------|-----------|---------|
| Build | GNU Make | Compilation orchestration |
| Compiler | g++/clang++ | C++ compilation |
| Platforms | macOS, Linux | Cross-platform support |
| Dependencies | pkg-config | Library detection |

---

## 4. Communication Protocol

### 4.1 Protocol Overview
- **Format**: JSON messages with newline delimiter
- **Transport**: TCP sockets (raw TCP, NOT WebSocket)
- **Authentication**: Token-based (authToken field in data)
- **Error Handling**: HTTP-like status codes (200, 400, 401, etc.)

### 4.2 Request Types (27 Total)

#### Authentication (No auth required)
- `LOGIN`: Authenticate user, get authToken
- `REGISTER`: Create new account
- `CONNECTION`: Initial handshake (auto-sent by server)

#### Game Actions (Auth required)
- `START`: Begin new game (returns gameId)
- `ANSWER`: Submit answer to current question
- `LIFELINE`: Use lifeline (5050, PHONE, AUDIENCE)
- `GIVE_UP`: Quit game, take current prize
- `RESUME`: Continue saved game after disconnect
- `LEAVE_GAME`: Exit game (auto-saves progress)

#### Social Features (Auth required)
- `LEADERBOARD`: Get rankings (global or friends)
- `FRIEND_STATUS`: Check online/offline/ingame status of friends
- `ADD_FRIEND`: Send friend request
- `ACCEPT_FRIEND`: Accept incoming request
- `DECLINE_FRIEND`: Reject incoming request
- `FRIEND_REQ_LIST`: List pending friend requests
- `DEL_FRIEND`: Remove friend
- `CHAT`: Send message to another user

#### User Info (Auth required)
- `USER_INFO`: Get profile of any user
- `VIEW_HISTORY`: Get last 20 games of logged-in user
- `CHANGE_PASS`: Update password (old + new required)

#### Admin Operations (Auth + role="admin" required)
- `ADD_QUES`: Create new question
- `CHANGE_QUES`: Update existing question
- `VIEW_QUES`: List questions (paginated)
- `DEL_QUES`: Soft-delete question (sets is_active=false)
- `BAN_USER`: Permanently ban user account

#### Connection Management (Auth required)
- `LOGOUT`: End session, disconnect
- `PING`: Keep-alive / health check

### 4.3 Response Codes (HTTP-like)

**Success (2xx)**
- `200 OK`: Request succeeded
- `201 CREATED`: User registered successfully

**Client Errors (4xx)**
- `400 INVALID_DATA`: Malformed JSON or missing fields
- `401 LOGIN_FAILED`: Wrong username/password
- `402 AUTH_ERROR`: Invalid/missing authToken
- `403 FORBIDDEN`: Insufficient permissions or banned
- `404 NOT_FOUND`: Resource doesn't exist
- `405 USER_ALREADY_IN_GAME`: Cannot START, already playing
- `406 USER_NOT_IN_GAME`: Cannot ANSWER/LIFELINE, no active game
- `407 LIFELINE_ALREADY_CHOSEN`: Lifeline already used
- `408 QUESTION_TIMEOUT`: 30 seconds elapsed (game ends as "lost")
- `409 CONFLICT`: Duplicate resource (username exists, etc.)
- `410 WEAK_PASSWORD`: Password doesn't meet requirements
- `412 INVALID_GAME_STATE`: Request not allowed in current state
- `415 UNKNOWN_REQUEST_TYPE`: Unsupported requestType
- `422 UNPROCESSABLE_DATA`: Valid JSON but invalid field values

**Server Errors (5xx)**
- `500 SERVER_ERROR`: Internal server error
- `501 DATABASE_ERROR`: Database connection/query failure
- `502 SERVICE_UNAVAILABLE`: Service temporarily down

### 4.4 Key Protocol Rules
1. **Game ID Validation**: All game requests (ANSWER, LIFELINE, GIVE_UP) must include correct `gameId` from GAME_START
2. **Question Number Matching**: Server validates `questionNumber` matches current game state
3. **Lifeline Penalty**: Using any lifeline deducts 5 points (max points becomes 25 instead of 30)
4. **Safe Checkpoints**: Questions 5, 10, 15 are checkpoints - player receives checkpoint prize on failure
5. **Auto-Save**: Server automatically saves game state on disconnect (can RESUME later)
6. **Game State Clearing**: Game state is cleared when game ends (won/lost/quit), not when player disconnects

---

## 5. Directory Structure

```
Network Programming/
├── server/                      # Server implementation (C++)
│   ├── bin/                     # Compiled server executable
│   │   └── server               # Main server binary
│   ├── obj/                     # Object files (.o)
│   ├── request_handlers/        # Protocol request handlers
│   │   ├── auth_handlers.cpp/.h         # LOGIN, REGISTER, LOGOUT, CHANGE_PASS
│   │   ├── game_handlers.cpp/.h         # START, ANSWER, LIFELINE, GIVE_UP, RESUME, LEAVE_GAME
│   │   ├── social_handlers.cpp/.h       # Friends, Chat, Leaderboard
│   │   ├── user_handlers.cpp/.h         # USER_INFO, VIEW_HISTORY
│   │   ├── admin_handlers.cpp/.h        # Question CRUD, BAN_USER
│   │   └── connection_handlers.cpp/.h   # PING, CONNECTION
│   ├── docs/                    # Server-specific docs
│   │   ├── PROTOCOL_IMPL_STATUS.md      # Implementation tracking
│   │   ├── INTEGRATION.md               # Integration guide
│   │   ├── IMPLEMENTATION_CHECKLIST.md  # Task checklist
│   │   └── AUTH_TOKEN_FLOW.md           # Authentication flow
│   ├── server.cpp               # Main entry point
│   ├── server_core.cpp/.h       # TCP server core (accept, epoll)
│   ├── client_handler.cpp/.h    # Per-client thread handler
│   ├── request_router.cpp/.h    # Route requests to handlers
│   ├── session_manager.cpp/.h   # Track client sessions
│   ├── auth_manager.cpp/.h      # Token generation/validation
│   ├── game_state_manager.cpp/.h    # Game ID generation
│   ├── question_manager.cpp/.h      # Random question selection
│   ├── scoring_system.cpp/.h        # Point calculation
│   ├── lifeline_manager.cpp/.h      # Lifeline logic
│   ├── game_timer.cpp/.h            # 30-second countdown
│   ├── stream_handler.cpp/.h        # Newline-delimited JSON stream
│   ├── json_utils.cpp/.h            # JSON parsing helpers
│   ├── notification_utils.cpp/.h    # Server-to-client notifications
│   ├── logger.cpp/.h                # Logging system
│   ├── config.cpp/.h                # Config file loader
│   ├── config.json              # Server configuration
│   ├── Makefile                 # Build script
│   └── server.log               # Runtime logs
│
├── database/                    # Database schema & integration
│   ├── schema.sql               # Database schema (10 tables)
│   ├── mock_data.sql            # Test data (users, questions)
│   ├── database.cpp/.h          # Database module (libpq)
│   ├── Database Schema Summary.md   # Schema documentation
│   ├── INTEGRATION_GUIDE.md         # Database integration guide
│   ├── TEST_GUIDE.md                # Testing procedures
│   ├── INTEGRATION_COMPLETE.md      # Integration status
│   └── QUICK_TEST_REFERENCE.md      # Quick test commands
│
├── client/                      # Client implementation (C++ + ImGui)
│   ├── bin/                     # Compiled client executable
│   │   └── client               # Main client binary
│   ├── obj/                     # Object files (.o)
│   ├── assets/                  # Images, icons
│   │   ├── lifeline_5050.png
│   │   ├── lifeline_phone.png
│   │   └── lifeline_audience.png
│   ├── third_party/             # Third-party headers
│   │   └── stb_image.h          # Image loading
│   ├── main.cpp                 # GUI main loop (ImGui)
│   ├── socket_client.cpp/.h     # TCP socket client
│   ├── protocol_handler.cpp/.h  # JSON protocol handling
│   ├── json_utils.cpp/.h        # JSON parsing
│   ├── texture_loader.cpp/.h    # Load PNG images
│   ├── game_event.h             # Game event structures
│   ├── Makefile                 # Build script
│   ├── README.md                # Client build instructions
│   ├── BUILD.md                 # Linux build guide
│   ├── BUILD_MACOS.md           # macOS build guide
│   └── imgui.ini                # ImGui layout config
│
├── imgui/                       # ImGui library (submodule/copy)
│   ├── imgui.cpp/.h             # Core ImGui
│   ├── imgui_draw.cpp           # Rendering
│   ├── imgui_widgets.cpp        # UI widgets
│   ├── imgui_tables.cpp         # Table widgets
│   ├── imgui_demo.cpp           # Demo window
│   └── backends/                # Platform backends
│       ├── imgui_impl_glfw.cpp/.h     # GLFW backend
│       └── imgui_impl_opengl3.cpp/.h  # OpenGL3 backend
│
├── glfw/                        # GLFW library (submodule/copy)
│   ├── include/                 # GLFW headers
│   ├── lib/                     # Compiled libraries
│   ├── build/                   # CMake build artifacts
│   └── src/                     # GLFW source files
│
├── docs/                        # Project-wide documentation
│   ├── PROTOCOL.md              # Complete protocol specification
│   ├── ERROR_CODES.md           # All error codes explained
│   ├── NOTIFICATION.md          # Server notifications
│   ├── TEAM_WORKFLOW.md         # Team collaboration guide
│   └── HANDOFF_GUIDE.md         # Handoff documentation
│
├── ERROR_CODES.md               # Root-level error codes reference
├── PROTOCOL.md                  # Root-level protocol reference
├── README.md                    # Project overview
├── SETUP_UBUNTU.md              # Ubuntu setup instructions
├── HUONG_DAN_CHAY_UBUNTU.md     # Vietnamese Ubuntu guide
└── run_all.sh                   # Start server + client script
```

---

## 6. Core Components Deep Dive

### 6.1 Server Components

#### **server_core.cpp** - TCP Server Core
**Purpose**: Accept client connections, manage server lifecycle  
**Key Methods**:
- `start()`: Bind socket, listen on port 8080
- `run()`: Main event loop (epoll/select), accept new clients
- `shutdown()`: Clean shutdown, close all connections

**Location**: `/server/server_core.cpp`

#### **client_handler.cpp** - Per-Client Thread
**Purpose**: Handle one client connection in separate thread  
**Key Methods**:
- `run()`: Receive loop for client messages
- `handleMessage()`: Parse JSON, call request router
- `sendNotification()`: Push server-initiated messages

**Location**: `/server/client_handler.cpp`

#### **request_router.cpp** - Request Dispatcher
**Purpose**: Route incoming requests to appropriate handlers  
**Key Methods**:
- `processRequest()`: Main routing logic
  - Extract `requestType` from JSON
  - Validate authentication (except LOGIN/REGISTER/CONNECTION)
  - Call specific handler function
  - Return JSON response
- `requiresAuth()`: Check if request needs authentication

**Location**: `/server/request_router.cpp`

**Routing Table**:
```cpp
if (request_type == "LOGIN")         → AuthHandlers::handleLogin()
else if (request_type == "START")    → GameHandlers::handleStart()
else if (request_type == "LEADERBOARD") → SocialHandlers::handleLeaderboard()
else if (request_type == "ADD_QUES") → AdminHandlers::handleAddQues()
// ... 27 total routes
```

#### **session_manager.cpp** - Client Session Tracking
**Purpose**: Track connected clients and their session data  
**Key Methods**:
- `createSession(client_fd)`: Create new session on connect
- `getSession(client_fd)`: Retrieve session by file descriptor
- `removeSession(client_fd)`: Clean up on disconnect
- `updateSession()`: Update session state (username, game, etc.)

**Data Structure**:
```cpp
struct ClientSession {
    int client_fd;
    std::string username;
    std::string current_game_id;
    bool authenticated;
    time_t last_activity;
};
```

**Location**: `/server/session_manager.cpp`

#### **auth_manager.cpp** - Authentication & Token Management
**Purpose**: Generate/validate auth tokens, manage auth state  
**Key Methods**:
- `generateToken(username)`: Create 32-char hex token
- `validateToken(authToken)`: Check if token valid & not expired
- `requireAuth(request, session, client_fd)`: Validate request has valid token
- `revokeToken(authToken)`: Logout, invalidate token
- `getUsername(authToken)`: Map token → username

**Token Format**: 32 hexadecimal characters (e.g., `a1b2c3d4e5f6...`)  
**Storage**: In-memory map (`authToken → username`)

**Location**: `/server/auth_manager.cpp`

#### **game_state_manager.cpp** - Game Session Management
**Purpose**: Generate unique game IDs, track game states  
**Key Methods**:
- `generateGameId()`: Atomic counter for unique IDs
- `loadGameProgress(username)`: Get saved game from DB
- `saveGameProgress(username, level, prize)`: Auto-save game

**Location**: `/server/game_state_manager.cpp`

#### **question_manager.cpp** - Question Selection
**Purpose**: Randomly select questions for games  
**Key Methods**:
- `getRandomQuestionForLevel(level)`: Pick random active question
- `assignQuestionsToGame(gameId)`: Assign 15 questions (levels 1-15)

**Algorithm**:
- For each level 1-15: Query database for active questions at that level
- Randomly select one question
- Insert into `game_questions` table with `question_order`

**Location**: `/server/question_manager.cpp`

#### **scoring_system.cpp** - Point Calculation
**Purpose**: Calculate points earned per question  
**Formula**:
```
pointsEarned = max(0, timeRemaining - (5 * numberOfLifelinesUsedThisQuestion))
```

**Rules**:
- Max 30 points per question (answer in 0 seconds)
- Each second elapsed reduces by 1 point
- Each lifeline used reduces by 5 points (applied when lifeline used, not when answering)
- Minimum 0 points

**Example**:
```
Answer in 15s, no lifeline: 30 - 15 = 15 points
Answer in 10s, used 1 lifeline: 30 - 10 - 5 = 15 points
Answer in 25s, used 2 lifelines: 30 - 25 - 10 = 0 points (max(0, ...))
```

**Location**: `/server/scoring_system.cpp`

#### **lifeline_manager.cpp** - Lifeline Logic
**Purpose**: Implement 50/50, Phone, Audience lifeline effects  

**50/50 (5050)**:
- Read `lifeline_5050_info` JSON array from question (e.g., `[0, 2]`)
- Return indices of correct answer + one wrong answer (2 total)
- Delay: 5 seconds

**Phone a Friend (PHONE)**:
- Read `lifeline_call_info` text from question (e.g., "I'm 85% sure it's A")
- Return suggestion with confidence message
- Delay: 10 seconds

**Ask the Audience (AUDIENCE)**:
- Read `lifeline_ask_info` JSON object from question (e.g., `{"A":65,"B":15,"C":10,"D":10}`)
- Return poll percentages (must sum to 100)
- Delay: 5 seconds

**Location**: `/server/lifeline_manager.cpp`

#### **game_timer.cpp** - Question Timer
**Purpose**: Track 30-second countdown per question  
**Key Methods**:
- `startTimer(gameId, questionNumber)`: Begin 30s countdown
- `stopTimer(gameId)`: Cancel timer
- `getTimeRemaining(gameId)`: Query remaining seconds
- `isExpired(gameId)`: Check if 30s elapsed

**Behavior**: If timer expires, server sends GAME_END notification with `status: "lost"` and safe checkpoint prize

**Location**: `/server/game_timer.cpp`

### 6.2 Database Module

#### **database.cpp** - PostgreSQL Interface
**Purpose**: All database operations (CRUD for 10 tables)  

**Connection**:
```cpp
bool connect(const std::string& connection_string);
// Example: "host=localhost dbname=millionaire_game user=postgres"
```

**User Operations**:
- `authenticateUser()`: Verify username + password hash
- `registerUser()`: Create new user (hash password)
- `changePassword()`: Update password after verifying old one
- `banUser()`: Set `is_banned = true`, record reason
- `getUserRole()`: Return "user" or "admin"
- `updateLastLogin()`: Update `last_login` timestamp

**Game Operations**:
- `createGameSession()`: INSERT into `game_sessions`, return `gameId`
- `updateGameSession()`: Update current_question_number, total_score, current_prize
- `getActiveGameSession()`: Get user's current game (status='active')
- `endGame()`: Set status='won'/'lost'/'quit', set `ended_at`, `final_prize`
- `addGameQuestion()`: INSERT into `game_questions` (game_id, question_order, question_id)
- `addGameAnswer()`: INSERT into `game_answers` (selected_option, is_correct, response_time)

**Saved Game Operations**:
- `saveGameProgress()`: INSERT/UPDATE `saved_games` table
- `loadGameProgress()`: Retrieve saved game for RESUME

**Leaderboard Operations**:
- `getLeaderboard()`: Query `leaderboard` table, join with `users`, order by `final_question_number` DESC, `total_score` DESC
- `updateLeaderboard()`: Update/INSERT user's best score (only if new game is better)

**Friend Operations**:
- `getFriendsList()`: Query `friendships` table (bidirectional: `user1_id` or `user2_id`)
- `addFriendRequest()`: INSERT into `friend_requests` with status='pending'
- `acceptFriendRequest()`: Update status='accepted', INSERT into `friendships`
- `declineFriendRequest()`: Delete from `friend_requests`
- `deleteFriend()`: DELETE from `friendships`
- `getFriendRequests()`: Query pending requests for user

**Admin Operations**:
- `addQuestion()`: INSERT into `questions` (returns auto-generated `questionId`)
- `updateQuestion()`: UPDATE `questions` SET ..., update `updated_at`, `updated_by`
- `deleteQuestion()`: Soft delete (UPDATE `is_active = false`)
- `getQuestions()`: Paginated list, filter by level
- `getRandomQuestion(level)`: SELECT random active question WHERE `level = X AND is_active = true`

**Location**: `/database/database.cpp`

### 6.3 Client Components

#### **main.cpp** - GUI Main Loop
**Purpose**: ImGui application with game UI  

**Key Screens**:
1. **Login/Register Screen**: Username/password input
2. **Main Menu**: Start Game, View History, Leaderboard, Friends, Admin Panel, Logout
3. **Game Screen**: Question text, 4 answer buttons (A/B/C/D), 3 lifeline buttons, timer, prize display
4. **Leaderboard Screen**: Paginated ranking table (global or friends)
5. **Friend List Screen**: Friend status (online/offline/ingame), send requests, chat
6. **Admin Panel**: Question CRUD form, ban user form (only shown if role='admin')

**UI State Machine**:
```cpp
enum UIState {
    STATE_DISCONNECTED,
    STATE_LOGIN,
    STATE_MAIN_MENU,
    STATE_IN_GAME,
    STATE_LEADERBOARD,
    STATE_FRIENDS,
    STATE_ADMIN_PANEL
};
```

**Rendering Loop**:
```cpp
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render current screen based on UIState
    if (uiState == STATE_LOGIN) {
        renderLoginScreen();
    } else if (uiState == STATE_IN_GAME) {
        renderGameScreen();
    }
    // ...
    
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}
```

**Location**: `/client/main.cpp`

#### **socket_client.cpp** - TCP Client Socket
**Purpose**: Connect to server, send/receive JSON messages  

**Key Methods**:
- `connect()`: Establish TCP connection to server (host:port)
- `disconnect()`: Close socket
- `sendRequest(requestType, data)`: Send JSON request with newline delimiter
- `getMessage(msg, timeoutMs)`: Pop message from receive queue (blocking with timeout)
- `receiveLoop()`: Background thread continuously reading from socket

**Threading Model**:
- Main thread: GUI rendering, user input
- Receive thread: Continuously read from socket, parse JSON, enqueue messages

**Message Queue**:
```cpp
std::queue<Message> messageQueue_;
std::mutex queueMutex_;

struct Message {
    std::string type;    // "QUESTION_INFO", "ANSWER", etc.
    std::string data;    // JSON string
};
```

**Location**: `/client/socket_client.cpp`

#### **protocol_handler.cpp** - Protocol Logic
**Purpose**: Build JSON requests, parse JSON responses  

**Key Methods**:
- `buildLoginRequest(username, password)`: Return JSON string
- `parseLoginResponse(jsonStr)`: Extract authToken, username, role
- `buildStartRequest(authToken)`: Return JSON string
- `parseQuestionInfo(jsonStr)`: Extract question text, options, prize, timeRemaining
- `buildAnswerRequest(authToken, gameId, questionNumber, answerIndex)`: Return JSON string
- `parseAnswerResponse(jsonStr)`: Extract correct, pointsEarned, totalScore, gameOver

**Example**:
```cpp
std::string buildLoginRequest(const std::string& username, const std::string& password) {
    return "{\"requestType\":\"LOGIN\",\"data\":{\"username\":\"" + username + 
           "\",\"password\":\"" + password + "\"}}\n";
}
```

**Location**: `/client/protocol_handler.cpp`

---

## 7. Database Schema

### 7.1 Schema Overview (10 Tables)

#### **users** - User accounts
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(10) CHECK (role IN ('user', 'admin')) DEFAULT 'user',
    is_banned BOOLEAN DEFAULT FALSE,
    ban_reason TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);
```

#### **questions** - Question bank
```sql
CREATE TABLE questions (
    id SERIAL PRIMARY KEY,
    question_text TEXT NOT NULL,
    option_a TEXT NOT NULL,
    option_b TEXT NOT NULL,
    option_c TEXT NOT NULL,
    option_d TEXT NOT NULL,
    correct_answer INTEGER CHECK (correct_answer >= 0 AND correct_answer <= 3) NOT NULL,
    level INTEGER CHECK (level >= 1 AND level <= 15) NOT NULL,
    lifeline_5050_info TEXT,     -- JSON: "[0,2]" (indices to keep)
    lifeline_ask_info TEXT,      -- JSON: "{\"A\":65,\"B\":15,\"C\":10,\"D\":10}"
    lifeline_call_info TEXT,     -- Text: "I'm 85% sure it's A"
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP,
    updated_by INTEGER REFERENCES users(id)
);
```

#### **game_sessions** - Active & completed games
```sql
CREATE TABLE game_sessions (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) CHECK (status IN ('active', 'won', 'lost', 'quit')) DEFAULT 'active',
    current_question_number INTEGER DEFAULT 1,
    current_level INTEGER DEFAULT 1,
    current_prize BIGINT DEFAULT 1000000,
    total_score INTEGER DEFAULT 0,
    final_prize BIGINT,
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP
);
```

#### **game_questions** - Questions assigned to each game
```sql
CREATE TABLE game_questions (
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_order INTEGER CHECK (question_order >= 1 AND question_order <= 15),
    question_id INTEGER REFERENCES questions(id) ON DELETE CASCADE,
    PRIMARY KEY (game_id, question_order)
);
```

#### **game_answers** - Player's submitted answers
```sql
CREATE TABLE game_answers (
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_order INTEGER,
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    selected_option INTEGER CHECK (selected_option >= 0 AND selected_option <= 3),
    is_correct BOOLEAN NOT NULL,
    response_time_second INTEGER,
    PRIMARY KEY (game_id, question_order)
);
```

#### **saved_games** - Auto-saved game progress
```sql
CREATE TABLE saved_games (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE CASCADE,
    question_number INTEGER NOT NULL,
    prize BIGINT NOT NULL,
    score INTEGER NOT NULL,
    used_lifelines TEXT,  -- JSON: "[\"5050\",\"PHONE\"]"
    saved_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

#### **friendships** - Accepted friend relationships
```sql
CREATE TABLE friendships (
    id SERIAL PRIMARY KEY,
    user1_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    user2_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user1_id, user2_id),
    CHECK (user1_id < user2_id)  -- Ensures single record per friendship
);
```

#### **friend_requests** - Pending friend requests
```sql
CREATE TABLE friend_requests (
    id SERIAL PRIMARY KEY,
    from_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    to_user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) CHECK (status IN ('pending', 'accepted', 'declined')) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (from_user_id, to_user_id),
    CHECK (from_user_id != to_user_id)
);
```

#### **messages** - Chat messages
```sql
CREATE TABLE messages (
    id SERIAL PRIMARY KEY,
    sender_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    receiver_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    game_id INTEGER REFERENCES game_sessions(id) ON DELETE SET NULL,
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    content TEXT NOT NULL,
    is_read BOOLEAN DEFAULT FALSE
);
```

#### **leaderboard** - Cached best scores
```sql
CREATE TABLE leaderboard (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE UNIQUE,
    final_question_number INTEGER,  -- Highest question reached (1-15)
    total_score BIGINT NOT NULL,
    highest_prize BIGINT NOT NULL,
    games_played INTEGER DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 7.2 Key Relationships
- **users** ← **game_sessions** (user plays many games)
- **game_sessions** ← **game_questions** (game has 15 questions)
- **questions** ← **game_questions** (question used in many games)
- **game_sessions** ← **game_answers** (game has many answers)
- **users** ← **saved_games** (user has at most one saved game)
- **users** ↔ **friendships** (bidirectional, enforced by `user1_id < user2_id`)
- **users** → **friend_requests** (user sends requests to other users)
- **users** ↔ **messages** (user sends/receives messages)
- **users** ← **leaderboard** (one-to-one, best score cached)

### 7.3 Indexes (Performance Optimization)
```sql
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_questions_level ON questions(level);
CREATE INDEX idx_questions_is_active ON questions(is_active);
CREATE INDEX idx_game_sessions_user_id ON game_sessions(user_id);
CREATE INDEX idx_game_sessions_status ON game_sessions(status);
CREATE INDEX idx_game_questions_game_id ON game_questions(game_id);
CREATE INDEX idx_game_answers_game_id ON game_answers(game_id);
CREATE INDEX idx_saved_games_user_id ON saved_games(user_id);
CREATE INDEX idx_friendships_user1 ON friendships(user1_id);
CREATE INDEX idx_friendships_user2 ON friendships(user2_id);
CREATE INDEX idx_friend_requests_to_user ON friend_requests(to_user_id);
CREATE INDEX idx_leaderboard_total_score ON leaderboard(total_score DESC);
CREATE INDEX idx_leaderboard_final_question ON leaderboard(final_question_number DESC);
```

---

## 8. Request Flow

### 8.1 Authentication Flow (LOGIN)

```
┌──────────┐                          ┌──────────┐                           ┌──────────┐
│  Client  │                          │  Server  │                           │ Database │
└────┬─────┘                          └────┬─────┘                           └────┬─────┘
     │                                     │                                      │
     │ 1. User enters username/password    │                                      │
     ├────────────────────────────────────>│                                      │
     │ {"requestType":"LOGIN",             │                                      │
     │  "data":{"username":"player1",      │                                      │
     │          "password":"TestPass123"}} │                                      │
     │                                     │                                      │
     │                                     │ 2. Extract username/password         │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT password_hash, role, is_banned│
     │                                     │ FROM users WHERE username='player1'  │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ password_hash, role='user',          │
     │                                     │ is_banned=false                      │
     │                                     │                                      │
     │                                     │ 3. Verify password (hash + compare)  │
     │                                     │    Generate authToken (32-char hex)  │
     │                                     │    Store token → username mapping    │
     │                                     │                                      │
     │                                     │ 4. Update last_login                 │
     │                                     │────────────────────────────────────> │
     │                                     │ UPDATE users SET last_login=NOW()    │
     │                                     │ WHERE username='player1'             │
     │                                     │                                      │
     │<────────────────────────────────────│                                      │
     │ {"responseCode":200,                │                                      │
     │  "data":{"authToken":"a1b2c3...",   │                                      │
     │          "username":"player1",      │                                      │
     │          "role":"user",             │                                      │
     │          "message":"Login successful"}}                                    │
     │                                     │                                      │
     │ 5. Store authToken for future reqs  │                                      │
```

**Error Cases**:
- Wrong password → `401 LOGIN_FAILED`
- Username not found → `401 LOGIN_FAILED`
- User is banned → `403 FORBIDDEN`

### 8.2 Game Start Flow (START)

```
┌──────────┐                          ┌──────────┐                          ┌──────────┐
│  Client  │                          │  Server  │                          │ Database │
└────┬─────┘                          └────┬─────┘                          └────┬─────┘
     │                                     │                                      │
     │ 1. User clicks "Start Game"         │                                      │
     ├────────────────────────────────────>│                                      │
     │ {"requestType":"START",             │                                      │
     │  "data":{"authToken":"a1b2c3..."}}  │                                      │
     │                                     │                                      │
     │                                     │ 2. Validate authToken                │
     │                                     │    (AuthManager::requireAuth)        │
     │                                     │    Extract username from token       │
     │                                     │                                      │
     │                                     │ 3. Check if user already in game     │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT id FROM game_sessions         │
     │                                     │ WHERE user_id=X AND status='active'  │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ (no results → OK to start)           │
     │                                     │                                      │
     │                                     │ 4. Create game session               │
     │                                     │────────────────────────────────────> │
     │                                     │ INSERT INTO game_sessions            │
     │                                     │ (user_id, status, current_question_  │
     │                                     │  number, current_prize, total_score) │
     │                                     │ VALUES (X,'active',1,1000000,0)      │
     │                                     │ RETURNING id                         │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ gameId = 12345                       │
     │                                     │                                      │
     │                                     │ 5. Assign 15 random questions        │
     │                                     │────────────────────────────────────> │
     │                                     │ For level 1-15:                      │
     │                                     │   SELECT id FROM questions           │
     │                                     │   WHERE level=L AND is_active=true   │
     │                                     │   ORDER BY RANDOM() LIMIT 1          │
     │                                     │                                      │
     │                                     │   INSERT INTO game_questions         │
     │                                     │   (game_id, question_order,          │
     │                                     │    question_id) VALUES (12345,L,Q)   │
     │                                     │                                      │
     │ <────────────────────────────────────│                                      │
     │ NOTIFICATION: GAME_START            │                                      │
     │ {"responseCode":200,                │                                      │
     │  "data":{"message":"Game started",  │                                      │
     │          "gameId":12345,            │                                      │
     │          "timestamp":1705320000}}   │                                      │
     │                                     │                                      │
     │                                     │ 6. Fetch first question              │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT q.* FROM questions q          │
     │                                     │ JOIN game_questions gq ON q.id=gq.id │
     │                                     │ WHERE gq.game_id=12345 AND           │
     │                                     │       gq.question_order=1            │
     │                                     │                                      │
     │ <────────────────────────────────────│                                      │
     │ NOTIFICATION: QUESTION_INFO         │                                      │
     │ {"responseCode":200,                │                                      │
     │  "data":{"questionId":100,          │                                      │
     │          "questionNumber":1,        │                                      │
     │          "question":"What is...",   │                                      │
     │          "options":[...],           │                                      │
     │          "prize":1000000,           │                                      │
     │          "lifelines":["5050",...],  │                                      │
     │          "timeLimit":30,            │                                      │
     │          "timeRemaining":30,        │                                      │
     │          "gameId":12345,            │                                      │
     │          "totalScore":0}}           │                                      │
     │                                     │                                      │
     │ 7. Start 30-second timer            │ 8. Server starts GameTimer(12345,1)  │
```

**Error Cases**:
- User already in game → `405 USER_ALREADY_IN_GAME`
- User has saved game and `overrideSavedGame` not set → `412 INVALID_GAME_STATE`
- Invalid authToken → `402 AUTH_ERROR`

### 8.3 Answer Question Flow (ANSWER)

```
┌──────────┐                          ┌──────────┐                          ┌──────────┐
│  Client  │                          │  Server  │                          │ Database │
└────┬─────┘                          └────┬─────┘                          └────┬─────┘
     │                                     │                                      │
     │ 1. User selects answer (e.g., B=1)  │                                      │
     ├────────────────────────────────────>│                                      │
     │ {"requestType":"ANSWER",            │                                      │
     │  "data":{"authToken":"a1b2c3...",   │                                      │
     │          "gameId":12345,            │                                      │
     │          "questionNumber":1,        │                                      │
     │          "answerIndex":1}}          │                                      │
     │                                     │                                      │
     │                                     │ 2. Validate authToken                │
     │                                     │    Extract username                  │
     │                                     │                                      │
     │                                     │ 3. Validate gameId matches active    │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT * FROM game_sessions          │
     │                                     │ WHERE id=12345 AND user_id=X         │
     │                                     │       AND status='active'            │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ current_question_number=1,           │
     │                                     │ total_score=0, current_prize=1000000 │
     │                                     │                                      │
     │                                     │ 4. Check question number matches     │
     │                                     │    (questionNumber=1 == current=1 ✓) │
     │                                     │                                      │
     │                                     │ 5. Check timeout (GameTimer)         │
     │                                     │    timeElapsed = 15s, timeout=30s ✓  │
     │                                     │                                      │
     │                                     │ 6. Get correct answer from DB        │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT correct_answer FROM questions │
     │                                     │ WHERE id IN (SELECT question_id      │
     │                                     │  FROM game_questions WHERE           │
     │                                     │  game_id=12345 AND question_order=1) │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ correct_answer = 1                   │
     │                                     │                                      │
     │                                     │ 7. Check if answer correct           │
     │                                     │    (answerIndex=1 == correct=1 ✓)   │
     │                                     │                                      │
     │                                     │ 8. Calculate points                  │
     │                                     │    timeRemaining = 30 - 15 = 15s     │
     │                                     │    lifelinesUsed = 0                 │
     │                                     │    pointsEarned = 15 - 0 = 15        │
     │                                     │                                      │
     │                                     │ 9. Record answer in DB               │
     │                                     │────────────────────────────────────> │
     │                                     │ INSERT INTO game_answers             │
     │                                     │ (game_id, question_order,            │
     │                                     │  selected_option, is_correct,        │
     │                                     │  response_time_second)               │
     │                                     │ VALUES (12345,1,1,true,15)           │
     │                                     │                                      │
     │                                     │ 10. Update game session              │
     │                                     │────────────────────────────────────> │
     │                                     │ UPDATE game_sessions SET             │
     │                                     │ current_question_number=2,           │
     │                                     │ total_score=15,                      │
     │                                     │ current_prize=2000000                │
     │                                     │ WHERE id=12345                       │
     │                                     │                                      │
     │ <────────────────────────────────────│                                      │
     │ {"responseCode":200,                │                                      │
     │  "data":{"gameId":12345,            │                                      │
     │          "correct":true,            │                                      │
     │          "questionNumber":1,        │                                      │
     │          "timeRemaining":15,        │                                      │
     │          "pointsEarned":15,         │                                      │
     │          "totalScore":15,           │                                      │
     │          "currentPrize":2000000,    │                                      │
     │          "gameOver":false,          │                                      │
     │          "isWinner":false}}         │                                      │
     │                                     │                                      │
     │                                     │ 11. Send next QUESTION_INFO          │
     │ <────────────────────────────────────│                                      │
     │ NOTIFICATION: QUESTION_INFO         │                                      │
     │ (questionNumber=2, ...)             │                                      │
```

**Error Cases**:
- Wrong gameId → `412 INVALID_GAME_STATE`
- Wrong questionNumber → `422 UNPROCESSABLE_DATA`
- Timeout (30s elapsed) → `408 QUESTION_TIMEOUT` (game ends with status "lost")
- Invalid answerIndex (not 0-3) → `422 UNPROCESSABLE_DATA`
- User not in game → `406 USER_NOT_IN_GAME`

### 8.4 Lifeline Flow (LIFELINE)

```
┌──────────┐                          ┌──────────┐                          ┌──────────┐
│  Client  │                          │  Server  │                          │ Database │
└────┬─────┘                          └────┬─────┘                          └────┬─────┘
     │                                     │                                      │
     │ 1. User clicks "50/50" button       │                                      │
     ├────────────────────────────────────>│                                      │
     │ {"requestType":"LIFELINE",          │                                      │
     │  "data":{"authToken":"a1b2c3...",   │                                      │
     │          "gameId":12345,            │                                      │
     │          "questionNumber":1,        │                                      │
     │          "lifelineType":"5050"}}    │                                      │
     │                                     │                                      │
     │                                     │ 2. Validate authToken, gameId,       │
     │                                     │    questionNumber (same as ANSWER)   │
     │                                     │                                      │
     │                                     │ 3. Check lifeline not already used   │
     │                                     │    (track in session or query DB)    │
     │                                     │                                      │
     │                                     │ 4. Get current question              │
     │                                     │────────────────────────────────────> │
     │                                     │ SELECT lifeline_5050_info            │
     │                                     │ FROM questions WHERE id IN (...)     │
     │                                     │                                      │
     │                                     │ <────────────────────────────────────│
     │                                     │ lifeline_5050_info = "[0,2]"         │
     │                                     │                                      │
     │                                     │ 5. Parse JSON: [0,2] → keep A & C    │
     │                                     │                                      │
     │                                     │ 6. Delay 5 seconds (simulate)        │
     │                                     │    (GameTimer paused during lifeline)│
     │                                     │                                      │
     │ <────────────────────────────────────│                                      │
     │ NOTIFICATION: LIFELINE_INFO         │                                      │
     │ {"responseCode":200,                │                                      │
     │  "data":{"lifelineType":"5050",     │                                      │
     │          "questionNumber":1,        │                                      │
     │          "remainingOptions":[0,2],  │                                      │
     │          "lifelinesLeft":           │                                      │
     │           ["PHONE","AUDIENCE"],     │                                      │
     │          "timeRemaining":25,        │                                      │
     │          "lifelinePenalty":5,       │                                      │
     │          "maxPointsAfterLifeline":  │                                      │
     │           25}}                      │                                      │
     │                                     │                                      │
     │ 7. Client grays out B & D buttons   │                                      │
     │    Client updates max points: 25    │                                      │
```

**Lifeline Types**:
- **5050**: Read `lifeline_5050_info` JSON (e.g., `[0,2]`), return those indices (correct + 1 wrong)
- **PHONE**: Read `lifeline_call_info` text (e.g., "I'm 85% sure it's A"), return message
- **AUDIENCE**: Read `lifeline_ask_info` JSON (e.g., `{"A":65,"B":15,"C":10,"D":10}`), return poll

**Error Cases**:
- Lifeline already used → `407 LIFELINE_ALREADY_CHOSEN`
- Invalid lifelineType → `422 UNPROCESSABLE_DATA`
- User not in game → `406 USER_NOT_IN_GAME`
- Wrong gameId/questionNumber → `412` or `422`

---

## 9. Where to Find & Modify Code

### 9.1 Adding a New Request Type

**Example**: Add `VIEW_STATS` request to show user statistics

**Step 1**: Define in protocol (documentation)
- Edit `/docs/PROTOCOL.md`: Add new section under "User Information" describing request/response format

**Step 2**: Add error handling (if needed)
- Edit `/docs/ERROR_CODES.md`: Document any new error codes

**Step 3**: Create handler function
- Edit `/server/request_handlers/user_handlers.cpp`:
```cpp
string UserHandlers::handleViewStats(const string& request, ClientSession& session) {
    // 1. Parse request JSON
    // 2. Query database for statistics
    // 3. Build JSON response
    // 4. Return response string
}
```

**Step 4**: Declare handler in header
- Edit `/server/request_handlers/user_handlers.h`:
```cpp
static string handleViewStats(const string& request, ClientSession& session);
```

**Step 5**: Route request
- Edit `/server/request_router.cpp`:
```cpp
else if (request_type == "VIEW_STATS") {
    return UserHandlers::handleViewStats(request, *session);
}
```

**Step 6**: Update Makefile (if new file created)
- Edit `/server/Makefile`: Add new source/object files if you created new .cpp files

**Step 7**: Update protocol status
- Edit `/server/docs/PROTOCOL_IMPL_STATUS.md`: Mark new request as implemented

**Step 8**: Add database method (if needed)
- Edit `/database/database.h`: Declare new method
- Edit `/database/database.cpp`: Implement method

**Step 9**: Test
- Use `nc localhost 8080` or client to send `{"requestType":"VIEW_STATS","data":{...}}`

### 9.2 Adding a New Error Code

**Example**: Add `413 PAYLOAD_TOO_LARGE` error code

**Step 1**: Document in ERROR_CODES.md
- Edit `/docs/ERROR_CODES.md`: Add section describing code 413

**Step 2**: Use in handlers
- Edit `/server/request_handlers/*.cpp`: Return error where applicable:
```cpp
if (message_size > MAX_MESSAGE_SIZE) {
    return StreamUtils::createErrorResponse(413, "Payload too large");
}
```

### 9.3 Modifying Game Rules

**Example**: Change max points per question from 30 to 60

**Step 1**: Update protocol documentation
- Edit `/docs/PROTOCOL.md`: Update "Scoring System" section

**Step 2**: Update scoring calculation
- Edit `/server/scoring_system.cpp`:
```cpp
const int MAX_POINTS_PER_QUESTION = 60;  // Changed from 30
```

**Step 3**: Update database schema (if needed)
- Edit `/database/schema.sql`: Adjust constraints if necessary

**Step 4**: Update client display
- Edit `/client/main.cpp`: Update max points display in UI

**Step 5**: Update tests
- Edit `/database/TEST_GUIDE.md`: Update expected values in test cases

### 9.4 Adding a New Database Table

**Example**: Add `achievements` table

**Step 1**: Design schema
- Edit `/database/Database Schema Summary.md`: Add new table section

**Step 2**: Update schema.sql
- Edit `/database/schema.sql`:
```sql
CREATE TABLE achievements (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    achievement_name VARCHAR(100) NOT NULL,
    unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Step 3**: Add struct to database.h
- Edit `/database/database.h`:
```cpp
struct Achievement {
    int id;
    int user_id;
    std::string achievement_name;
    time_t unlocked_at;
};
```

**Step 4**: Add database methods
- Edit `/database/database.h`: Declare methods
```cpp
bool unlockAchievement(int user_id, const std::string& achievement_name);
std::vector<Achievement> getUserAchievements(int user_id);
```

- Edit `/database/database.cpp`: Implement methods

**Step 5**: Recreate database
```bash
psql -U postgres -d postgres -c "DROP DATABASE millionaire_game;"
createdb -U postgres millionaire_game
psql -U postgres -d millionaire_game < database/schema.sql
psql -U postgres -d millionaire_game < database/mock_data.sql
```

### 9.5 Modifying Client UI

**Example**: Add new screen for achievements

**Step 1**: Add UI state enum
- Edit `/client/main.cpp`:
```cpp
enum UIState {
    // ... existing states
    STATE_ACHIEVEMENTS
};
```

**Step 2**: Create render function
- Edit `/client/main.cpp`:
```cpp
void renderAchievementsScreen() {
    ImGui::Begin("Achievements");
    // ... render achievement list
    ImGui::End();
}
```

**Step 3**: Add navigation button
- Edit `/client/main.cpp` in `renderMainMenu()`:
```cpp
if (ImGui::Button("Achievements")) {
    uiState = STATE_ACHIEVEMENTS;
}
```

**Step 4**: Add to main render loop
- Edit `/client/main.cpp` in `main()`:
```cpp
if (uiState == STATE_ACHIEVEMENTS) {
    renderAchievementsScreen();
}
```

**Step 5**: Add protocol support
- Edit `/client/protocol_handler.cpp`: Add methods to build/parse requests/responses

### 9.6 Changing Server Configuration

**Step 1**: Edit config file
- Edit `/server/config.json`:
```json
{
    "port": 8080,
    "log_file": "server.log",
    "max_clients": 100,
    "database": {
        "host": "localhost",
        "port": 5432,
        "dbname": "millionaire_game",
        "user": "postgres",
        "password": ""
    }
}
```

**Step 2**: Update config loader (if new fields)
- Edit `/server/config.h`: Add new fields to `ServerConfig` struct
- Edit `/server/config.cpp`: Parse new fields from JSON

**Step 3**: Use config in server code
- Edit `/server/server_core.cpp`: Access config via `config.port`, etc.

### 9.7 File Reference Quick Guide

| Task | File(s) to Edit |
|------|----------------|
| Add new request type | `request_router.cpp`, `request_handlers/*.cpp` |
| Change game rules | `scoring_system.cpp`, `PROTOCOL.md` |
| Add error code | `ERROR_CODES.md`, use in handlers |
| Modify database schema | `schema.sql`, `database.h`, `database.cpp` |
| Change UI layout | `client/main.cpp` |
| Update authentication | `auth_manager.cpp`, `auth_handlers.cpp` |
| Modify lifeline behavior | `lifeline_manager.cpp`, `game_handlers.cpp` |
| Change scoring formula | `scoring_system.cpp` |
| Add server logging | `logger.cpp`, add calls in handlers |
| Change network protocol | `stream_handler.cpp`, `socket_client.cpp` |
| Update question selection | `question_manager.cpp` |
| Modify timer behavior | `game_timer.cpp` |
| Change prize progression | `game_handlers.cpp` (safe checkpoint logic) |

---

## 10. Development Workflow

### 10.1 Initial Setup

#### **Prerequisites**
- C++11 compiler (g++ or clang++)
- PostgreSQL 14+ installed
- CMake (for GLFW build)
- OpenGL development libraries
- pkg-config (optional, for system GLFW)

#### **Database Setup**
```bash
# 1. Create database
createdb -U postgres millionaire_game

# 2. Load schema
cd "Network Programming"
psql -U postgres -d millionaire_game < database/schema.sql

# 3. Load test data (optional)
psql -U postgres -d millionaire_game < database/mock_data.sql

# 4. Verify tables
psql -U postgres -d millionaire_game -c "\dt"
# Should show 10 tables: users, questions, game_sessions, etc.
```

#### **Server Setup**
```bash
cd server

# 1. Configure database connection
cp config.json.example config.json
# Edit config.json: Set database credentials

# 2. Build server
make clean
make

# 3. Run server
./bin/server
# Should see: "Database connected successfully"
#             "Server started on port 8080"
```

#### **Client Setup**
```bash
cd client

# 1. Build GLFW (if not using system library)
make rebuild-glfw

# 2. Build client
make clean
make

# 3. Run client
./bin/client
# GUI window should open
```

### 10.2 Development Cycle

**Typical workflow for adding a feature**:

1. **Design Phase**
   - Update `/docs/PROTOCOL.md` with new request/response format
   - Update `/docs/ERROR_CODES.md` if adding new errors
   - Design database schema changes (if needed)

2. **Server Implementation**
   - Add handler in `/server/request_handlers/`
   - Add route in `/server/request_router.cpp`
   - Add database methods in `/database/database.cpp` (if needed)
   - Update `/server/docs/PROTOCOL_IMPL_STATUS.md`

3. **Build & Test Server**
   ```bash
   cd server
   make
   ./bin/server
   
   # In another terminal, test with nc
   echo '{"requestType":"NEW_REQUEST","data":{...}}' | nc localhost 8080
   ```

4. **Client Implementation**
   - Add protocol methods in `/client/protocol_handler.cpp`
   - Add UI in `/client/main.cpp`
   - Update socket handling if needed

5. **Build & Test Client**
   ```bash
   cd client
   make
   ./bin/client
   # Test in GUI
   ```

6. **Integration Testing**
   - Run server + client together
   - Test full request/response flow
   - Verify database updates
   - Check error handling

7. **Documentation**
   - Update `/database/TEST_GUIDE.md` with test cases
   - Update this technical documentation if architecture changed

### 10.3 Build Commands Reference

#### Server
```bash
cd server

# Clean build
make clean && make

# Build and run
make run

# Just compile (no run)
make

# View build artifacts
ls obj/      # Object files
ls bin/      # Executable

# Check dependencies
make -n      # Dry run, show what would be compiled
```

#### Client
```bash
cd client

# Clean build
make clean && make

# Rebuild GLFW (if issues)
make rebuild-glfw

# Build and run
make run

# Check GLFW status
pkg-config --exists glfw3 && echo "System GLFW found" || echo "Building from source"
```

#### Database
```bash
# Reset database (WARNING: Deletes all data)
cd "Network Programming"
psql -U postgres -d postgres -c "DROP DATABASE millionaire_game;"
createdb -U postgres millionaire_game
psql -U postgres -d millionaire_game < database/schema.sql
psql -U postgres -d millionaire_game < database/mock_data.sql

# Quick checks
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM users;"
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM questions;"
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM game_sessions WHERE status='active';"
```

### 10.4 Debugging Tips

#### Server Debugging
```bash
# View server logs
tail -f server/server.log

# Run with verbose logging
cd server
./bin/server -l server_debug.log

# Check which port server is using
lsof -i :8080
# or
netstat -an | grep 8080

# Test server with raw TCP
nc localhost 8080
{"requestType":"PING","data":{"authToken":"test"}}
# (Press Enter, should get response)
```

#### Database Debugging
```bash
# Check if PostgreSQL running
pg_isready

# View active connections
psql -U postgres -d millionaire_game -c "SELECT * FROM pg_stat_activity WHERE datname='millionaire_game';"

# Check recent game sessions
psql -U postgres -d millionaire_game -c "SELECT id, user_id, status, current_question_number FROM game_sessions ORDER BY started_at DESC LIMIT 10;"

# Debug specific user
psql -U postgres -d millionaire_game -c "SELECT * FROM users WHERE username='testuser';"
psql -U postgres -d millionaire_game -c "SELECT * FROM game_sessions WHERE user_id=(SELECT id FROM users WHERE username='testuser');"
```

#### Client Debugging
```bash
# Run client with terminal output
cd client
./bin/client 2>&1 | tee client.log

# Check if client can connect
telnet localhost 8080
# (Should connect if server running)

# Monitor network traffic (requires sudo)
sudo tcpdump -i lo0 -A port 8080
```

### 10.5 Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| Server won't start: "Port already in use" | `lsof -i :8080` to find process, then `kill <PID>` |
| Database connection failed | Check PostgreSQL running: `pg_isready`, verify credentials in `config.json` |
| GLFW build errors on macOS | Install CMake: `brew install cmake`, then `make rebuild-glfw` |
| Client window is blank | Check OpenGL support: `glxinfo \| grep OpenGL` (Linux) |
| "Cannot find libpq-fe.h" | Install PostgreSQL dev: `brew install postgresql` (macOS) or `apt install libpq-dev` (Ubuntu) |
| JSON parsing errors | Verify newline delimiter: All messages must end with `\n` |
| "No saved game found" | Check `saved_games` table: User must have saved game to RESUME |
| Lifeline not working | Verify question has lifeline data: `SELECT lifeline_5050_info FROM questions WHERE id=X;` |

---

## 11. Testing Strategy

### 11.1 Testing Levels

**All tests documented in `/database/TEST_GUIDE.md` have been passed ✅**

#### Level 1: Unit Testing (Manual)
- Test individual database methods
- Test protocol parsing functions
- Test scoring calculations

#### Level 2: Integration Testing (Manual)
- Test server + database interaction
- Test client + server communication
- Test complete request/response flows

#### Level 3: End-to-End Testing (Manual)
- Test complete game flows (register → login → start → answer → end)
- Test social features (add friend → accept → chat)
- Test admin operations (add question → start game → question appears)

### 11.2 Testing Tools

**Server Testing**:
```bash
# Use netcat (nc) to send raw JSON
nc localhost 8080
{"requestType":"LOGIN","data":{"username":"testuser","password":"TestPass123"}}

# Or use echo with pipe
echo '{"requestType":"PING","data":{"authToken":"abc123"}}' | nc localhost 8080
```

**Database Testing**:
```bash
# Direct SQL queries
psql -U postgres -d millionaire_game -c "SELECT * FROM users WHERE username='testuser';"

# Check game session after START request
psql -U postgres -d millionaire_game -c "SELECT * FROM game_sessions ORDER BY id DESC LIMIT 1;"
```

**Client Testing**:
- Use GUI directly
- Monitor server logs while clicking UI buttons
- Check database state after each action

### 11.3 Test Scenarios (From TEST_GUIDE.md)

**All scenarios below have been tested and passed ✅**

#### Test Case 1: User Registration & Login
```bash
# Register
echo '{"requestType":"REGISTER","data":{"username":"testuser1","password":"TestPass123"}}' | nc localhost 8080
# Expected: {"responseCode":201,"data":{"username":"testuser1","message":"Registration successful. Please login to continue."}}

# Login
echo '{"requestType":"LOGIN","data":{"username":"testuser1","password":"TestPass123"}}' | nc localhost 8080
# Expected: {"responseCode":200,"data":{"authToken":"...","username":"testuser1","role":"user","message":"Login successful"}}
```

#### Test Case 2: Game Flow (START → ANSWER → WIN/LOSE)
```bash
# 1. Start game
echo '{"requestType":"START","data":{"authToken":"YOUR_TOKEN"}}' | nc localhost 8080

# 2. Server sends GAME_START and QUESTION_INFO

# 3. Answer question 1 (check correct answer from DB first)
echo '{"requestType":"ANSWER","data":{"authToken":"YOUR_TOKEN","gameId":12345,"questionNumber":1,"answerIndex":1}}' | nc localhost 8080

# 4. Continue answering until win or lose
```

#### Test Case 3: Lifeline Usage
```bash
# Use 50/50
echo '{"requestType":"LIFELINE","data":{"authToken":"YOUR_TOKEN","gameId":12345,"questionNumber":1,"lifelineType":"5050"}}' | nc localhost 8080

# Try using same lifeline again (should fail)
echo '{"requestType":"LIFELINE","data":{"authToken":"YOUR_TOKEN","gameId":12345,"questionNumber":1,"lifelineType":"5050"}}' | nc localhost 8080
# Expected: {"responseCode":407,"error":"Lifeline already used"}
```

#### Test Case 4: Friend System
```bash
# Add friend
echo '{"requestType":"ADD_FRIEND","data":{"authToken":"YOUR_TOKEN","friendUsername":"testuser2"}}' | nc localhost 8080

# Accept friend (as testuser2)
echo '{"requestType":"ACCEPT_FRIEND","data":{"authToken":"USER2_TOKEN","friendUsername":"testuser1"}}' | nc localhost 8080

# Check friend list
echo '{"requestType":"FRIEND_STATUS","data":{"authToken":"YOUR_TOKEN"}}' | nc localhost 8080
```

#### Test Case 5: Admin Operations
```bash
# Add question (requires admin role)
echo '{"requestType":"ADD_QUES","data":{"authToken":"ADMIN_TOKEN","question":"What is 2+2?","options":["3","4","5","6"],"correctAnswer":1,"level":1}}' | nc localhost 8080

# View questions
echo '{"requestType":"VIEW_QUES","data":{"authToken":"ADMIN_TOKEN","page":1,"limit":10,"level":1}}' | nc localhost 8080

# Ban user
echo '{"requestType":"BAN_USER","data":{"authToken":"ADMIN_TOKEN","username":"testuser1","reason":"Test ban"}}' | nc localhost 8080
```

### 11.4 Test Data

**Mock data loaded from `/database/mock_data.sql`**:
- **Users**: `testuser1`, `testuser2`, `admin1` (role='admin')
- **Questions**: 45 questions total (15 easy, 15 medium, 15 hard)
- **Passwords**: All test accounts use password `TestPass123` (hashed in DB)

**Creating test admin account**:
```sql
-- Method 1: Update existing user
UPDATE users SET role = 'admin' WHERE username = 'testuser1';

-- Method 2: Create new admin
INSERT INTO users (username, password_hash, role) 
VALUES ('admin1', '<hashed_password>', 'admin');
```

---

## 12. Common Scenarios

### 12.1 Complete Game Playthrough

**Scenario**: User plays a complete game from start to finish

```
1. User opens client → CLIENT: STATE_DISCONNECTED
2. Client connects to server → SERVER: Accept connection, create session
3. Server sends CONNECTION notification → CLIENT: STATE_LOGIN
4. User enters credentials, clicks Login → CLIENT: Send LOGIN request
5. Server validates, returns authToken → CLIENT: Store token, STATE_MAIN_MENU
6. User clicks "Start Game" → CLIENT: Send START request
7. Server creates game session (gameId=100), assigns 15 questions
8. Server sends GAME_START notification → CLIENT: Update UI
9. Server sends QUESTION_INFO (Q1) → CLIENT: STATE_IN_GAME, display question, start timer
10. User clicks lifeline "50/50" → CLIENT: Send LIFELINE request
11. Server returns remainingOptions [0,2] → CLIENT: Gray out options B and D
12. User clicks answer "A" → CLIENT: Send ANSWER request
13. Server validates answer (correct), calculates points (15 - 5 lifeline = 10 points)
14. Server sends ANSWER response (correct=true) → CLIENT: Show "Correct!" animation
15. Server sends QUESTION_INFO (Q2) → CLIENT: Display next question
16. ...repeat steps 12-15 for questions 2-15...
17. User answers Q15 correctly → SERVER: Game ends, status='won'
18. Server sends ANSWER response (gameOver=true, isWinner=true)
19. Server sends GAME_END notification
20. Server updates leaderboard
21. Client shows victory screen → CLIENT: STATE_MAIN_MENU
22. User can start new game immediately
```

### 12.2 Disconnect and Resume

**Scenario**: User disconnects during game, then reconnects and resumes

```
1. User playing game at question 5 → SERVER: Game active, gameId=100
2. Connection lost (client closed, network error) → SERVER: Detect disconnect
3. Server auto-saves game state → DB: INSERT into saved_games (gameId, questionNumber=5, prize=10000000, score=100)
4. Server marks session as disconnected → SERVER: ClientSession removed
5. ...later...
6. User reopens client, logs in → CLIENT: Send LOGIN request
7. Server returns authToken (same user) → CLIENT: STATE_MAIN_MENU
8. User clicks "Resume Game" → CLIENT: Send RESUME request
9. Server checks saved_games table → DB: Find saved game for user
10. Server returns game state → CLIENT: {"questionNumber":5,"prize":10000000,"score":100}
11. Server sends QUESTION_INFO (Q5) → CLIENT: STATE_IN_GAME, display question
12. User continues playing from question 5
```

### 12.3 Friend Request Flow

**Scenario**: Player1 sends friend request to Player2, Player2 accepts

```
1. Player1 logs in → username='player1', authToken='token1'
2. Player2 logs in (separate client) → username='player2', authToken='token2'
3. Player1 clicks "Add Friend", enters "player2" → CLIENT1: Send ADD_FRIEND request
4. Server validates player2 exists → DB: SELECT id FROM users WHERE username='player2'
5. Server checks not already friends → DB: Check friendships table
6. Server creates friend request → DB: INSERT into friend_requests (from_user_id=1, to_user_id=2, status='pending')
7. Server returns success → CLIENT1: Show "Friend request sent"
8. Player2 clicks "Friend Requests" → CLIENT2: Send FRIEND_REQ_LIST request
9. Server queries pending requests → DB: SELECT from_user_id FROM friend_requests WHERE to_user_id=2 AND status='pending'
10. Server returns list → CLIENT2: Show "player1 sent you a friend request"
11. Player2 clicks "Accept" → CLIENT2: Send ACCEPT_FRIEND request
12. Server creates friendship → DB: INSERT into friendships (user1_id=1, user2_id=2)
13. Server updates request status → DB: UPDATE friend_requests SET status='accepted' (or DELETE)
14. Server returns success → CLIENT2: Show "You are now friends with player1"
15. Both players can now see each other in friend list
```

### 12.4 Admin Adding Question

**Scenario**: Admin adds a new question with lifeline data

```
1. Admin logs in → role='admin', authToken='adminToken'
2. Admin clicks "Admin Panel" → CLIENT: Show admin UI (only visible if role='admin')
3. Admin fills form:
   - Question: "What is 2+2?"
   - Option A: "3"
   - Option B: "4"
   - Option C: "5"
   - Option D: "6"
   - Correct Answer: 1 (B)
   - Level: 1
   - 50/50 info: [1, 2] (keep B and C)
   - Audience info: {"A": 10, "B": 70, "C": 15, "D": 5}
   - Phone info: "I'm 90% sure it's B"
4. Admin clicks "Add Question" → CLIENT: Send ADD_QUES request
5. Server validates admin role → AUTH: Check getUserRole(username) == 'admin'
6. Server validates data → Correct answer 0-3, level 1-15, 4 options
7. Server inserts question → DB: INSERT into questions (...) RETURNING id
8. Server returns success → CLIENT: Show "Question added successfully, ID: 500"
9. Question is now available for random selection in games
```

### 12.5 Error Handling Example

**Scenario**: User tries to answer when not in a game

```
1. User logs in → authToken='token123'
2. User sends ANSWER request (without starting game) → CLIENT: {"requestType":"ANSWER",...}
3. Server validates authToken → AUTH: Valid user
4. Server checks if user has active game → DB: SELECT id FROM game_sessions WHERE user_id=X AND status='active'
5. No active game found → SERVER: Return error
6. Server sends error response → CLIENT: {"responseCode":406,"error":"Not in a game"}
7. Client displays error message → CLIENT: Show modal "You must start a game first"
```

---

## Appendix

### A. Quick Reference: Key Constants

```cpp
// Game Constants
const int TOTAL_QUESTIONS = 15;
const int TIME_LIMIT_PER_QUESTION = 30;  // seconds
const int MAX_POINTS_PER_QUESTION = 30;
const int LIFELINE_PENALTY = 5;  // points
const int SAFE_CHECKPOINTS[] = {5, 10, 15};  // Question numbers

// Prize Structure (BIGINT values)
const long long PRIZES[] = {
    1000000,     // Q1: 1 million
    2000000,     // Q2: 2 million
    4000000,     // Q3: 4 million
    8000000,     // Q4: 8 million
    10000000,    // Q5: 10 million (safe checkpoint)
    20000000,    // Q6: 20 million
    40000000,    // Q7: 40 million
    60000000,    // Q8: 60 million
    80000000,    // Q9: 80 million
    100000000,   // Q10: 100 million (safe checkpoint)
    200000000,   // Q11: 200 million
    400000000,   // Q12: 400 million
    600000000,   // Q13: 600 million
    800000000,   // Q14: 800 million
    1000000000   // Q15: 1 billion (safe checkpoint, max prize)
};

// Lifeline Delays
const int LIFELINE_5050_DELAY = 5;      // seconds
const int LIFELINE_PHONE_DELAY = 10;    // seconds
const int LIFELINE_AUDIENCE_DELAY = 5;  // seconds

// Network
const int DEFAULT_PORT = 8080;
const char MESSAGE_DELIMITER = '\n';
const int AUTH_TOKEN_LENGTH = 32;  // hexadecimal characters

// Database
const char* DATABASE_NAME = "millionaire_game";
const int MAX_GAME_HISTORY = 20;  // games returned by VIEW_HISTORY
```

### B. Quick Reference: Database Table Sizes

| Table | Typical Row Count | Growth Rate |
|-------|-------------------|-------------|
| users | 10-1000 | Slow (new registrations) |
| questions | 45-500 | Slow (admin adds) |
| game_sessions | 100-100,000 | Fast (every game played) |
| game_questions | 1,500-1,500,000 | Fast (15 per game) |
| game_answers | 1,000-1,000,000 | Fast (variable per game) |
| saved_games | 0-100 | Slow (only active saved games) |
| friendships | 10-10,000 | Medium (social activity) |
| friend_requests | 0-100 | Fast (high churn, accepted/declined) |
| messages | 100-100,000 | Medium (chat activity) |
| leaderboard | 10-1000 | Slow (one row per user) |

### C. Protocol Quick Reference

**Most Common Requests**:
```json
// Login
{"requestType":"LOGIN","data":{"username":"player1","password":"Pass123"}}

// Start Game
{"requestType":"START","data":{"authToken":"abc123"}}

// Answer Question
{"requestType":"ANSWER","data":{"authToken":"abc123","gameId":100,"questionNumber":1,"answerIndex":1}}

// Use Lifeline
{"requestType":"LIFELINE","data":{"authToken":"abc123","gameId":100,"questionNumber":1,"lifelineType":"5050"}}

// Leaderboard
{"requestType":"LEADERBOARD","data":{"authToken":"abc123","type":"global","page":1,"limit":20}}
```

**Most Common Errors**:
- `400 INVALID_DATA`: Check JSON syntax, required fields
- `402 AUTH_ERROR`: Include valid authToken from LOGIN response
- `405 USER_ALREADY_IN_GAME`: Call GIVE_UP or wait for game to end before START
- `406 USER_NOT_IN_GAME`: Call START before ANSWER/LIFELINE
- `408 QUESTION_TIMEOUT`: Answer within 30 seconds
- `412 INVALID_GAME_STATE`: Ensure correct gameId, game must be active

---

## Summary

This document covers the complete technical implementation of the **Who Wants to Be a Millionaire** game project. It serves as the **source of truth** for:

✅ **Architecture**: Client-Server-Database layers with TCP socket communication  
✅ **Protocol**: 27 request types, JSON format, error codes  
✅ **Database**: 10-table schema with PostgreSQL  
✅ **Server**: Modular C++ handlers, session management, game logic  
✅ **Client**: ImGui GUI, socket client, protocol handling  
✅ **Testing**: All test cases passed (documented in TEST_GUIDE.md)  
✅ **Code Locations**: Where to find and modify every component  
✅ **Workflows**: How to add features, build, test, debug  

**For new developers**: Read sections 1-4 for overview, then jump to section 9 ("Where to Find & Modify Code") when implementing features.

**For testing**: Refer to section 11 and `/database/TEST_GUIDE.md`.

**For protocol questions**: Check `/docs/PROTOCOL.md` and `/docs/ERROR_CODES.md`.

**Current Status**: All core features implemented and tested. Ready for deployment or feature extensions.

---

**Last Updated**: 2026-01-08  
**Project Status**: ✅ All Tests Passed, Production Ready  
**Documentation Version**: 1.0

