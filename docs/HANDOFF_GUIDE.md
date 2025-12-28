# Handoff Guide

## Tóm Tắt

**Server code đã hoàn thành 95%**
- ✅ Tất cả 27 protocol request types đã được implement
- ✅ Tất cả error codes từ ERROR_CODES.md đã được implement (với placeholders cho database)
- ✅ Code đã được refactor thành modules rõ ràng, dễ bảo trì
- ✅ Tất cả validations theo PROTOCOL.md đã được implement
- ✅ Kiến trúc concurrent server với I/O multiplexing và worker thread pool
- ⚠️ Còn 5%: Các validations cần database (404, 409 checks) - sẽ được hoàn thành khi tích hợp database

## Cấu Trúc Project

```
TheMillionaireGame/
├── server/
│   └── Tất cả code server đã xong
│
├── database/
│   ├── schema.sql              # Database schema (đã có template)
│   ├── database.h/cpp          # Database module (cần implement)
│   └── game_logic/             # Game logic modules (cần implement)
│
├── client/
│   ├── src/                    # Client code (cần implement)
│   └── gui/                    # GUI code (cần implement)
│
└── docs/           Tài liệu đầy đủ
```

## Giao Việc Cho Member A (Database + Game Logic)

### File Cần Đọc Trước:
1. `server/INTEGRATION.md` - Hướng dẫn tích hợp database
2. `database/README.md` - Tổng quan về database module
3. `database/INTEGRATION_GUIDE.md` - Chi tiết integration steps
4. `database/schema.sql` - Database schema template

### Công Việc Cần Làm:

#### 1. Database Module (`database/database.h/cpp`)
- Implement tất cả methods trong `database.h.template`
- Xem danh sách methods trong `server/INTEGRATION.md`
- Sử dụng PostgreSQL với `libpq`

#### 2. Game Logic Modules (`database/game_logic/`)
- **QuestionManager**: Lấy câu hỏi ngẫu nhiên, kiểm tra đáp án
- **GameStateManager**: Quản lý trạng thái game
- **ScoringSystem**: Tính điểm
- **LifelineManager**: Xử lý các lifeline
- **GameTimer**: Quản lý thời gian

#### 3. Tích Hợp Vào Server
- Tìm tất cả `TODO: Replace with database call` trong server code
- Thay thế bằng `Database::getInstance().methodName()`
- Update `server/Makefile` để include database files

### Checklist Cho Member A:
- [ ] Đọc tất cả tài liệu trong `database/` và `server/INTEGRATION.md`
- [ ] Tạo database PostgreSQL và chạy `schema.sql`
- [ ] Implement `database.h/cpp` với tất cả methods
- [ ] Test database connection và các operations
- [ ] Implement game logic modules
- [ ] Tích hợp vào server code
- [ ] Test end-to-end với server

## Giao Việc Cho Member B (Client + GUI)

### File Cần Đọc Trước:
1. `docs/PROTOCOL.md` - Protocol specification
2. `docs/ERROR_CODES.md` - Error codes
3. `client/README.md` - Client development guide

### Công Việc Cần Làm:

#### 1. Client Core (`client/src/client_core.h/cpp`)
- TCP socket connection đến server
- Gửi/nhận messages
- Connection management

#### 2. Protocol Handler (`client/src/protocol_handler.h/cpp`)
- Build requests theo format trong `PROTOCOL.md`
- Parse responses từ server
- Handle error codes từ `ERROR_CODES.md`

#### 3. GUI (`client/src/gui/`)
- **Login/Register Screen**: Đăng nhập, đăng ký
- **Main Menu**: Menu chính sau khi login
- **Game Screen**: 
  - Hiển thị câu hỏi
  - 4 đáp án
  - Timer
  - Lifeline buttons
  - Current prize
- **User History Screen**
- **User Info Screen**: change pass,...
- **Leaderboard Screen**: Bảng xếp hạng
- **Friend Management**: Quản lý bạn bè
- **Admin Management Screen**: for admin (CRUD,...)

#### 4. Game Flow Logic
- Implement complete game flow
- Handle tất cả request types
- Update UI dựa trên server responses

