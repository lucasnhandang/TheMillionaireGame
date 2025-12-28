# Hướng Dẫn Cài Đặt GLFW

## Windows (MinGW/MSYS2)

### Bước 1: Giải nén GLFW

Sau khi tải GLFW precompiled binaries cho Windows (MinGW), giải nén file zip.

### Bước 2: Đặt GLFW vào đúng vị trí

Bạn cần tạo cấu trúc thư mục như sau:

```
TheMillionaireGame/
├── client/              (thư mục hiện tại của bạn)
│   ├── socket_client.h
│   ├── gui_app.h
│   └── ...
├── imgui/               (đã đặt ở đây)
└── glfw/                ← ĐẶT GLFW Ở ĐÂY
    ├── include/
    │   └── GLFW/
    │       └── glfw3.h
    └── lib/
        ├── libglfw3.a
        └── (hoặc các file .dll)
```

### Cách đặt:

1. **Tạo thư mục `glfw`** trong `TheMillionaireGame` (cùng cấp với `client`)

2. **Copy cấu trúc từ GLFW đã giải nén:**
   - Từ thư mục GLFW đã giải nén, tìm thư mục `include/` và copy vào `glfw/include/`
   - Tìm thư mục `lib/` hoặc `lib-mingw-w64/` và copy vào `glfw/lib/`

3. **Đảm bảo có các file:**
   ```
   glfw/
   ├── include/
   │   └── GLFW/
   │       └── glfw3.h          ← File header chính
   └── lib/
       ├── libglfw3.a           ← Library file (static)
       └── glfw3.dll            ← DLL file (nếu có)
   ```

### Ví dụ cụ thể:

Nếu bạn đang ở:
```
C:\Users\ASUS\Downloads\TheMillionaireGame\client\
```

Thì GLFW cần ở:
```
C:\Users\ASUS\Downloads\TheMillionaireGame\glfw\    ← Cùng cấp với client
    ├── include\
    │   └── GLFW\
    │       └── glfw3.h
    └── lib\
        └── libglfw3.a
```

## Linux (Ubuntu/Debian) - KHUYẾN NGHỊ

**Cách đơn giản nhất là cài đặt qua package manager:**

```bash
sudo apt-get update
sudo apt-get install libglfw3-dev libgl1-mesa-dev
```

Sau đó không cần đặt GLFW thủ công, Makefile sẽ tự động tìm library trong system.

## Linux (Build từ source)

Nếu bạn muốn build từ source hoặc dùng bản precompiled:

1. Giải nén GLFW source code
2. Build và install:
   ```bash
   cd glfw-3.x.x
   mkdir build
   cd build
   cmake ..
   make
   sudo make install
   ```

Hoặc đặt như Windows:
- Tạo `glfw/` trong `TheMillionaireGame/`
- Copy `include/` và `lib/` vào `glfw/`

## macOS

Cách đơn giản nhất:
```bash
brew install glfw
```

Hoặc đặt như Windows nếu muốn dùng local copy.

## Kiểm tra sau khi đặt:

### Trên Windows:
Mở Command Prompt ở thư mục `client` và chạy:
```bash
cd ..
dir glfw
dir glfw\include
dir glfw\lib
```

Bạn nên thấy:
- `glfw\include\GLFW\glfw3.h`
- `glfw\lib\libglfw3.a` (hoặc các file .a/.lib)

### Trên Linux (nếu dùng package manager):
```bash
pkg-config --cflags --libs glfw3
```

Nếu không lỗi thì đã cài đặt đúng.

## Sau khi đặt xong:

1. Quay lại thư mục `client`
2. Chạy `make` để build project
3. Nếu build thành công, chạy `./client` (hoặc `./client.exe` trên Windows)

## Troubleshooting

### Lỗi "cannot find -lglfw3" trên Windows:
- Kiểm tra file `libglfw3.a` có trong `glfw/lib/` không
- Đảm bảo tên file chính xác là `libglfw3.a`

### Lỗi "cannot find glfw3.h":
- Kiểm tra `glfw/include/GLFW/glfw3.h` có tồn tại không
- Đảm bảo cấu trúc thư mục đúng

### Lỗi trên Linux:
- Thử cài đặt qua package manager: `sudo apt-get install libglfw3-dev`
- Hoặc kiểm tra library path: `pkg-config --modversion glfw3`

## Lưu ý:

- **Trên Windows**: Nếu dùng MinGW, cần download GLFW precompiled cho MinGW
- **Trên Linux**: Nên dùng package manager để tránh vấn đề dependencies
- **Build từ source**: Có thể cần cài thêm CMake và build tools

