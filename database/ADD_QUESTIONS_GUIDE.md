# Hướng dẫn Thêm Câu Hỏi vào Database

## Cách 1: Thêm bằng SQL Script (Khuyến nghị)

### Bước 1: Đảm bảo Database đã được tạo

```bash
# Tạo database (nếu chưa có)
createdb millionaire_game

# Tạo schema
psql millionaire_game < schema.sql
```

### Bước 2: Thêm câu hỏi mẫu

```bash
cd database
psql millionaire_game < add_sample_questions.sql
```

Script này sẽ thêm 30 câu hỏi mẫu (2 câu hỏi cho mỗi level từ 1-15).

### Bước 3: Kiểm tra

```bash
psql millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"
```

## Cách 2: Thêm câu hỏi thủ công bằng SQL

```sql
-- Kết nối database
psql millionaire_game

-- Thêm một câu hỏi
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) 
VALUES (
    'Câu hỏi của bạn?',           -- question_text
    'Đáp án A',                   -- option_a
    'Đáp án B',                   -- option_b
    'Đáp án C',                   -- option_c
    'Đáp án D',                   -- option_d
    0,                            -- correct_answer (0=A, 1=B, 2=C, 3=D)
    1                             -- level (1-15)
);
```

### Ví dụ:

```sql
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) 
VALUES (
    'Ai là tác giả của "Truyện Kiều"?',
    'Nguyễn Du',
    'Nguyễn Trãi',
    'Hồ Xuân Hương',
    'Nguyễn Bỉnh Khiêm',
    0,  -- Đáp án A (Nguyễn Du)
    5   -- Level 5
);
```

## Cách 3: Thêm nhiều câu hỏi cùng lúc

```sql
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('Câu hỏi 1?', 'A1', 'B1', 'C1', 'D1', 0, 1),
('Câu hỏi 2?', 'A2', 'B2', 'C2', 'D2', 1, 1),
('Câu hỏi 3?', 'A3', 'B3', 'C3', 'D3', 2, 2);
```

## Cách 4: Dùng Admin Interface (Khi đã tích hợp database)

Nếu server đã tích hợp database, bạn có thể:
1. Đăng nhập bằng tài khoản admin
2. Vào menu "Quản trị (Admin)"
3. Chọn "Thêm câu hỏi"
4. Điền thông tin câu hỏi

**Lưu ý:** Hiện tại admin interface chưa được tích hợp database, nên cần dùng SQL.

## Cấu trúc Bảng Questions

```sql
CREATE TABLE questions (
    id SERIAL PRIMARY KEY,                    -- ID tự động
    question_text TEXT NOT NULL,              -- Nội dung câu hỏi
    option_a TEXT NOT NULL,                   -- Đáp án A
    option_b TEXT NOT NULL,                   -- Đáp án B
    option_c TEXT NOT NULL,                   -- Đáp án C
    option_d TEXT NOT NULL,                   -- Đáp án D
    correct_answer INTEGER NOT NULL,          -- 0=A, 1=B, 2=C, 3=D
    level INTEGER NOT NULL,                   -- Level 1-15
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## Quy tắc Thêm Câu Hỏi

1. **Level 1-5**: Câu hỏi dễ
2. **Level 6-10**: Câu hỏi trung bình
3. **Level 11-15**: Câu hỏi khó

3. **correct_answer**: 
   - 0 = Đáp án A
   - 1 = Đáp án B
   - 2 = Đáp án C
   - 3 = Đáp án D

4. **Nên có ít nhất 2-3 câu hỏi cho mỗi level** để game có thể chọn ngẫu nhiên

## Kiểm tra và Quản lý Câu Hỏi

### Xem tất cả câu hỏi:
```sql
SELECT id, level, question_text, correct_answer FROM questions ORDER BY level, id;
```

### Xem câu hỏi theo level:
```sql
SELECT * FROM questions WHERE level = 1;
```

### Đếm số câu hỏi mỗi level:
```sql
SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;
```

### Xóa câu hỏi:
```sql
DELETE FROM questions WHERE id = 1;
```

### Sửa câu hỏi:
```sql
UPDATE questions 
SET question_text = 'Câu hỏi mới?',
    option_a = 'A mới',
    correct_answer = 1
WHERE id = 1;
```

## Lưu ý

- Đảm bảo có đủ câu hỏi cho tất cả 15 levels
- Mỗi level nên có ít nhất 2-3 câu hỏi để server có thể chọn ngẫu nhiên
- correct_answer phải là 0, 1, 2, hoặc 3
- level phải từ 1 đến 15

## Sau khi thêm câu hỏi

1. Restart server (nếu cần)
2. Chơi game và kiểm tra câu hỏi có hiển thị không
3. Nếu vẫn không có câu hỏi, kiểm tra:
   - Database connection trong server
   - Server có load questions từ database không
   - Xem server log để debug

