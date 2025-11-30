# Handoff Guide

## Tóm Tắt

**Server code đã hoàn thành 95%**
- ✅ Tất cả 27 protocol request types đã được implement
- ✅ Tất cả error codes từ ERROR_CODES.md đã được implement (với placeholders cho database)
- ✅ Code đã được refactor thành modules rõ ràng, dễ bảo trì
- ✅ Tất cả validations theo PROTOCOL.md đã được implement
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
    ├── PROTOCOL.md
    ├── ERROR_CODES.md
    └── TEAM_WORKFLOW.md
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
- Server code đã có sẵn các điểm tích hợp
- Tìm `TODO: Replace with database call` trong:
  - `server/request_handlers/auth_handlers.cpp`
  - `server/request_handlers/game_handlers.cpp`
  - `server/request_handlers/social_handlers.cpp`
  - `server/request_handlers/user_handlers.cpp`
  - `server/request_handlers/admin_handlers.cpp`

### Client ↔ Server (Member B)
- Server đã implement đầy đủ protocol
- Client chỉ cần follow `PROTOCOL.md`
- Server sẽ respond theo format đã định nghĩa

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

