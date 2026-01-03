# Hướng dẫn Build và Chạy Qt GUI Client

## Yêu cầu

### 1. Cài đặt Qt

Trên Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-base-dev-tools cmake build-essential
```

Hoặc nếu dùng Qt5:
```bash
sudo apt-get install qt5-default qtbase5-dev qtbase5-dev-tools cmake build-essential
```

### 2. Kiểm tra Qt đã cài đặt

```bash
qmake --version
# hoặc
cmake --version
```

## Build Project

### Cách 1: Sử dụng CMake (Khuyến nghị)

```bash
cd client/qt
mkdir build
cd build
cmake ..
make
```

Executable sẽ ở: `build/bin/client_qt`

### Cách 2: Sử dụng Qt Creator

1. Mở Qt Creator
2. File → Open File or Project
3. Chọn file `client/qt/CMakeLists.txt`
4. Configure project (chọn compiler và Qt version)
5. Build → Build Project (Ctrl+B)
6. Run → Run (Ctrl+R)

## Chạy Application

### 1. Đảm bảo server đang chạy

```bash
cd server
make
./bin/server
```

### 2. Chạy Qt client

```bash
cd client/qt/build/bin
./client_qt
```

Hoặc với server host/port tùy chỉnh:
```bash
./client_qt localhost 8080
```

## Cấu trúc Project

```
client/qt/
├── CMakeLists.txt          # CMake build configuration
├── main_qt.cpp             # Main entry point
├── network_thread.h/cpp    # Network communication thread
├── login_register_page.h/cpp   # Login/Register page
├── landing_page.h/cpp      # Main menu page
├── ingame_page.h/cpp       # Gameplay page
├── result_page.h/cpp       # Result page
├── instruction_dialog.h/cpp    # Instruction dialog
├── walkaway_dialog.h/cpp   # Walk Away confirmation dialog
└── resources.qrc          # Qt resource file
```

## Troubleshooting

### Lỗi: "Cannot find Qt"

```bash
# Kiểm tra Qt đã cài
dpkg -l | grep qt

# Cài đặt lại nếu cần
sudo apt-get install qt6-base-dev
```

### Lỗi: "CMake cannot find Qt"

Thử chỉ định đường dẫn Qt:
```bash
cmake -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/qt6 ..
```

### Lỗi: "Cannot connect to server"

1. Kiểm tra server đang chạy:
```bash
ps aux | grep server
```

2. Kiểm tra port 8080:
```bash
netstat -tuln | grep 8080
```

3. Thử kết nối thủ công:
```bash
telnet localhost 8080
```

## Database Setup

Trước khi chạy, đảm bảo database đã được setup và có câu hỏi:

```bash
cd database
# Thêm các trường lifeline vào questions table
sudo -u postgres psql millionaire_game -f add_lifeline_fields.sql

# Kiểm tra câu hỏi
./check_questions_by_level.sh
```

## Notes

- Qt client sử dụng QThread để xử lý network communication, tránh block UI
- Tất cả network operations chạy trong background thread
- UI updates được thực hiện qua Qt signals/slots mechanism
- Dark theme được áp dụng cho tất cả các pages

