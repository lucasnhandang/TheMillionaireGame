# Database Integration Guide

## Tổng quan

Database đã được tích hợp vào server để load questions từ PostgreSQL database.

## Cấu hình Database

Thêm vào `config.json`:

```json
{
  "port": 8080,
  "log_file": "server.log",
  "log_level": "INFO",
  "max_clients": 100,
  "ping_timeout_seconds": 60,
  "connection_timeout_seconds": 300,
  "worker_threads": 4,
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "postgres",
  "db_password": ""
}
```

## Build Server với Database

```bash
cd server
make clean
make
```

**Lưu ý:** Cần cài đặt PostgreSQL development libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libpq-dev

# macOS
brew install postgresql
```

## Database Module

### Files đã tạo:
- `server/database.h` - Database interface
- `server/database.cpp` - Database implementation với PostgreSQL (libpq)

### Methods đã implement:
- `connect()` - Kết nối database
- `getRandomQuestion(int level)` - Lấy câu hỏi ngẫu nhiên cho level
- `getQuestions(int level, int limit)` - Lấy danh sách câu hỏi
- `questionExists(int question_id)` - Kiểm tra câu hỏi tồn tại
- `authenticateUser()` - Xác thực user (basic)
- `registerUser()` - Đăng ký user (basic)
- `getUserRole()` - Lấy role của user
- `userExists()` - Kiểm tra user tồn tại

## Tích hợp vào Game Handlers

### Đã thay thế TODO trong:
1. **handleStart()** - Load và gửi câu hỏi đầu tiên
2. **handleAnswer()** - Check answer từ database và load câu hỏi tiếp theo
3. **handleResume()** - Load câu hỏi khi resume game

### Helper function:
- `buildQuestionInfoData()` - Build JSON data cho QUESTION_INFO notification

## Kiểm tra

1. **Đảm bảo database đã setup:**
   ```bash
   cd database
   ./setup_database.sh
   ```

2. **Kiểm tra questions trong database:**
   ```bash
   psql -U postgres millionaire_game -c "SELECT level, COUNT(*) FROM questions GROUP BY level ORDER BY level;"
   ```

3. **Chạy server:**
   ```bash
   cd server
   ./bin/server
   ```

4. **Test với client:**
   - Đăng nhập/đăng ký
   - Bắt đầu game mới
   - Câu hỏi sẽ được load từ database và hiển thị

## Troubleshooting

### Lỗi "Database not connected"
- Kiểm tra PostgreSQL đang chạy: `sudo systemctl status postgresql`
- Kiểm tra config.json có đúng thông tin database không
- Kiểm tra database đã được tạo: `psql -U postgres -l | grep millionaire_game`

### Lỗi "No question found for level X"
- Kiểm tra có câu hỏi trong database: `psql -U postgres millionaire_game -c "SELECT * FROM questions WHERE level = X;"`
- Thêm câu hỏi: `psql -U postgres millionaire_game < database/add_sample_questions.sql`

### Lỗi compile "libpq-fe.h: No such file"
- Cài đặt libpq-dev: `sudo apt-get install libpq-dev`

## Next Steps

Các tính năng còn lại cần tích hợp:
- User authentication với password hashing
- Game session persistence
- Leaderboard
- Friends system
- Admin operations (add/edit/delete questions)

