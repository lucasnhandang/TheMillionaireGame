# Hướng Dẫn Chi Tiết Chạy Client và Server Trên Ubuntu

Hướng dẫn đầy đủ để setup và chạy toàn bộ project "Who Wants to be a Millionaire" trên Ubuntu/Linux.

---

## 📋 Mục Lục

1. [Yêu Cầu Hệ Thống](#yêu-cầu-hệ-thống)
2. [Cài Đặt Dependencies](#cài-đặt-dependencies)
3. [Setup Database](#setup-database)
4. [Build và Chạy Server](#build-và-chạy-server)
5. [Build và Chạy Client](#build-và-chạy-client)
6. [Cách Chạy Nhanh (Script Tự Động)](#cách-chạy-nhanh-script-tự-động)
7. [Kiểm Tra và Troubleshooting](#kiểm-tra-và-troubleshooting)
8. [Sử Dụng Game](#sử-dụng-game)

---

## 🔧 Yêu Cầu Hệ Thống

- **OS**: Ubuntu 18.04+ hoặc Linux distribution tương tự
- **Quyền**: Quyền sudo để cài đặt packages
- **Internet**: Kết nối internet để tải dependencies
- **RAM**: Tối thiểu 2GB (khuyến nghị 4GB+)
- **Disk**: Tối thiểu 1GB trống

---

## 📦 Cài Đặt Dependencies

### Bước 1: Cập nhật hệ thống

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### Bước 2: Cài đặt C++ Compiler và Build Tools

```bash
sudo apt-get install -y build-essential g++ make cmake
```

Kiểm tra:
```bash
g++ --version
make --version
cmake --version
```

### Bước 3: Cài đặt PostgreSQL Database

```bash
# Cài đặt PostgreSQL
sudo apt-get install -y postgresql postgresql-contrib libpq-dev

# Khởi động PostgreSQL service
sudo systemctl start postgresql
sudo systemctl enable postgresql

# Kiểm tra PostgreSQL đang chạy
sudo systemctl status postgresql
```

### Bước 4: Cài đặt OpenGL và X11 libraries (cho GUI client)

```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Bước 5: Cài đặt Python 3 (nếu client dùng Python)

```bash
sudo apt-get install -y python3 python3-pip python3-tk
```

Kiểm tra:
```bash
python3 --version
```

---

## 🗄️ Setup Database

### Bước 1: Tạo Database và User

```bash
# Chuyển sang user postgres
sudo -u postgres psql

# Trong PostgreSQL prompt, chạy các lệnh sau:
CREATE DATABASE millionaire_game;
CREATE USER game_user WITH PASSWORD 'game_password';
GRANT ALL PRIVILEGES ON DATABASE millionaire_game TO game_user;
\q
```

### Bước 2: Import Database Schema

```bash
# Quay lại terminal thường
cd /path/to/TheMillionaireGame/database

# Import schema
sudo -u postgres psql millionaire_game < schema.sql

# Hoặc nếu dùng user game_user:
psql -U game_user -d millionaire_game -h localhost < schema.sql
# (Nhập password: game_password)
```

### Bước 3: Kiểm tra Database

```bash
# Kiểm tra tables đã được tạo
psql -U game_user -d millionaire_game -h localhost -c "\dt"

# Xem danh sách users (nếu có)
psql -U game_user -d millionaire_game -h localhost -c "SELECT * FROM users;"
```

### Bước 4: (Tùy chọn) Import Mock Data

```bash
cd /path/to/TheMillionaireGame/database
psql -U game_user -d millionaire_game -h localhost < mock_data.sql
```

---

## 🚀 Build và Chạy Server

### Bước 1: Kiểm tra thư mục server

```bash
cd /path/to/TheMillionaireGame
ls -la server/
```

**Lưu ý**: Nếu không có thư mục `server/`, có thể server code nằm ở vị trí khác hoặc cần được tạo. Kiểm tra lại cấu trúc project.

### Bước 2: Tạo file config cho server

```bash
cd server

# Tạo config.json nếu chưa có
cat > config.json << EOF
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
  "db_password": "game_password"
}
EOF
```

### Bước 3: Build server

```bash
cd server

# Clean build trước
make clean

# Build
make
```

**Nếu build thành công**, bạn sẽ thấy:
```
g++ -std=c++11 -Wall -Wextra -pthread -g -c server.cpp -o obj/server.o
...
g++ -std=c++11 -Wall -Wextra -pthread -g -o bin/server ...
```

**Nếu có lỗi**, xem phần [Troubleshooting](#kiểm-tra-và-troubleshooting).

### Bước 4: Kiểm tra file executable

```bash
ls -lh server/bin/server
chmod +x server/bin/server
```

### Bước 5: Chạy server

**Cách 1: Chạy trực tiếp (foreground)**

```bash
cd server
./bin/server
```

**Cách 2: Chạy với port tùy chỉnh**

```bash
cd server
./bin/server -p 8080
```

**Cách 3: Chạy với config file**

```bash
cd server
./bin/server -c config.json
```

**Cách 4: Chạy ở background (khuyến nghị)**

```bash
cd server
nohup ./bin/server > server_output.log 2>&1 &
echo $! > server.pid  # Lưu PID để dễ dừng sau
```

### Bước 6: Kiểm tra server đang chạy

```bash
# Kiểm tra process
ps aux | grep server

# Kiểm tra port đang listen
sudo netstat -tuln | grep 8080
# hoặc
sudo ss -tuln | grep 8080

# Xem log (nếu chạy background)
tail -f server/server.log
# hoặc
tail -f server/server_output.log
```

**Kết quả mong đợi:**
```
tcp        0      0 0.0.0.0:8080            0.0.0.0:*               LISTEN
```

### Bước 7: Test server bằng telnet

```bash
# Cài telnet nếu chưa có
sudo apt-get install -y telnet

# Test kết nối
telnet localhost 8080
```

Nếu kết nối thành công, bạn sẽ thấy kết nối được thiết lập. Nhấn `Ctrl+]` rồi `quit` để thoát.

---

## 💻 Build và Chạy Client

### Phương án 1: Client C++ (nếu có)

#### Bước 1: Build GLFW (nếu cần)

```bash
cd glfw
mkdir -p build
cd build
cmake ..
make
cd ../..
```

#### Bước 2: Build client

```bash
cd client
make clean
make
```

Nếu build thành công, bạn sẽ thấy file `bin/client`.

#### Bước 3: Chạy client

```bash
cd client

# Chạy với server mặc định (localhost:8080)
./bin/client

# Hoặc kết nối đến server ở địa chỉ khác
./bin/client 192.168.1.100 8080
```

### Phương án 2: Client Python (nếu có main.py)

#### Bước 1: Kiểm tra file client

```bash
cd client
ls -la *.py
```

#### Bước 2: Chạy client Python

```bash
cd client
python3 main.py
```

---

## 🎯 Cách Chạy Nhanh (Script Tự Động)

### Sử dụng script run_all.sh

Script `run_all.sh` sẽ tự động:
- Kiểm tra và cài đặt dependencies
- Build server
- Khởi động server ở background
- Chạy client
- Tắt server khi client đóng

**Cách sử dụng:**

```bash
cd /path/to/TheMillionaireGame

# Cấp quyền thực thi
chmod +x run_all.sh

# Chạy script
./run_all.sh
```

**Lưu ý**: Script này giả định client là Python (`main.py`). Nếu client là C++, bạn cần chỉnh sửa script.

---

## 🔍 Kiểm Tra và Troubleshooting

### Kiểm tra Server

#### 1. Server không khởi động được

**Kiểm tra:**
```bash
# Xem log lỗi
cat server/server.log
# hoặc
cat server/server_output.log

# Kiểm tra port đã được sử dụng chưa
sudo lsof -i :8080
```

**Giải pháp:**
- Nếu port đã được sử dụng:
  ```bash
  # Tìm và kill process đang dùng port
  sudo fuser -k 8080/tcp
  
  # Hoặc chạy server ở port khác
  ./bin/server -p 8081
  ```

#### 2. Server không kết nối được database

**Kiểm tra:**
```bash
# Kiểm tra PostgreSQL đang chạy
sudo systemctl status postgresql

# Test kết nối database
psql -U game_user -d millionaire_game -h localhost
```

**Giải pháp:**
- Đảm bảo PostgreSQL đang chạy: `sudo systemctl start postgresql`
- Kiểm tra lại thông tin trong `config.json` (db_host, db_user, db_password)
- Kiểm tra file `pg_hba.conf` cho phép kết nối local:
  ```bash
  sudo nano /etc/postgresql/*/main/pg_hba.conf
  # Đảm bảo có dòng: local all all md5
  ```

#### 3. Lỗi "Permission denied"

```bash
chmod +x server/bin/server
chmod +x client/bin/client  # nếu có
```

### Kiểm tra Client

#### 1. Client không kết nối được server

**Kiểm tra:**
```bash
# Server đang chạy
ps aux | grep server

# Port đang listen
sudo netstat -tuln | grep 8080

# Firewall không chặn
sudo ufw status
sudo ufw allow 8080/tcp  # nếu cần
```

**Giải pháp:**
- Đảm bảo server đang chạy
- Kiểm tra địa chỉ IP và port đúng
- Kiểm tra firewall

#### 2. Lỗi "GL/gl.h: No such file or directory" (Client C++)

```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
```

#### 3. Lỗi "X11/Xlib.h: No such file or directory" (Client C++)

```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Lỗi Build

#### 1. "g++: command not found"

```bash
sudo apt-get install -y build-essential g++
```

#### 2. "make: command not found"

```bash
sudo apt-get install -y make
```

#### 3. "cmake: command not found"

```bash
sudo apt-get install -y cmake
```

#### 4. Lỗi thiếu thư viện PostgreSQL

```bash
sudo apt-get install -y libpq-dev
```

### Lỗi Database

#### 1. "database does not exist"

```bash
# Tạo lại database
sudo -u postgres createdb millionaire_game
psql -U game_user -d millionaire_game -h localhost < database/schema.sql
```

#### 2. "password authentication failed"

- Kiểm tra lại password trong `config.json`
- Hoặc reset password:
  ```bash
  sudo -u postgres psql
  ALTER USER game_user WITH PASSWORD 'new_password';
  ```

---

## 🎮 Sử Dụng Game

### Đăng ký tài khoản mới

1. Mở client
2. Chọn tab "Register"
3. Nhập username (ví dụ: `player1`)
4. Nhập password (tối thiểu 8 ký tự, có chữ hoa, chữ thường, số)
5. Nhấn "Register"

### Đăng nhập

1. Chọn tab "Login"
2. Nhập username và password
3. Nhấn "Login"
4. Lưu `authToken` từ response (nếu client tự động lưu thì không cần)

### Chơi Game

1. Sau khi đăng nhập, nhấn **"Start New Game"**
2. Đọc câu hỏi và chọn đáp án (A, B, C, hoặc D)
3. Nhấn **"Submit Answer"**
4. Sử dụng lifelines khi cần:
   - **50/50**: Loại bỏ 2 đáp án sai
   - **Phone a Friend**: Nhận gợi ý
   - **Ask the Audience**: Xem kết quả bình chọn
5. Trả lời đúng 15 câu để thắng!

### Các tính năng khác

- **Resume Game**: Tiếp tục game đã lưu (nếu disconnect)
- **Give Up**: Bỏ cuộc và nhận giải thưởng hiện tại
- **Leave Game**: Rời game (tự động lưu)
- **Leaderboard**: Xem bảng xếp hạng
- **Friends**: Quản lý bạn bè
- **History**: Xem lịch sử game

---

## 📊 Quản Lý Server

### Dừng Server

**Nếu chạy foreground:**
- Nhấn `Ctrl+C`

**Nếu chạy background:**
```bash
# Tìm PID
ps aux | grep server

# Kill process
kill <PID>

# Hoặc nếu đã lưu PID
kill $(cat server/server.pid)
```

### Xem Log Server

```bash
# Xem log real-time
tail -f server/server.log

# Xem 50 dòng cuối
tail -n 50 server/server.log

# Tìm lỗi
grep -i error server/server.log
```

### Restart Server

```bash
# Dừng server
kill $(cat server/server.pid)

# Chờ 2 giây
sleep 2

# Khởi động lại
cd server
nohup ./bin/server > server_output.log 2>&1 &
echo $! > server.pid
```

---

## 🔐 Firewall Configuration

Nếu chạy server trên máy khác trong mạng:

```bash
# Cho phép port 8080
sudo ufw allow 8080/tcp

# Kiểm tra
sudo ufw status

# Nếu firewall chưa được enable
sudo ufw enable
```

---

## ✅ Checklist Trước Khi Chạy

- [ ] Đã cài đặt g++, make, cmake
- [ ] Đã cài đặt PostgreSQL và libpq-dev
- [ ] Đã cài đặt Python 3 và tkinter (nếu client là Python)
- [ ] Đã cài đặt OpenGL và X11 libraries (nếu client là C++)
- [ ] Đã tạo database `millionaire_game`
- [ ] Đã import schema.sql vào database
- [ ] Đã tạo file `server/config.json` với thông tin database
- [ ] Đã build server thành công (`make` không lỗi)
- [ ] Server đang chạy và listen trên port 8080
- [ ] Firewall cho phép port 8080 (nếu cần)
- [ ] Client có thể kết nối đến server

---

## 📝 Ghi Chú Quan Trọng

1. **Port mặc định**: Server chạy trên port **8080** (hoặc **8888** tùy config)
2. **Database**: Cần PostgreSQL đang chạy trước khi start server
3. **Thread Safety**: Server hỗ trợ nhiều clients đồng thời
4. **Auto-save**: Game tự động lưu khi disconnect, có thể resume sau
5. **Protocol**: Tất cả communication theo format JSON, newline-delimited

---

## 📞 Hỗ Trợ

Nếu gặp vấn đề:

1. **Kiểm tra logs**: `tail -f server/server.log`
2. **Kiểm tra server đang chạy**: `ps aux | grep server`
3. **Kiểm tra port**: `netstat -tuln | grep 8080`
4. **Kiểm tra database**: `psql -U game_user -d millionaire_game -h localhost`
5. **Xem lại hướng dẫn troubleshooting ở trên**

---

## 📚 Tài Liệu Tham Khảo

- **Protocol Specification**: `docs/PROTOCOL.md`
- **Error Codes**: `docs/ERROR_CODES.md`
- **Database Guide**: `database/INTEGRATION_GUIDE.md`
- **Hướng dẫn nhanh**: `SETUP_UBUNTU.md`

---

**Chúc bạn chơi game vui vẻ! 🎉**

