# Build Instructions

## Requirements

- Linux operating system (không hỗ trợ Windows)
- GCC compiler với C++11 support
- Make hoặc CMake
- pthread library (thường có sẵn trên Linux)

## Build với Makefile

```bash
cd client
make
```

Executable sẽ được tạo tại `bin/client`

## Build với CMake

```bash
cd client
mkdir build
cd build
cmake ..
make
```

Executable sẽ được tạo tại `build/bin/client`

## Chạy chương trình

```bash
# Sử dụng Makefile
make run

# Hoặc chạy trực tiếp
./bin/client

# Hoặc với server khác
./bin/client localhost 8080
```

## Clean build artifacts

```bash
make clean
```

## Debug build

```bash
make debug
```

## Cấu trúc thư mục sau khi build

```
client/
├── bin/
│   └── client          # Executable
├── obj/
│   ├── *.o            # Object files
│   ├── gui/
│   └── utils/
└── ...
```

## Lưu ý

- Chương trình chỉ chạy trên Linux
- Sử dụng socket TCP thông thường (không phải WebSocket)
- Cần server đang chạy để kết nối
- Mặc định kết nối đến localhost:8080

## Troubleshooting

### Lỗi compilation
- Kiểm tra GCC version: `gcc --version` (cần >= 4.8)
- Kiểm tra pthread: `ldconfig -p | grep pthread`

### Lỗi kết nối
- Đảm bảo server đang chạy
- Kiểm tra firewall
- Kiểm tra địa chỉ và port server

