# Hướng Dẫn Chạy GUI Client Trên Ubuntu

## ⚠️ Lưu Ý Quan Trọng

**Client GUI cần server để hoạt động!** Client không thể chạy độc lập vì:
- Client kết nối đến server qua TCP socket
- Server xử lý game logic, authentication, questions
- Client chỉ hiển thị UI và gửi/nhận messages từ server

## 🎯 Có 2 Cách Chạy GUI

### Cách 1: Chạy với Server Thật (Khuyến nghị)

Đây là cách đúng để test toàn bộ hệ thống.

#### Bước 1: Build và chạy Server

**Terminal 1:**
```bash
cd server
make
./bin/server
```

Server sẽ chạy trên port 8080 (mặc định).

#### Bước 2: Build và chạy Client

**Terminal 2:**
```bash
cd client
make
./bin/client
```

Client sẽ tự động kết nối đến `localhost:8080`.

#### Bước 3: Sử dụng

1. Đăng ký tài khoản mới hoặc đăng nhập
2. Bắt đầu chơi game
3. Trả lời câu hỏi và sử dụng lifelines

---

### Cách 2: Chạy với Demo Mode (Test GUI không cần server)

Nếu bạn chỉ muốn xem GUI hoạt động mà không cần server, có thể dùng demo mode.

#### Build Client với Demo Mode

```bash
cd client
make clean
make DEMO_MODE=1
./bin/client
```

**Lưu ý:** Demo mode chỉ hiển thị UI, không có chức năng thật. Để có đầy đủ tính năng, cần chạy với server.

---

## 📋 Yêu Cầu Để Chạy GUI

### Dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Build GLFW (nếu chưa có)

```bash
cd glfw
cmake .
make
cd ..
```

---

## 🔧 Build Client

### Build Bình Thường (cần server)

```bash
cd client
make clean
make
```

Sau khi build thành công, file executable sẽ ở: `client/bin/client`

**Kiểm tra:**
```bash
ls -lh client/bin/client
```

### Build với Demo Mode (không cần server)

```bash
cd client
make clean
CXXFLAGS="-DDEMO_MODE" make
```

Hoặc sửa Makefile để thêm flag:

```makefile
CXXFLAGS = -std=c++11 -Wall -Wextra -g -DDEMO_MODE -I. -I../imgui ...
```

---

## 🚀 Chạy Client

### Kết nối đến Server Mặc Định (localhost:8080)

```bash
cd client
make          # Build nếu chưa build
./bin/client
```

**Lưu ý:** Nếu chưa có file `bin/client`, cần build trước:
```bash
cd client
make
```

### Kết nối đến Server Khác

```bash
cd client
./bin/client 192.168.1.100 8080
```

### Chạy với Demo Mode

```bash
cd client
./bin/client --demo
```

**Nếu không có file `bin/client`:**
```bash
cd client
make          # Build trước
./bin/client --demo
```

---

## 🐛 Troubleshooting

### Lỗi: "Failed to connect to server"

**Nguyên nhân:** Server chưa chạy hoặc không thể kết nối.

**Giải pháp:**
1. Kiểm tra server có đang chạy:
```bash
ps aux | grep server
netstat -tuln | grep 8080
```

2. Khởi động server trước:
```bash
cd server
./bin/server
```

3. Kiểm tra firewall:
```bash
sudo ufw allow 8080
```

### Lỗi: "GL/gl.h: No such file or directory"

**Giải pháp:**
```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
```

### Lỗi: "GLFW library not found"

**Giải pháp:**
```bash
cd glfw
cmake .
make
cd ../client
make
```

### Lỗi: "Segmentation fault"

**Giải pháp:**
1. Kiểm tra tất cả dependencies đã cài đủ
2. Build lại GLFW
3. Chạy với gdb để debug:
```bash
gdb ./bin/client
run
```

---

## 📝 Kiểm Tra Kết Nối

### Test Server có đang chạy

```bash
# Kiểm tra process
ps aux | grep server

# Kiểm tra port
netstat -tuln | grep 8080
# hoặc
ss -tuln | grep 8080

# Test kết nối
telnet localhost 8080
```

### Test Client có thể kết nối

Khi chạy client, nếu kết nối thành công sẽ thấy:
- Cửa sổ login xuất hiện
- Không có error message về connection

Nếu thất bại sẽ thấy:
- Error dialog: "Failed to connect to server"
- Hoặc console output: "Connection failed"

---

## 🎮 Sử Dụng GUI

### Đăng Ký Tài Khoản

1. Mở client
2. Chọn tab "Register" (nếu có) hoặc nhấn "Switch to Register"
3. Nhập:
   - Username: (ví dụ: `testuser`)
   - Password: Tối thiểu 8 ký tự, có chữ hoa, chữ thường, số (ví dụ: `Test1234`)
   - Confirm Password: Nhập lại password
4. Nhấn "Register"
5. Nếu thành công, chuyển sang tab Login

### Đăng Nhập

1. Nhập username và password
2. Nhấn "Login"
3. Nếu thành công, sẽ vào màn hình game

### Chơi Game

1. Nhấn "Start New Game" để bắt đầu
2. Đọc câu hỏi và chọn đáp án (A, B, C, hoặc D)
3. Nhấn "Submit Answer"
4. Sử dụng lifelines khi cần:
   - **50/50**: Loại bỏ 2 đáp án sai
   - **Phone a Friend**: Nhận gợi ý
   - **Ask the Audience**: Xem kết quả bình chọn
5. Trả lời đúng 15 câu để thắng!

---

## 🔄 Workflow Đầy Đủ

### Terminal 1 - Server
```bash
cd TheMillionaireGame/server
make
./bin/server
```

### Terminal 2 - Client
```bash
cd TheMillionaireGame/client
make
./bin/client
```

### Terminal 3 - Xem Logs (tùy chọn)
```bash
tail -f server/server.log
```

---

## ✅ Checklist

Trước khi chạy client, đảm bảo:

- [ ] Đã cài đặt tất cả dependencies
- [ ] Đã build GLFW thành công
- [ ] Đã build server thành công
- [ ] Server đang chạy và listen trên port 8080
- [ ] Firewall cho phép port 8080 (nếu cần)
- [ ] Đã build client thành công

---

## 💡 Tips

1. **Chạy server ở background:**
```bash
cd server
nohup ./bin/server > server.log 2>&1 &
```

2. **Kiểm tra server đang chạy:**
```bash
ps aux | grep server
```

3. **Dừng server:**
```bash
pkill server
# hoặc
killall server
```

4. **Xem log server:**
```bash
tail -f server/server.log
```

---

## 📞 Hỗ Trợ

Nếu gặp vấn đề:

1. Kiểm tra server có đang chạy
2. Kiểm tra port 8080 có đang listen
3. Kiểm tra firewall settings
4. Xem logs: `tail -f server/server.log`
5. Xem lại hướng dẫn troubleshooting ở trên

---

**Lưu ý:** Client GUI **KHÔNG THỂ** chạy độc lập mà không có server. Server là thành phần bắt buộc để xử lý game logic, authentication, và quản lý game state.

