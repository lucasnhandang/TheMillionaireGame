# Hướng Dẫn Rebuild GLFW Cho Linux

## Vấn Đề

GLFW library hiện tại trong project có thể là bản build cho Windows, gây lỗi "dangerous relocation" khi link trên Linux.

## Giải Pháp

### Cách 1: Dùng Makefile (Dễ nhất)

```bash
cd client
make rebuild-glfw
```

Lệnh này sẽ tự động:
- Xóa build cũ
- Build GLFW mới cho Linux
- Copy library vào đúng vị trí

### Cách 2: Chạy Script Thủ Công

```bash
cd client
chmod +x rebuild_glfw.sh
./rebuild_glfw.sh
```

Nếu vẫn bị permission denied:

```bash
cd client
bash rebuild_glfw.sh
```

### Cách 3: Chạy Từng Lệnh (Nếu vẫn lỗi)

```bash
# Vào thư mục glfw
cd glfw

# Xóa build cũ
rm -rf build

# Tạo thư mục build
mkdir -p build
cd build

# Configure cho Linux
cmake .. \
    -DGLFW_BUILD_X11=ON \
    -DGLFW_BUILD_WAYLAND=OFF \
    -DGLFW_BUILD_WIN32=OFF \
    -DGLFW_BUILD_COCOA=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DGLFW_BUILD_EXAMPLES=OFF \
    -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_DOCS=OFF

# Build
make -j$(nproc)

# Copy library
mkdir -p ../lib
cp src/libglfw3.a ../lib/

# Quay lại client và build
cd ../../client
make clean
make
```

### Cách 4: Dùng System GLFW (Khuyến nghị nếu có)

```bash
# Cài system GLFW
sudo apt-get install -y libglfw3-dev

# Build client (sẽ tự động dùng system GLFW)
cd client
make clean
make
```

## Kiểm Tra

Sau khi rebuild, kiểm tra:

```bash
# Kiểm tra file library
ls -lh glfw/lib/libglfw3.a
file glfw/lib/libglfw3.a

# File phải là ELF (Linux), không phải PE (Windows)
```

## Troubleshooting

### Lỗi: "Permission denied"

**Giải pháp 1:** Dùng bash thay vì chạy trực tiếp:
```bash
bash rebuild_glfw.sh
```

**Giải pháp 2:** Chạy từng lệnh thủ công (xem Cách 3 ở trên)

**Giải pháp 3:** Kiểm tra quyền thư mục:
```bash
ls -ld glfw
chmod -R u+w glfw  # Nếu cần
```

### Lỗi: "cmake: command not found"

```bash
sudo apt-get install -y cmake
```

### Lỗi: "No rule to make target rebuild-glfw"

Đảm bảo bạn đang ở đúng thư mục:
```bash
cd client
make rebuild-glfw
```

### Vẫn bị lỗi "dangerous relocation"

1. Xóa hoàn toàn build cũ:
```bash
cd glfw
rm -rf build lib/libglfw3.a
```

2. Rebuild lại:
```bash
cd ../client
make rebuild-glfw
```

3. Build client:
```bash
make clean
make
```

## Sau Khi Rebuild Thành Công

```bash
cd client
make clean
make
./bin/client
```

