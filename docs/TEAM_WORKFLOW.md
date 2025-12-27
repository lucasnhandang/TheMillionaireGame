# Team Workflow Guide

## Project Structure

```
TheMillionaireGame/
├── server/          # Server code (Member: Server Developer)
│   ├── server_core.h/cpp
│   ├── request_handlers/
│   └── ...
├── database/        # Database + Game Logic (Member A)
│   ├── schema.sql
│   ├── database.h/cpp
│   └── game_logic/
├── client/          # Client + GUI (Member B)
│   ├── src/
│   └── gui/
├── docs/            # Documentation
│   ├── PROTOCOL.md
│   ├── ERROR_CODES.md
│   └── TEAM_WORKFLOW.md
└── README.md        # Project overview
```

## Member Responsibilities

### Server Developer
- ✅ **COMPLETED**: Server architecture and protocol handlers
- ✅ **COMPLETED**: Request routing and session management
- ✅ **COMPLETED**: Authentication system
- **TODO**: Integrate database module when ready
- **TODO**: Integrate game logic modules when ready
- **TODO**: Testing and debugging

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

## Integration Points

### Server ↔ Database
- Server calls `Database::getInstance().methodName()`
- Database module provides all methods specified in `server/INTEGRATION.md`
- Integration points marked with `TODO: Replace with database call`

### Server ↔ Game Logic
- Server handlers call game logic modules
- Game logic modules use database for data
- Integration points in game handlers (START, ANSWER, LIFELINE)

### Client ↔ Server
- Client sends requests following `PROTOCOL.md`
- Server responds with format from `PROTOCOL.md`
- Error codes from `ERROR_CODES.md`

## Workflow Steps

### Phase 1: Database Integration (Member A)
1. Create database schema (`database/schema.sql`)
2. Implement `database.h/cpp` with all methods
3. Test database operations independently
4. Update server Makefile to include database module
5. Replace placeholders in server code
6. Integration testing with server

### Phase 2: Game Logic Integration (Member A)
1. Implement game logic modules
2. Test each module independently
3. Integrate with server handlers
4. Test complete game flow

### Phase 3: Client Development (Member B)
1. Implement client core (socket communication)
2. Implement protocol handlers
3. Create GUI framework
4. Implement game UI
5. Integration testing with server

### Phase 4: Integration Testing (All Members)
1. End-to-end testing
2. Bug fixes
3. Performance optimization
4. Documentation

## Communication Protocol

### Request Format (Client → Server)
```json
{
  "requestType": "LOGIN",
  "authToken": "token_here",  // Required for authenticated requests
  "data": {
    "username": "user",
    "password": "pass"
  }
}
```

### Response Format (Server → Client)
```json
{
  "responseCode": 200,
  "message": "Success",
  "data": { ... }
}
```

### Error Response
```json
{
  "responseCode": 401,
  "message": "Invalid credentials",
  "data": null
}
```

## Testing Checklist

### Server Testing
- [x] Server starts and accepts connections
- [x] All request types are handled
- [x] Authentication works
- [ ] Database integration works
- [ ] Game logic integration works

### Database Testing
- [ ] Database connection works
- [ ] All CRUD operations work
- [ ] Thread-safety verified
- [ ] Performance acceptable

### Client Testing
- [ ] Connection to server works
- [ ] All requests are sent correctly
- [ ] All responses are parsed correctly
- [ ] GUI is responsive
- [ ] Error handling works

### Integration Testing
- [ ] Complete game flow works
- [ ] Multiple clients can connect
- [ ] Leaderboard updates correctly
- [ ] Friend system works
- [ ] Admin functions work

## Git Workflow (Recommended)

1. **Main Branch**: Stable, working code
2. **Feature Branches**: 
   - `database-game-logic`
   - `client`
1. **Pull Requests**: Review before merging

## Common Issues & Solutions

### Issue: Database connection fails
- Check PostgreSQL is running
- Verify connection string in config.json
- Check firewall settings

### Issue: Client can't connect
- Verify server is running
- Check port number
- Verify network connectivity

### Issue: Protocol mismatch
- Review PROTOCOL.md
- Check JSON format
- Verify error codes

## Resources

- **Protocol Specification**: `docs/PROTOCOL.md`
- **Error Codes**: `docs/ERROR_CODES.md`
- **Server Integration**: `server/INTEGRATION.md`
- **Database Guide**: `database/INTEGRATION_GUIDE.md`
- **Client Guide**: `client/PROTOCOL_GUIDE.md`

