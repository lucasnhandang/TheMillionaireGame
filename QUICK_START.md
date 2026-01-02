# Hướng dẫn Chạy Game - Quick Start

## Bước 1: Chạy Server (Đã xong ✅)

Server đang chạy trên port 8080. **GIỮ NGUYÊN terminal này**, đừng tắt server!

Bạn sẽ thấy:
```
Server started on port 8080
EventLoop started with poll()
```

## Bước 2: Mở Terminal Mới và Chạy Client

### Mở terminal mới (Terminal 2)

**Trong terminal mới**, thực hiện:

```bash
# Di chuyển đến thư mục client
cd ~/TheMillionaireGame/client

# Build client (nếu chưa build)
make
# Hoặc nếu dùng CMake:
# mkdir -p build && cd build && cmake .. && make

# Chạy client
./bin/client
# Hoặc nếu dùng CMake:
# ./build/bin/client
```

### Nếu client chưa được build

```bash
cd ~/TheMillionaireGame/client

# Với Makefile:
make

# Hoặc với CMake:
mkdir -p build
cd build
cmake ..
make
cd ..
./build/bin/client
```

## Bước 3: Sử dụng Game

Sau khi client kết nối thành công, bạn sẽ thấy:

1. **Màn hình đăng nhập/đăng ký**:
   - Chọn 1 để đăng nhập
   - Chọn 2 để đăng ký (tạo tài khoản mới)
   - Chọn 3 để thoát

2. **Sau khi đăng nhập thành công**:
   - Menu chính với các tùy chọn:
     - 1. Bắt đầu chơi mới
     - 2. Tiếp tục game đã lưu
     - 3. Bảng xếp hạng
     - 4. Thông tin người chơi
     - 5. Lịch sử chơi
     - 6. Đổi mật khẩu
     - 7. Bạn bè
     - 9. Đăng xuất

3. **Khi chơi game**:
   - Xem câu hỏi và 4 đáp án
   - Chọn đáp án (0-3, tương ứng A-D)
   - Sử dụng trợ giúp (50/50, Gọi điện, Hỏi khán giả)
   - Bỏ cuộc nếu muốn

## Lưu ý Quan Trọng

1. **Server phải chạy trước**: Terminal 1 chạy server, Terminal 2 chạy client
2. **Đừng tắt server**: Nếu tắt server, client sẽ mất kết nối
3. **Port mặc định**: Server chạy trên port 8080, client tự động kết nối
4. **Nếu dùng port khác**: 
   ```bash
   # Server: ./bin/server -p 8081
   # Client: ./bin/client localhost 8081
   ```

## Troubleshooting

### Client không kết nối được

1. **Kiểm tra server đang chạy**:
   - Xem terminal server, phải có "Server started on port 8080"

2. **Kiểm tra port**:
   - Server và client phải dùng cùng port
   - Mặc định: 8080

3. **Kiểm tra firewall**:
   ```bash
   sudo ufw status
   sudo ufw allow 8080
   ```

### Lỗi "Connection refused"

- Server chưa chạy hoặc đã tắt
- Chạy lại server trong Terminal 1

### Client build lỗi

Xem `client/BUILD.md` để biết cách build client.

## Tóm tắt

```
Terminal 1 (Server):    Terminal 2 (Client):
cd server              cd client
./bin/server           make
                        ./bin/client
```

Chúc bạn chơi game vui vẻ! 🎮

