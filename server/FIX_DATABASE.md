# Hướng dẫn Sửa Lỗi Database Connection

## Vấn đề: "Database not connected"

Từ log, server không kết nối được database. Hãy làm theo các bước sau:

## Bước 1: Kiểm tra PostgreSQL

```bash
# Kiểm tra PostgreSQL có chạy không
sudo systemctl status postgresql

# Nếu chưa chạy, start nó
sudo systemctl start postgresql
```

## Bước 2: Kiểm tra Database

```bash
# Chạy script kiểm tra
cd server
chmod +x check_database_connection.sh
./check_database_connection.sh
```

## Bước 3: Tạo Database (nếu chưa có)

```bash
cd database
sudo -u postgres createdb millionaire_game
sudo -u postgres psql millionaire_game < schema.sql
```

## Bước 4: Thêm Câu Hỏi

```bash
cd database
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

## Bước 5: Tạo Config File

Đảm bảo có file `server/config.json`:

```bash
cd server
cat config.json
```

Nếu không có, file đã được tạo tự động với nội dung:
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

## Bước 6: Restart Server

```bash
cd server
make clean
make
./bin/server
```

Bạn sẽ thấy log:
- `[INFO] Database connected successfully` - Nếu kết nối thành công
- `[WARNING] Failed to connect to database...` - Nếu vẫn lỗi

## Troubleshooting

### Lỗi "connection refused"

PostgreSQL không chạy hoặc không lắng nghe trên port 5432:

```bash
# Start PostgreSQL
sudo systemctl start postgresql

# Kiểm tra port
sudo netstat -tlnp | grep 5432
```

### Lỗi "database does not exist"

```bash
sudo -u postgres createdb millionaire_game
```

### Lỗi "authentication failed"

Kiểm tra password trong `config.json`. Nếu PostgreSQL yêu cầu password, cần set:

```json
"db_password": "your_password"
```

Hoặc cấu hình PostgreSQL để cho phép local connection không cần password (sửa `/etc/postgresql/*/main/pg_hba.conf`).

### Không có câu hỏi

```bash
# Kiểm tra
psql -U postgres millionaire_game -c "SELECT COUNT(*) FROM questions;"

# Thêm câu hỏi
cd database
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

## Sau khi sửa

Restart server và kiểm tra log:
- Phải thấy `Database connected successfully`
- Không còn `Database not connected` error
- Khi start game, phải thấy câu hỏi

