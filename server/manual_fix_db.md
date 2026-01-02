# Hướng dẫn Sửa Database Authentication Thủ công

## Bước 1: Tìm file pg_hba.conf

```bash
sudo find /etc/postgresql -name "pg_hba.conf"
```

Hoặc:
```bash
sudo -u postgres psql -c "SHOW hba_file;"
```

## Bước 2: Xem nội dung hiện tại

```bash
sudo cat /etc/postgresql/*/main/pg_hba.conf | grep -E "^local|^host.*127|^host.*::1"
```

## Bước 3: Sửa file

```bash
sudo nano /etc/postgresql/*/main/pg_hba.conf
```

**Tìm và sửa:**

Tìm các dòng có dạng:
```
local   all             all                                     peer
host    all             all             127.0.0.1/32            md5
host    all             all             ::1/128                 md5
```

**Thay thành:**
```
local   all             all                                     trust
host    all             all             127.0.0.1/32            trust
host    all             all             ::1/128                 trust
```

**Lưu ý:** Phải giữ đúng số khoảng trắng và format!

## Bước 4: Reload PostgreSQL

```bash
sudo systemctl reload postgresql
```

Hoặc restart:
```bash
sudo systemctl restart postgresql
```

## Bước 5: Test

```bash
# Test không cần password
psql -U postgres -d millionaire_game -c "SELECT 1;"
```

Nếu thành công, bạn sẽ thấy output `1` mà không có lỗi.

## Bước 6: Restart Server

```bash
cd server
./bin/server
```

Bạn sẽ thấy: `[INFO] Database connected successfully`

## Nếu vẫn lỗi

Kiểm tra:
1. PostgreSQL đang chạy: `sudo systemctl status postgresql`
2. File đã được sửa đúng: `sudo grep trust /etc/postgresql/*/main/pg_hba.conf`
3. PostgreSQL đã reload: `sudo systemctl reload postgresql`

