# Hướng Dẫn Build GLFW trên Windows (MinGW)

## Vấn đề
Bạn đã tải GLFW source code từ trang web, nhưng chưa có file `.lib` hoặc `.a` cần thiết để link vào project.

## Giải pháp 1: Tải Precompiled Binaries (KHUYẾN NGHỊ - Đơn giản nhất)

### Bước 1: Tải GLFW Precompiled cho MinGW

1. Vào trang: https://www.glfw.org/download.html
2. Tìm phần **"Pre-compiled binaries"** 
3. Tải file: **glfw-3.x.x.bin.WIN64.zip** hoặc **glfw-3.x.x.bin.WIN32.zip** (tùy hệ thống 64-bit hay 32-bit)

### Bước 2: Giải nén và đặt đúng vị trí

1. Giải nén file zip vừa tải
2. Bên trong sẽ có thư mục `glfw-3.x.x.bin.WIN64/` hoặc tương tự
3. Mở thư mục đó, bạn sẽ thấy cấu trúc:
   ```
   glfw-3.x.x.bin.WIN64/
   ├── include/
   │   └── GLFW/
   │       └── glfw3.h
   └── lib-mingw-w64/
       └── libglfw3.a
   ```

4. **Copy các file vào đúng vị trí:**
   - Copy `include/GLFW/` vào `TheMillionaireGame/glfw/include/GLFW/` (đã có rồi thì bỏ qua)
   - Copy `lib-mingw-w64/libglfw3.a` vào `TheMillionaireGame/glfw/lib/libglfw3.a`
   - Hoặc copy toàn bộ `lib-mingw-w64/` vào `glfw/lib/`

### Cấu trúc sau khi đặt:
```
TheMillionaireGame/
└── glfw/
    ├── include/
    │   └── GLFW/
    │       └── glfw3.h          ✅ Đã có
    └── lib/                      ← CẦN TẠO THƯ MỤC NÀY
        └── libglfw3.a            ← Copy file này vào đây
```

## Giải pháp 2: Build GLFW từ Source (Nếu không tìm thấy precompiled)

### Yêu cầu:
- CMake (tải từ https://cmake.org/)
- MinGW-w64 (bạn đã có g++ rồi)

### Các bước:

1. **Cài CMake** (nếu chưa có):
   ```powershell
   # Kiểm tra xem đã có CMake chưa
   cmake --version
   ```

2. **Tạo thư mục build trong GLFW:**
   ```powershell
   cd ..\glfw
   mkdir build
   cd build
   ```

3. **Build với CMake:**
   ```powershell
   cmake -G "MinGW Makefiles" ..
   ```

4. **Compile:**
   ```powershell
   mingw32-make
   ```

5. **Copy library vào đúng vị trí:**
   Sau khi build xong, file `libglfw3.a` sẽ ở trong `build/src/`
   
   Tạo thư mục lib nếu chưa có:
   ```powershell
   cd ..\..
   cd glfw
   mkdir lib
   ```
   
   Copy file:
   ```powershell
   copy build\src\libglfw3.a lib\
   ```

## Giải pháp 3: Dùng GLFW từ MSYS2 (Nếu bạn dùng MSYS2)

Nếu bạn đang dùng MSYS2, có thể cài đặt qua package manager:

```bash
pacman -S mingw-w64-x86_64-glfw
```

Sau đó không cần đặt trong project folder, Makefile sẽ tự tìm.

## Kiểm tra sau khi hoàn thành

Sau khi đã có file lib, kiểm tra:

```powershell
cd client
dir ..\glfw\lib
```

Bạn nên thấy file `libglfw3.a` trong đó.

## Sau đó build client:

Bạn cần có `make` hoặc tôi sẽ tạo script build cho bạn.