### Checklist Cho Member B:
- [ ] Đọc `PROTOCOL.md` và `ERROR_CODES.md`
- [ ] Implement client core (socket communication)
- [ ] Implement protocol handler
- [ ] Tạo GUI framework
- [ ] Implement các screens
- [ ] Implement game flow logic
- [ ] Test với server
- [ ] Error handling và user feedback

## Integration Points

### Server ↔ Database (Member A)
- **Kiến trúc**: Database calls được thực hiện trong worker threads (không block I/O thread)
- **Thread Safety**: Database module PHẢI thread-safe (worker threads có thể call đồng thời)
- **Integration Points**: Tìm `TODO: Replace with database call` trong:
  - `server/request_handlers/auth_handlers.cpp` (user authentication, registration)
  - `server/request_handlers/game_handlers.cpp` (game state, questions, scoring)
  - `server/request_handlers/social_handlers.cpp` (friends, friend leaderboard)
  - `server/request_handlers/user_handlers.cpp` (user info, history, global leaderboard)
  - `server/request_handlers/admin_handlers.cpp` (CRUD questions, ban/unban users)
- **Cách tích hợp**:
  ```cpp
  // Example trong handler
  Database& db = Database::getInstance();
  bool success = db.validateUser(username, password);
  ```

### Client ↔ Server (Member B)
- **Protocol**: Server đã implement đầy đủ protocol theo `PROTOCOL.md`
- **Connection**: Client connect qua TCP socket đến server port (default: 8888)
- **Message Format**: JSON messages, newline-delimited (`\n`)
- **Response Format**: Server respond theo format trong `PROTOCOL.md`
- **Concurrency**: Server có thể handle multiple clients đồng thời
- **Testing**: Có thể dùng telnet hoặc netcat để test:
  ```bash
  telnet localhost 8888
  # Gửi: {"requestType":"REGISTER","data":{"username":"test","password":"pass123"}}
  ```

## Testing Strategy

### Member A Testing:
1. Test database module độc lập
2. Test game logic modules độc lập
3. Integration test với server handlers
4. End-to-end test với client (khi client sẵn sàng)

### Member B Testing:
1. Test client core connection
2. Test protocol handler với mock responses
3. Test GUI components
4. Integration test với server
5. End-to-end game flow test

## Communication

### Khi Member A Hoàn Thành Database:
- Update `server/Makefile` để include database files
- Test với server handlers
- Báo cho bạn để review và test

### Khi Member B Hoàn Thành Client:
- Test connection với server
- Test tất cả request types
- Báo cho bạn để integration test

## Chiến Lược Testing (Project Môn Học)

### Mục Tiêu Testing

**Cho project môn Lập trình mạng, cần chứng minh:**
1. ✅ Server xử lý concurrent connections (poll + threads)
2. ✅ Protocol implementation đúng spec
3. ✅ Database integration hoạt động
4. ✅ Client-server communication work
5. ✅ Game flow hoàn chỉnh từ đầu đến cuối

---

### Phase 1: Testing Riêng Lẻ

#### Member A (Database):
```bash
# 1. Setup & test database
createdb millionaire_game
psql millionaire_game < database/schema.sql

# 2. Test basic operations
psql millionaire_game -c "SELECT * FROM users;"
psql millionaire_game -c "SELECT * FROM questions LIMIT 5;"

# 3. Compile server với database
cd server && make clean && make

# 4. Run server
./bin/server
```

**Quick Test với netcat:**
```bash
# Test REGISTER
echo '{"requestType":"REGISTER","data":{"username":"test","password":"123"}}' | nc localhost 8888

# Test LOGIN (copy token từ response)
echo '{"requestType":"LOGIN","data":{"username":"test","password":"123"}}' | nc localhost 8888

# Test với token
echo '{"requestType":"GETUSERINFO","authToken":"TOKEN_HERE","data":{}}' | nc localhost 8888
```

#### Member B (Client):
```bash
# 1. Test connection
telnet localhost 8888

# 2. Build & run client
cd client && make
./client

# 3. Test basic flow trong GUI
# Register → Login → Start Game → Answer → View Score
```

---

### Phase 2: Integration Testing (Cả Team)

**Test Environment:**
```bash
# Terminal 1: Database
pg_ctl -D /usr/local/var/postgres start

# Terminal 2: Server với debug log
cd server && ./bin/server -l debug.log

# Terminal 3: Monitor logs
tail -f debug.log

# Terminal 4-5: Clients
cd client && ./client
```

