# Hướng Dẫn Build Client

## ⚠️ Quan Trọng

File executable sẽ được tạo ở: **`client/bin/client`**

Nếu không thấy file này, có nghĩa là build chưa thành công hoặc chưa chạy `make`.

## 🔨 Build Client

### Bước 1: Kiểm tra dependencies

```bash
# Kiểm tra g++
g++ --version

# Kiểm tra make
make --version

# Kiểm tra cmake (cho GLFW)
cmake --version
```

### Bước 2: Cài đặt dependencies (nếu thiếu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Bước 3: Build GLFW (nếu chưa có)

```bash
cd glfw
cmake .
make
cd ..
```

Kiểm tra file `glfw/lib/libglfw3.a` có tồn tại.

### Bước 4: Build Client

```bash
cd client
make clean    # Xóa build cũ (tùy chọn)
make          # Build client
```

### Bước 5: Kiểm tra file executable

```bash
ls -lh bin/client
```

Nếu thấy file `bin/client` và có quyền thực thi (x), nghĩa là build thành công!

## 🚀 Chạy Client

```bash
cd client
./bin/client
```

Hoặc:

```bash
cd client
make run
```

## 🐛 Troubleshooting Build

### Lỗi: "No rule to make target 'bin/client'"

**Nguyên nhân:** Chưa tạo thư mục `bin/` hoặc Makefile không chạy đúng.

**Giải pháp:**
```bash
cd client
mkdir -p bin obj
make
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

### Lỗi: "GL/gl.h: No such file or directory"

**Giải pháp:**
```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
```

### Lỗi: "imgui.h: No such file or directory"

**Nguyên nhân:** Include path không đúng.

**Giải pháp:**
Kiểm tra cấu trúc thư mục:
```bash
ls ../imgui/imgui.h    # Phải có file này
```

Nếu không có, đảm bảo bạn đang ở đúng thư mục project root.

### Lỗi: "undefined reference to..."

**Nguyên nhân:** Thiếu libraries khi link.

**Giải pháp:**
Kiểm tra Makefile có đầy đủ libraries:
- `-lGL -lGLU` (OpenGL)
- `-lX11 -lXrandr -lXinerama -lXcursor` (X11)
- `-pthread` (threading)

### Build thành công nhưng không chạy được

**Kiểm tra:**
```bash
# Kiểm tra file có tồn tại
ls -lh bin/client

# Kiểm tra quyền thực thi
chmod +x bin/client

# Chạy với output chi tiết
./bin/client 2>&1 | head -20
```

## 📝 Build với Verbose Output

Để xem chi tiết quá trình build:

```bash
cd client
make clean
make VERBOSE=1
```

Hoặc:

```bash
cd client
make clean
make -n    # Chỉ hiển thị commands, không chạy
```

## ✅ Checklist Build

Trước khi build, đảm bảo:

- [ ] Đã cài đặt g++, make, cmake
- [ ] Đã cài đặt OpenGL và X11 libraries
- [ ] Đã build GLFW (file `glfw/lib/libglfw3.a` tồn tại)
- [ ] Đang ở đúng thư mục `client/`
- [ ] Thư mục `imgui/` ở cùng level với `client/`
- [ ] Thư mục `glfw/` ở cùng level với `client/`

Sau khi build:

- [ ] File `bin/client` tồn tại
- [ ] File có quyền thực thi
- [ ] Có thể chạy: `./bin/client`

## 🔍 Kiểm Tra Build

```bash
cd client

# Kiểm tra file executable
file bin/client

# Kiểm tra dependencies
ldd bin/client

# Kiểm tra kích thước
ls -lh bin/client
```

## 💡 Tips

1. **Build lại từ đầu:**
```bash
cd client
make clean
make
```

2. **Xem tất cả files được tạo:**
```bash
cd client
find . -name "*.o" -o -name "client"
```

3. **Build với debug info:**
```bash
cd client
make clean
CXXFLAGS="-g -O0" make
```

4. **Build release (tối ưu):**
```bash
cd client
make clean
CXXFLAGS="-O2 -DNDEBUG" make
```

