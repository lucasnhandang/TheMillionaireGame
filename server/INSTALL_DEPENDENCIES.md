# Hướng dẫn Cài đặt Dependencies cho Server

## Lỗi: libpq-fe.h: No such file or directory

Lỗi này xảy ra khi thiếu PostgreSQL development libraries.

## Giải pháp

### Ubuntu/Debian:

```bash
# Cài đặt PostgreSQL development libraries
sudo apt-get update
sudo apt-get install libpq-dev

# Kiểm tra đã cài đặt chưa
dpkg -l | grep libpq-dev
```

### Nếu vẫn không tìm thấy header:

```bash
# Tìm vị trí libpq-fe.h
find /usr -name "libpq-fe.h" 2>/dev/null

# Hoặc
locate libpq-fe.h
```

### Nếu header ở vị trí khác thường:

Có thể cần thêm include path vào Makefile:

```makefile
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g -I. -I/usr/include/postgresql
```

## Kiểm tra sau khi cài đặt

```bash
# Kiểm tra header file
ls /usr/include/postgresql/libpq-fe.h

# Hoặc
ls /usr/include/libpq-fe.h

# Rebuild server
cd server
make clean
make
```

## Troubleshooting

### Nếu vẫn lỗi sau khi cài libpq-dev:

1. **Kiểm tra PostgreSQL packages:**
   ```bash
   dpkg -l | grep postgresql
   ```

2. **Cài đặt đầy đủ:**
   ```bash
   sudo apt-get install postgresql postgresql-contrib libpq-dev
   ```

3. **Kiểm tra include path:**
   ```bash
   echo | g++ -E -v -x c++ - 2>&1 | grep include
   ```

4. **Thử compile thủ công:**
   ```bash
   g++ -I/usr/include/postgresql -c database.cpp -o test.o
   ```

## Alternative: Dùng pkg-config

Nếu có pkg-config:

```bash
# Kiểm tra
pkg-config --cflags libpq

# Thêm vào Makefile
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -g -I. $(shell pkg-config --cflags libpq)
LDFLAGS = -pthread $(shell pkg-config --libs libpq)
```

