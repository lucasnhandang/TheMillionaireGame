# Hướng Dẫn Build Client với Qt

## Yêu Cầu Hệ Thống (Linux)

```bash
# Cài đặt Qt5 và các dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake
sudo apt-get install -y qtbase5-dev qt5-qmake
sudo apt-get install -y libqt5widgets5 libqt5core5a libqt5gui5

# Nếu vẫn không tìm thấy Qt5, thử cài thêm:
sudo apt-get install -y qtchooser qt5-qmake qtbase5-dev-tools
```

**Lưu ý**: Nếu bạn đang dùng Ubuntu 20.04 trở lên, package `qt5-default` đã bị xóa. Chỉ cần cài `qtbase5-dev` là đủ.

## Build Project

```bash
cd client
mkdir -p build
cd build
cmake ..
make
```

**Nếu gặp lỗi "Could not find a package configuration file provided by Qt5":**

1. Kiểm tra Qt5 đã được cài đặt:
   ```bash
   qmake --version
   ```

2. Tìm file Qt5Config.cmake:
   ```bash
   find /usr -name "Qt5Config.cmake" 2>/dev/null
   ```

3. Sau khi tìm thấy file (ví dụ: `/usr/lib/x86_64-linux-gnu/cmake/Qt5/Qt5Config.cmake`), 
   chạy cmake với đường dẫn thư mục chứa Qt5Config.cmake:
   ```bash
   # Ví dụ nếu tìm thấy tại /usr/lib/x86_64-linux-gnu/cmake/Qt5/Qt5Config.cmake
   cmake -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5 ..
   ```
   
   HOẶC set biến môi trường:
   ```bash
   export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5
   cmake ..
   ```

4. Nếu vẫn không tìm thấy, thử:
   ```bash
   # Tìm tất cả thư mục chứa Qt5
   find /usr -type d -name "Qt5" 2>/dev/null | grep cmake
   
   # Sau đó thử với từng đường dẫn tìm được
   cmake -DCMAKE_PREFIX_PATH=<đường_dẫn_tìm_được> ..
   ```

5. Nếu qmake không có, cài đặt:
   ```bash
   sudo apt-get install qtbase5-dev qt5-qmake
   ```

Executable sẽ được tạo tại `build/MillionaireGameClient`

## Chạy Client

```bash
# Chạy với server mặc định (localhost:8080)
./build/MillionaireGameClient

# Chạy với server tùy chỉnh
./build/MillionaireGameClient 192.168.1.100 8080

# Chạy ở chế độ demo (không cần server)
./build/MillionaireGameClient --demo
```

## Cấu Trúc Project

```
client/
├── CMakeLists.txt          # Build configuration cho Qt
├── main.cpp                # Entry point với QApplication
├── mainwindow.h/cpp        # Main window quản lý screens
├── loginscreen.h/cpp       # Màn hình đăng nhập/đăng ký
├── homescreen.h/cpp        # Màn hình chính
├── gamescreen.h/cpp        # Màn hình chơi game
├── resultscreen.h/cpp      # Màn hình kết quả
├── gamestate.h/cpp         # Game state structure
├── qt_texture_loader.h/cpp # Texture loader cho Qt
├── socket_client.h/cpp     # TCP socket communication
├── protocol_handler.h/cpp  # Protocol handling
├── json_utils.h/cpp        # JSON utilities
└── game_event.h            # Game event system
```

## Sử dụng Qt Creator

1. Mở Qt Creator
2. File → Open File or Project
3. Chọn file `client/CMakeLists.txt`
4. Configure project (Qt Creator sẽ tự động detect Qt5)
5. Build → Build All (hoặc Ctrl+B)
6. Run → Run (hoặc Ctrl+R)

## Thiết Kế GUI với Qt Creator

1. Mở Qt Creator
2. File → Open File or Project → chọn file `.ui` (nếu có)
   - Hoặc bạn có thể tạo `.ui` files mới:
   - File → New File or Project → Qt → Qt Designer Form
3. Sử dụng Qt Designer để kéo thả widgets
4. Save file `.ui`
5. CMakeLists.txt đã được cấu hình để tự động build `.ui` files (CMAKE_AUTOUIC ON)

## Lưu Ý

- Tất cả code ImGUI và GLFW đã được xóa khỏi project
- Project hiện tại chỉ sử dụng Qt5 Widgets
- Texture loader đã được chuyển sang dùng QPixmap/QImage
- Build system sử dụng CMake thay vì Makefile cũ
