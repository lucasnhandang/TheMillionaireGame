# Sửa Lỗi PostgreSQL Authentication

## Lỗi: "fe_sendauth: no password supplied"

PostgreSQL yêu cầu password nhưng server không cung cấp.

## Giải pháp 1: Cấu hình PostgreSQL không cần password (Development)

### Bước 1: Tìm file pg_hba.conf

```bash
# Tìm vị trí file
sudo find /etc -name "pg_hba.conf" 2>/dev/null

# Hoặc
sudo -u postgres psql -c "SHOW hba_file;"
```

Thường ở: `/etc/postgresql/*/main/pg_hba.conf`

### Bước 2: Sửa file pg_hba.conf

```bash
# Backup file cũ
sudo cp /etc/postgresql/*/main/pg_hba.conf /etc/postgresql/*/main/pg_hba.conf.backup

# Sửa file
sudo nano /etc/postgresql/*/main/pg_hba.conf
```

Tìm dòng:
```
local   all             all                                     peer
host    all             all             127.0.0.1/32            md5
host    all             all             ::1/128                 md5
```

Thay đổi thành:
```
local   all             all                                     trust
host    all             all             127.0.0.1/32            trust
host    all             all             ::1/128                 trust
```

**Lưu ý:** `trust` cho phép kết nối không cần password - chỉ dùng cho development!

### Bước 3: Restart PostgreSQL

```bash
sudo systemctl restart postgresql
```

### Bước 4: Test connection

```bash
# Test không cần password
psql -U postgres -d millionaire_game -c "SELECT 1;"
```

## Giải pháp 2: Thêm password vào config.json

### Bước 1: Set password cho user postgres

```bash
sudo -u postgres psql
ALTER USER postgres PASSWORD 'your_password';
\q
```

### Bước 2: Sửa config.json

```bash
cd server
nano config.json
```

Thay đổi:
```json
"db_password": "your_password"
```

### Bước 3: Restart server

```bash
./bin/server
```

## Giải pháp 3: Tạo user mới không cần password

```bash
sudo -u postgres psql
CREATE USER game_user WITH PASSWORD '';
ALTER USER game_user CREATEDB;
GRANT ALL PRIVILEGES ON DATABASE millionaire_game TO game_user;
\q
```

Sau đó sửa `config.json`:
```json
"db_user": "game_user",
"db_password": ""
```

## Kiểm tra sau khi sửa

```bash
# Test connection
psql -U postgres -d millionaire_game -c "SELECT 1;"

# Restart server
cd server
./bin/server
```

Bạn sẽ thấy:
- `[INFO] Database connected successfully` - Thành công!
- Không còn `fe_sendauth: no password supplied` error

