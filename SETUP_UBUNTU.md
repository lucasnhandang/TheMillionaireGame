# Hướng Dẫn Setup Nhanh Trên Ubuntu

Hướng dẫn nhanh để chạy project trên Ubuntu trong 5 phút.

## 🚀 Cách Nhanh Nhất

### Bước 1: Cài đặt dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### Bước 2: Chạy tất cả (server + client)

```bash
chmod +x run_all.sh
./run_all.sh
```

Script sẽ tự động:
- Build server
- Khởi động server ở background
- Chạy client
- Tắt server khi client đóng

---

## 📝 Cách Thủ Công

### Chạy Server

**Terminal 1:**
```bash
cd server
chmod +x start_server.sh
./start_server.sh
```

Hoặc:
```bash
cd server
make
./bin/server
```

### Chạy Client

**Terminal 2:**
```bash
cd client
make
./bin/client
```

---

## ✅ Kiểm Tra

1. Server đang chạy: `ps aux | grep server`
2. Port đang listen: `netstat -tuln | grep 8080`
3. Client kết nối được đến server

---

## 🐛 Lỗi Thường Gặp

### "g++ not found"
```bash
sudo apt-get install build-essential g++
```

### "GL/gl.h not found"
```bash
sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev
```

### "Address already in use"
```bash
sudo fuser -k 8080/tcp
```

---

Xem `HUONG_DAN_CHAY_UBUNTU.md` để biết chi tiết đầy đủ.

