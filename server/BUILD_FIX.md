# Hướng dẫn Build Server - Sửa lỗi

## Vấn đề đã sửa

1. **Thiếu include path**: Makefile đã được cập nhật để thêm `-I.` vào CXXFLAGS, giúp compiler tìm thấy các header files.

## Cách build lại

### Bước 1: Clean build cũ
```bash
cd server
make clean
```

### Bước 2: Build lại
```bash
make
```

### Bước 3: Kiểm tra
```bash
ls -la bin/server
```

Nếu thấy file `bin/server` thì build thành công!

## Chạy server

```bash
./bin/server
```

Hoặc với port tùy chỉnh:
```bash
./bin/server -p 8080
```

## Nếu vẫn gặp lỗi

### Lỗi "No such file or directory" cho header files
- Đảm bảo bạn đang ở đúng thư mục `server/`
- Kiểm tra file header có tồn tại: `ls *.h`

### Lỗi "No rule to make target"
- Chạy `make clean` trước
- Sau đó chạy `make` lại
- Đảm bảo Makefile đã được cập nhật với `-I.` flag

### Lỗi database
- Server có thể chạy mà không cần database (sẽ có warning)
- Để setup database, xem `server/docs/README.md`

## Kiểm tra build thành công

Sau khi build, bạn sẽ thấy:
```
[100%] Built target server
```

Và file `bin/server` sẽ được tạo.

