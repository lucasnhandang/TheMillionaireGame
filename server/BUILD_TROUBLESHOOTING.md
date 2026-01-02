# Hướng dẫn Sửa lỗi Build Server

## Lỗi: "file not recognized: file format not recognized"

Lỗi này xảy ra khi file object (`.o`) bị corrupt hoặc có format không đúng.

### Giải pháp nhanh:

```bash
cd server

# Bước 1: Clean hoàn toàn
make clean
# Hoặc xóa thủ công:
rm -rf obj bin

# Bước 2: Xóa tất cả file .o còn sót lại
find . -name "*.o" -type f -delete

# Bước 3: Tạo lại thư mục
mkdir -p obj bin

# Bước 4: Build lại
make
```

### Hoặc dùng script rebuild:

```bash
cd server
chmod +x rebuild.sh
./rebuild.sh
```

## Các lỗi thường gặp khác

### 1. Lỗi "No such file or directory" cho header files

**Nguyên nhân**: Thiếu include path

**Giải pháp**: Đảm bảo Makefile có `-I.` trong CXXFLAGS:
```makefile
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g -I.
```

### 2. Lỗi "No rule to make target"

**Nguyên nhân**: Makefile không tìm thấy source files

**Giải pháp**:
```bash
# Kiểm tra source files có tồn tại
ls *.cpp
ls request_handlers/*.cpp

# Clean và rebuild
make clean
make
```

### 3. Lỗi "undefined reference"

**Nguyên nhân**: Thiếu object files trong link command

**Giải pháp**: Đảm bảo tất cả object files được liệt kê trong rule `$(SERVER_EXE)`

### 4. Lỗi "multiple definition"

**Nguyên nhân**: Có function được define trong header file thay vì declare

**Giải pháp**: Kiểm tra header files, chỉ nên có declarations, definitions ở trong .cpp files

## Quy trình build đúng

```bash
# 1. Đảm bảo đang ở đúng thư mục
cd server

# 2. Clean build cũ
make clean

# 3. Build lại
make

# 4. Kiểm tra
ls -la bin/server
```

## Kiểm tra build thành công

Sau khi build, bạn sẽ thấy:
- File `bin/server` được tạo
- Không có error messages
- Có thể chạy: `./bin/server`

## Nếu vẫn không được

1. **Kiểm tra GCC version**:
   ```bash
   g++ --version
   ```
   Cần >= 4.8 (hỗ trợ C++11)

2. **Kiểm tra dependencies**:
   ```bash
   # Kiểm tra pthread
   ldconfig -p | grep pthread
   ```

3. **Build từng file riêng lẻ để tìm lỗi**:
   ```bash
   g++ -std=c++11 -Wall -Wextra -pthread -g -I. -c auth_manager.cpp -o obj/auth_manager.o
   ```

4. **Xem log chi tiết**:
   ```bash
   make 2>&1 | tee build.log
   ```

## Liên hệ

Nếu vẫn gặp vấn đề, kiểm tra:
- Tất cả source files có tồn tại
- Makefile đã được cập nhật đúng
- Không có file object corrupt từ lần build trước

