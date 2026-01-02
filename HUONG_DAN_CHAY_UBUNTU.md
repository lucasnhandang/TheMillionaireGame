# Hướng Dẫn Chạy Project Trên Ubuntu

Hướng dẫn chi tiết để chạy toàn bộ project "Who Wants to be a Millionaire" trên Ubuntu/Linux.

## 📋 Mục Lục

1. [Yêu Cầu Hệ Thống](#yêu-cầu-hệ-thống)
2. [Cài Đặt Dependencies](#cài-đặt-dependencies)
3. [Cấu Hình Server](#cấu-hình-server)
4. [Build và Chạy Server](#build-và-chạy-server)
5. [Chạy Client](#chạy-client)
6. [Kiểm Tra Kết Nối](#kiểm-tra-kết-nối)
7. [Troubleshooting](#troubleshooting)

---

## 🔧 Yêu Cầu Hệ Thống

- Ubuntu 18.04+ hoặc Linux distribution tương tự
- Quyền sudo để cài đặt packages
- Kết nối internet để tải dependencies

---

## 📦 Cài Đặt Dependencies

### Bước 1: Cập nhật hệ thống

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### Bước 2: Cài đặt C++ Compiler và Build Tools

```bash
sudo apt-get install -y build-essential g++ make
```

Kiểm tra:
```bash
g++ --version
make --version
```

### Bước 3: Cài đặt OpenGL và X11 libraries (cho GUI)

```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Bước 4: Cài đặt CMake (để build GLFW nếu cần)

```bash
sudo apt-get install -y cmake
```

### Bước 5: (Tùy chọn) Cài đặt PostgreSQL

**Lưu ý**: Server hiện tại có thể chạy mà không cần database (sử dụng placeholders). Nếu muốn test với database thật:

```bash
sudo apt-get install -y postgresql postgresql-contrib
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

---

## ⚙️ Cấu Hình Server

### Bước 1: Tạo file config cho server

```bash
cd server
cp config.json.example config.json
```

### Bước 2: Chỉnh sửa config.json (nếu cần)

Mở file `config.json`:

```bash
nano config.json
```

Nội dung mẫu:
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

Lưu và thoát (Ctrl+X, Y, Enter nếu dùng nano).

---

## 🚀 Build và Chạy Server

### Bước 1: Build server

```bash
cd server
make clean
make
```

Nếu build thành công, bạn sẽ thấy:
```
g++ -std=c++11 -Wall -Wextra -pthread -g -c server.cpp -o obj/server.o
...
g++ -std=c++11 -Wall -Wextra -pthread -g -o bin/server ...
```

### Bước 2: Kiểm tra file executable

```bash
ls -lh bin/server
```

File `bin/server` phải tồn tại và có quyền thực thi.

### Bước 3: Chạy server

**Cách 1: Chạy với config mặc định**
```bash
cd server
./bin/server
```

**Cách 2: Chạy với port tùy chỉnh**
```bash
cd server
./bin/server -p 8080
```

**Cách 3: Chạy với config file tùy chỉnh**
```bash
cd server
./bin/server -c config.json
```

**Cách 4: Chạy ở background (khuyến nghị)**
```bash
cd server
nohup ./bin/server > server_output.log 2>&1 &
```

Kiểm tra server đang chạy:
```bash
ps aux | grep server
netstat -tuln | grep 8080
```

### Bước 4: Xem log server (nếu chạy background)

```bash
tail -f server/server.log
# hoặc
tail -f server/server_output.log
```

---

## 💻 Chạy Client

### Bước 1: Build GLFW (nếu chưa có)

```bash
cd glfw
cmake .
make
cd ..
```

### Bước 2: Build client

```bash
cd client
make clean
make
```

Nếu build thành công, bạn sẽ thấy file `bin/client`.

### Bước 3: Chạy client

**Cách 1: Chạy với server mặc định (localhost:8080)**
```bash
cd client
./bin/client
```

**Cách 2: Kết nối đến server ở địa chỉ khác**
```bash
cd client
./bin/client 192.168.1.100 8080
```

**Cách 3: Sử dụng make run**
```bash
cd client
make run
```

### Bước 3: Sử dụng client

1. **Đăng ký tài khoản mới**:
   - Chọn tab "Register"
   - Nhập username (ví dụ: `player1`)
   - Nhập password (tối thiểu 8 ký tự, có chữ hoa, chữ thường, số)
   - Nhấn "Register"

2. **Đăng nhập**:
   - Chọn tab "Login"
   - Nhập username và password
   - Nhấn "Login"

3. **Chơi game**:
   - Nhấn "Start New Game"
   - Chọn đáp án và nhấn "Submit Answer"
   - Sử dụng lifelines khi cần

---

## 🔍 Kiểm Tra Kết Nối

### Kiểm tra server đang listen

```bash
# Kiểm tra port 8080
sudo netstat -tuln | grep 8080
# hoặc
sudo ss -tuln | grep 8080
```

Kết quả mong đợi:
```
tcp        0      0 0.0.0.0:8080            0.0.0.0:*               LISTEN
```

### Test kết nối bằng telnet

```bash
sudo apt-get install -y telnet
telnet localhost 8080
```

Nếu kết nối thành công, bạn sẽ thấy kết nối được thiết lập.

### Test bằng curl (nếu server hỗ trợ HTTP)

```bash
curl -v telnet://localhost:8080
```

---

## 🛠️ Troubleshooting

### Lỗi: "Connection refused"

**Nguyên nhân**: Server chưa chạy hoặc đang chạy ở port khác.

**Giải pháp**:
```bash
# Kiểm tra server có đang chạy không
ps aux | grep server

# Kiểm tra port
netstat -tuln | grep 8080

# Khởi động lại server
cd server
./bin/server
```

### Lỗi: "Permission denied" khi chạy server

**Giải pháp**:
```bash
chmod +x server/bin/server
```

### Lỗi: "Address already in use"

**Nguyên nhân**: Port 8080 đã được sử dụng bởi process khác.

**Giải pháp**:
```bash
# Tìm process đang dùng port 8080
sudo lsof -i :8080
# hoặc
sudo fuser -k 8080/tcp

# Hoặc chạy server ở port khác
./bin/server -p 8081
```

### Lỗi: "GL/gl.h: No such file or directory"

**Giải pháp**:
```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
```

### Lỗi: "X11/Xlib.h: No such file or directory"

**Giải pháp**:
```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Lỗi: "g++: command not found"

**Giải pháp**:
```bash
sudo apt-get install -y build-essential g++
```

### Lỗi: Build server thất bại

**Kiểm tra**:
1. Tất cả file .cpp và .h có trong thư mục server
2. Makefile đúng format
3. Compiler version: `g++ --version` (cần C++11 support)

**Thử build lại**:
```bash
cd server
make clean
make
```

### Client không kết nối được đến server

**Kiểm tra**:
1. Server đang chạy: `ps aux | grep server`
2. Firewall không chặn port 8080:
```bash
sudo ufw allow 8080
sudo ufw status
```
3. Server và client cùng network (hoặc dùng localhost)
4. Địa chỉ IP đúng: `ip addr show` hoặc `hostname -I`

---

## 📝 Script Tự Động Hóa

### Script chạy server (server/start_server.sh)

Tạo file `server/start_server.sh`:

```bash
#!/bin/bash
cd "$(dirname "$0")"

echo "Building server..."
make clean
make

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Starting server on port 8080..."
    ./bin/server
else
    echo "Build failed!"
    exit 1
fi
```

Chạy:
```bash
chmod +x server/start_server.sh
./server/start_server.sh
```

### Script chạy cả server và client (run_all.sh)

Tạo file `run_all.sh` ở thư mục gốc:

```bash
#!/bin/bash

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Who Wants to be a Millionaire - Setup ===${NC}"

# Check dependencies
echo -e "${GREEN}Checking dependencies...${NC}"
command -v g++ >/dev/null 2>&1 || { echo "g++ not found. Installing..."; sudo apt-get install -y build-essential g++; }
command -v python3 >/dev/null 2>&1 || { echo "python3 not found. Installing..."; sudo apt-get install -y python3; }
python3 -c "import tkinter" 2>/dev/null || { echo "tkinter not found. Installing..."; sudo apt-get install -y python3-tk; }

# Build server
echo -e "${GREEN}Building server...${NC}"
cd server
make clean
make
if [ $? -ne 0 ]; then
    echo "Server build failed!"
    exit 1
fi

# Start server in background
echo -e "${GREEN}Starting server in background...${NC}"
./bin/server > server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Check if server started
if ! ps -p $SERVER_PID > /dev/null; then
    echo "Server failed to start!"
    exit 1
fi

# Start client
echo -e "${GREEN}Starting client...${NC}"
cd ../client
python3 main.py

# Cleanup
echo -e "${BLUE}Shutting down server...${NC}"
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "${GREEN}Done!${NC}"
```

Chạy:
```bash
chmod +x run_all.sh
./run_all.sh
```

---

## 🎮 Hướng Dẫn Sử Dụng Game

### Đăng ký và Đăng nhập

1. Mở client, chọn tab "Register"
2. Nhập username (ví dụ: `testuser`)
3. Nhập password (ví dụ: `Test1234` - có chữ hoa, chữ thường, số)
4. Nhấn "Register"
5. Chuyển sang tab "Login" và đăng nhập

### Chơi Game

1. Sau khi đăng nhập, nhấn "Start New Game"
2. Đọc câu hỏi và chọn đáp án (A, B, C, hoặc D)
3. Nhấn "Submit Answer"
4. Sử dụng lifelines:
   - **50/50**: Loại bỏ 2 đáp án sai
   - **Phone a Friend**: Nhận gợi ý
   - **Ask the Audience**: Xem kết quả bình chọn
5. Trả lời đúng 15 câu để thắng!

### Các tính năng khác

- **Resume Game**: Tiếp tục game đã lưu
- **Give Up**: Bỏ cuộc và nhận giải thưởng hiện tại
- **Leave Game**: Rời game (tự động lưu)

---

## 📊 Kiểm Tra Logs

### Server logs

```bash
# Xem log real-time
tail -f server/server.log

# Xem 50 dòng cuối
tail -n 50 server/server.log

# Tìm lỗi
grep -i error server/server.log
```

### Client output

Client hiển thị output trên terminal khi chạy. Nếu có lỗi, sẽ hiển thị trực tiếp.

---

## 🔐 Firewall Configuration

Nếu chạy server trên máy khác trong mạng:

```bash
# Cho phép port 8080
sudo ufw allow 8080/tcp

# Kiểm tra
sudo ufw status
```

---

## 📞 Hỗ Trợ

Nếu gặp vấn đề:

1. Kiểm tra logs: `tail -f server/server.log`
2. Kiểm tra server đang chạy: `ps aux | grep server`
3. Kiểm tra port: `netstat -tuln | grep 8080`
4. Xem lại hướng dẫn troubleshooting ở trên

---

## ✅ Checklist Trước Khi Chạy

- [ ] Đã cài đặt g++ và make
- [ ] Đã cài đặt Python 3 và tkinter
- [ ] Đã build server thành công (`make` không lỗi)
- [ ] Đã tạo file `server/config.json`
- [ ] Server đang chạy và listen trên port 8080
- [ ] Firewall cho phép port 8080 (nếu cần)
- [ ] Client có thể kết nối đến server

---

**Chúc bạn chơi game vui vẻ! 🎉**

