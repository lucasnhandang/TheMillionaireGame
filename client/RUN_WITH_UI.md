# Hướng Dẫn Chạy Client với File UI (.ui)

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

## Cấu Hình File UI

File `loginscreen.ui` đã được cấu hình để:
- Load tự động qua CMake (`CMAKE_AUTOUIC ON`)
- Tạo class `Ui::LoginScreen` từ file `.ui`
- Widget names trong `.ui` phải khớp với code:
  - `usernameEdit`
  - `passwordEdit`
  - `confirmPasswordEdit`
  - `loginButton`
  - `registerButton`
  - `switchToRegisterButton`
  - `errorLabel`

## Build Project

### Bước 1: Di chuyển vào thư mục client
```bash
cd client
```

### Bước 2: Tạo thư mục build
```bash
mkdir -p build
cd build
```

### Bước 3: Chạy CMake
```bash
cmake ..
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

### Bước 4: Build
```bash
make
```

Nếu build thành công, bạn sẽ thấy:
- File `ui_loginscreen.h` được tự động generate từ `loginscreen.ui`
- Executable `MillionaireGameClient` được tạo trong thư mục `build/`

## Chạy Client

### Chạy với server mặc định (localhost:8080)
```bash
./MillionaireGameClient
```

### Chạy với server tùy chỉnh
```bash
./MillionaireGameClient 192.168.1.100 8080
```

### Chạy ở chế độ demo (không cần server)
```bash
./MillionaireGameClient --demo
```

## Chỉnh Sửa UI trong Qt Creator

1. **Mở file .ui trong Qt Creator:**
   ```bash
   # Trong Qt Creator:
   File → Open File or Project → chọn client/loginscreen.ui
   ```

2. **Chỉnh sửa UI:**
   - Kéo thả widgets từ Widget Box
   - Chỉnh properties trong Property Editor
   - Set stylesheet cho widgets
   - Đặt object names (quan trọng để code có thể truy cập)

3. **Lưu file:**
   - Save file `.ui` (Ctrl+S)

4. **Rebuild:**
   ```bash
   cd build
   make
   ```
   CMake sẽ tự động regenerate file `ui_loginscreen.h` từ `.ui` mới.

## Kiểm Tra Build UI File

Sau khi build, kiểm tra file `ui_loginscreen.h` đã được generate:
```bash
ls -la build/ui_loginscreen.h
```

File này chứa class `Ui::LoginScreen` với các widget pointers từ file `.ui`.

## Troubleshooting

### Lỗi: "ui_loginscreen.h: No such file or directory"
- Đảm bảo `CMAKE_AUTOUIC ON` trong CMakeLists.txt
- Xóa thư mục build và build lại:
  ```bash
  cd client
  rm -rf build
  mkdir build
  cd build
  cmake ..
  make
  ```

### Lỗi: "Undefined reference to Ui::LoginScreen"
- Đảm bảo file `.ui` được thêm vào `add_executable()` trong CMakeLists.txt
- Check xem `QT_UI_FILES` có chứa `loginscreen.ui`

### Widget không hiển thị
- Kiểm tra object name trong `.ui` file có khớp với code không
- Kiểm tra widget có được add vào UI layout không
- Check visibility properties

### UI không load đúng
- Xóa `build/CMakeCache.txt` và build lại
- Đảm bảo class name trong `.ui` file là `LoginScreen` (không phải `Dialog`)

## Cấu Trúc File UI

File `loginscreen.ui` sử dụng:
- Base widget: `QWidget` (class: `LoginScreen`)
- Widgets: `QLineEdit`, `QPushButton`, `QLabel`
- Geometry-based layout (absolute positioning)
- Style sheets cho styling

## Notes

- File `.ui` sẽ được compile thành C++ code tự động
- Không cần chỉnh sửa file `ui_loginscreen.h` thủ công
- Mọi thay đổi trong `.ui` file sẽ được áp dụng sau khi rebuild
- Widget pointers trong code được lấy từ `ui->` object sau khi gọi `ui->setupUi(this)`
