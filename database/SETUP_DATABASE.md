# Hướng dẫn Setup Database PostgreSQL

## Vấn đề: "role does not exist"

Lỗi này xảy ra khi PostgreSQL không có user/role tương ứng với username Linux của bạn.

## Giải pháp

### Cách 1: Dùng user postgres mặc định (Đơn giản nhất)

```bash
# Kết nối với user postgres (superuser)
sudo -u postgres psql

# Hoặc nếu không cần sudo:
psql -U postgres
```

Sau đó trong psql:

```sql
-- Tạo database
CREATE DATABASE millionaire_game;

-- Tạo user mới (tùy chọn)
CREATE USER luxiel WITH PASSWORD 'your_password';
GRANT ALL PRIVILEGES ON DATABASE millionaire_game TO luxiel;

-- Thoát
\q
```

Sau đó chạy schema:

```bash
sudo -u postgres psql millionaire_game < schema.sql
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

### Cách 2: Tạo PostgreSQL user cho username hiện tại

```bash
# Tạo PostgreSQL user với tên giống username Linux
sudo -u postgres createuser -s luxiel

# Hoặc với password:
sudo -u postgres createuser -s -P luxiel
# (sẽ hỏi password)

# Sau đó tạo database
sudo -u postgres createdb millionaire_game

# Chạy schema
sudo -u postgres psql millionaire_game < schema.sql
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

### Cách 3: Dùng user postgres trực tiếp

```bash
# Tạo database
sudo -u postgres createdb millionaire_game

# Chạy schema
sudo -u postgres psql millionaire_game < schema.sql

# Thêm câu hỏi
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

## Kiểm tra PostgreSQL đang chạy

```bash
# Kiểm tra service
sudo systemctl status postgresql

# Nếu chưa chạy, start nó
sudo systemctl start postgresql
```

## Kiểm tra kết nối

```bash
# Test kết nối với user postgres
sudo -u postgres psql -c "SELECT version();"

# Hoặc
psql -U postgres -c "SELECT version();"
```

## Sau khi setup xong

Bạn có thể kết nối bằng:

```bash
# Với user postgres
psql -U postgres millionaire_game

# Hoặc nếu đã tạo user luxiel
psql millionaire_game
```

## Kiểm tra câu hỏi đã thêm

```bash
psql -U postgres millionaire_game -c "SELECT level, COUNT(*) FROM questions GROUP BY level ORDER BY level;"
```

## Troubleshooting

### Lỗi "peer authentication failed"

Sửa file `/etc/postgresql/*/main/pg_hba.conf`:
```
# Thay đổi từ:
local   all             all                                     peer

# Thành:
local   all             all                                     md5
```

Sau đó restart PostgreSQL:
```bash
sudo systemctl restart postgresql
```

### Lỗi "database does not exist"

```bash
sudo -u postgres createdb millionaire_game
```

### Lỗi "permission denied"

Dùng `sudo -u postgres` hoặc tạo user với quyền phù hợp.