**3 Test Cases Chính (Demo cho giáo viên):**

**Test 1: Complete Game Flow**
```
Client 1:
1. Register (username="player1", password="pass123")
2. Login → nhận authToken
3. Start game → nhận câu hỏi đầu tiên
4. Answer 15 câu hỏi (đúng/sai mixed, dùng lifeline)
5. End game → xem điểm số
6. View leaderboard → thấy tên mình
7. Logout

✅ Chứng minh: Full game flow + database persistence
```

**Test 2: Concurrent Players (Chứng minh concurrent server)**
```
Client 1 & Client 2 chạy đồng thời:
- Client 1: player1 đang chơi game (câu 5/15)
- Client 2: player2 bắt đầu game mới (câu 1/15)
- Cả 2 answer cùng lúc
- Client 3: player3 xem leaderboard (thấy cả 2 đang chơi)

✅ Chứng minh: Server handle multiple clients, no blocking, độc lập
```

**Test 3: Friend System + Error Handling**
```
Client 1: player1 gửi friend request cho player2
Client 2: player2 accept request
Both: Xem friend leaderboard

Test errors:
- Invalid JSON → 400
- Wrong password → 401  
- Duplicate username → 409
- User not found → 404

✅ Chứng minh: Social features + proper error handling
```

---

### Phase 3: Performance Test (Optional - Bonus điểm)

```bash
# Test với 10-20 concurrent clients (đủ cho demo)
for i in {1..10}; do
  ./client &
done

# Monitor
top -p $(pgrep server)
ps aux | grep server  # Check thread count
psql millionaire_game -c "SELECT count(*) FROM pg_stat_activity;"
```

**Performance Goals (Realistic cho môn học):**
- ✅ 10+ concurrent clients work smoothly
- ✅ No crashes under normal usage
- ✅ Response time acceptable (< 1s)
- ✅ Memory stable (no obvious leaks)

---

## Demo Checklist (Cho Presentation)

### Member A (Database) phải demo:
- [ ] Database có data (users, questions, game history)
- [ ] Server connect thành công với database
- [ ] CRUD operations work (add user, add question, etc.)
- [ ] Thread safety: 2+ clients cùng query database không crash

### Member B (Client) phải demo:
- [ ] Client connect server thành công
- [ ] GUI hiển thị đẹp, dễ dùng
- [ ] Full game flow: Register → Game → Score → Leaderboard
- [ ] Error messages hiển thị rõ ràng

### Integration (Cả team) phải demo:
- [ ] 2-3 clients chạy đồng thời không conflict
- [ ] Data persist sau khi restart server
- [ ] Game logic đúng (scoring, lifelines, questions)

---

## Quick Debug Commands

```bash
# Check server running
ps aux | grep server
lsof -i :8888

# Check database
pg_isready
psql millionaire_game -c "\dt"  # List tables
psql millionaire_game -c "SELECT * FROM users;"

# Check logs
tail -f server/debug.log
tail -f /usr/local/var/postgres/server.log

# Test protocol manually
telnet localhost 8888
{"requestType":"PING","data":{}}

# Check memory leaks (if needed)
valgrind --leak-check=yes ./bin/server
```

## Lưu Ý Quan Trọng

1. **Protocol**: Tất cả communication phải follow `PROTOCOL.md`
2. **Error Codes**: Handle tất cả error codes trong `ERROR_CODES.md`
3. **Thread Safety**: Database operations phải thread-safe
4. **Error Handling**: Handle tất cả edge cases
5. **Testing**: Test kỹ trước khi integration

## Resources

- **Protocol**: `docs/PROTOCOL.md`
- **Error Codes**: `docs/ERROR_CODES.md`
- **Server Integration**: `server/INTEGRATION.md`
- **Database Guide**: `database/INTEGRATION_GUIDE.md`
- **Client Guide**: `client/README.md`
- **Team Workflow**: `docs/TEAM_WORKFLOW.md`

## Questions?

Nếu có thắc mắc về:
- **Server code**: Hỏi bạn (Server Developer)
- **Database/Game Logic**: Xem `database/INTEGRATION_GUIDE.md`
- **Client/GUI**: Xem `client/README.md`
- **Protocol**: Xem `docs/PROTOCOL.md`

