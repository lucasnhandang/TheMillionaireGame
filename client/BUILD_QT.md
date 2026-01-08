# Hướng Dẫn Build Client với Qt

## Yêu Cầu Hệ Thống (Linux)

```bash
# Cài đặt Qt5 và các dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake
sudo apt-get install -y qt5-default qtbase5-dev qtbase5-dev-tools
sudo apt-get install -y libqt5widgets5 libqt5core5a libqt5gui5
```

## Build Project

```bash
cd client
mkdir -p build
cd build
cmake ..
make
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
