# Team Workflow Guide

## Project Structure

```
TheMillionaireGame/
├── server/          # Server code (Server Developer) - ✅ COMPLETED
│   ├── server_core.h/cpp         # Main entry point
│   ├── event_loop.h/cpp          # I/O multiplexing với poll()
│   ├── session_manager.h/cpp     # Session management
│   ├── auth_manager.h/cpp        # Authentication
│   ├── request_router.h/cpp      # Request routing
│   ├── stream_handler.h/cpp      # Stream utilities
│   ├── json_utils.h/cpp          # JSON parsing
│   ├── logger.h/cpp              # Logging system
│   ├── config.h/cpp              # Configuration
│   ├── notification_utils.h/cpp  # Notification handling
│   ├── game_state_manager.h/cpp  # Game state tracking
│   ├── request_handlers/         # Request handlers
│   │   ├── auth_handlers.h/cpp
│   │   ├── game_handlers.h/cpp
│   │   ├── social_handlers.h/cpp
│   │   ├── user_handlers.h/cpp
│   │   ├── admin_handlers.h/cpp
│   │   └── connection_handlers.h/cpp
│   ├── test/                     # Unit tests
│   ├── Makefile                  # Build configuration
│   └── bin/server                # Compiled binary
│
├── database/        # Database + Game Logic (Member A) - ⚠️ TODO
│   ├── schema.sql
│   ├── database.h/cpp
│   └── game_logic/
│
├── client/          # Client + GUI (Member B) - ⚠️ TODO
│   ├── src/
│   └── gui/
│
├── docs/            # Documentation
│   ├── PROTOCOL.md
│   ├── ERROR_CODES.md
│   ├── NOTIFICATION.md
│   ├── HANDOFF_GUIDE.md
│   └── TEAM_WORKFLOW.md
│
└── README.md        # Project overview
```

## Member Responsibilities

### Server Developer (✅ Đã Hoàn Thành)
- ✅ **Server Architecture**: Hybrid I/O multiplexing + worker thread pool
  - EventLoop với poll() cho concurrent connections
  - Worker thread pool cho blocking operations
  - Thread-safe message queuing và task scheduling
- ✅ **Protocol Handlers**: Tất cả 27 request types
  - auth_handlers: LOGIN, REGISTER, LOGOUT, CHANGE_PASSWORD
  - game_handlers: START, ANSWER, LIFELINE, ENDGAME, GETGAME
  - social_handlers: Friend management và friend leaderboard
  - user_handlers: User info, history, global leaderboard
  - admin_handlers: Question CRUD, ban/unban users
  - connection_handlers: PING, DISCONNECT
- ✅ **Session Management**: Thread-safe session tracking
- ✅ **Authentication System**: Token-based auth với AuthManager
- ✅ **Request Routing**: Automatic auth validation và routing
- ✅ **Error Handling**: Tất cả error codes từ ERROR_CODES.md
- ✅ **Logging System**: Comprehensive logging với levels
- ✅ **Configuration**: JSON-based config system
- ⚠️ **TODO**: Integrate database module when ready (replace TODO placeholders)
- ⚠️ **TODO**: Integration testing với client

### Member A: Database + Game Logic
- **Database Schema**: Design and implement PostgreSQL schema
- **Database Module**: Create `database.h/cpp` with all required methods
- **Game Logic Modules**: 
  - QuestionManager
  - GameStateManager
  - ScoringSystem
  - LifelineManager
  - GameTimer
- **Integration**: Replace all `TODO` placeholders in server code

### Member B: Client + GUI
- **Client Core**: TCP socket communication with server
- **Protocol Handler**: Implement all request/response types
- **GUI**: User interface for all game features
- **Game Flow**: Implement complete game flow logic

## Server Architecture Details

### Concurrent Server Model (Đã Implement)

**I/O Multiplexing (poll) + Worker Thread Pool**

- **Main Thread (EventLoop)**: Monitor sockets với `poll()`, accept connections, read/write non-blocking
- **Worker Threads** (4 threads): Process requests, validate auth, route handlers, database ops
- **Thread-safe**: Mutex cho message queue, task queue, client removal

**Ưu điểm:**
- ✅ Non-blocking I/O
- ✅ Scalable (1 thread → nhiều connections)
- ✅ Thread-safe operations
- ✅ Cross-platform (Linux & macOS)

### Thread Safety Requirements

**Database Module (Member A)**: PHẢI thread-safe - worker threads call đồng thời
**Client (Member B)**: Single-threaded OK - server đã handle concurrency

## Integration Points

### Server ↔ Database (Member A)
- **Context**: Worker threads call database (KHÔNG trên I/O thread)
- **Pattern**: `Database::getInstance().methodName()`
- **Thread Safety**: Database module PHẢI thread-safe
- **Integration**: Tìm `TODO: Replace with database call` trong request handlers

### Client ↔ Server (Member B)  
- **Protocol**: TCP socket, port 8888
- **Format**: JSON newline-delimited
- **Test**: `telnet localhost 8888` hoặc `nc localhost 8888`

## Workflow Steps

1. **Phase 1**: Database Integration (Member A) - Setup DB, implement methods, integrate server
2. **Phase 2**: Game Logic (Member A) - QuestionManager, GameStateManager, etc.
3. **Phase 3**: Client Dev (Member B) - Socket communication, protocol handler, GUI
4. **Phase 4**: Integration Testing (All) - E2E testing, bug fixes

## Communication Protocol

**Request Format:**
```json
{"requestType":"LOGIN","authToken":"token","data":{"username":"user","password":"pass"}}
```

**Response Format:**
```json
{"responseCode":200,"message":"Success","data":{...}}
```

## Testing Checklist

### Server (✅ Completed)
- [x] All 27 request types work
- [x] Authentication, session management
- [x] EventLoop + worker threads
- [ ] Database integration (Member A)

### Database Testing (Member A)
- [ ] Connection works
- [ ] CRUD operations correct
- [ ] Thread-safe với concurrent access

### Client Testing (Member B)
- [ ] Connect to server
- [ ] All 27 request types implemented
- [ ] GUI responsive
- [ ] Error handling

### Integration (All)
- [ ] Full game flow works
- [ ] Multiple concurrent clients
- [ ] Leaderboard, friend system
- [ ] No crashes

**Run server tests**: `cd server/test && make && ./run_all_tests.sh`

## Common Issues & Quick Fixes

| Issue | Quick Fix |
|-------|-----------|
| Database connection fails | `pg_isready` → Check PostgreSQL running |
| Client can't connect | `lsof -i :8888` → Verify server running |
| Protocol mismatch | Check JSON format ends with `\n` |
| Server performance slow | Check no blocking ops in I/O thread |

## Debug Commands

```bash
# Check server
ps aux | grep server && lsof -i :8888

# Check database  
pg_isready && psql millionaire_game -c "\dt"

# Test protocol
telnet localhost 8888
{"requestType":"PING","data":{}}

# Monitor logs
tail -f server/debug.log
```

## Resources

- **Protocol Specification**: `docs/PROTOCOL.md`
- **Error Codes**: `docs/ERROR_CODES.md`
- **Server Integration**: `server/INTEGRATION.md`
- **Database Guide**: `database/INTEGRATION_GUIDE.md`
- **Client Guide**: `client/PROTOCOL_GUIDE.md`