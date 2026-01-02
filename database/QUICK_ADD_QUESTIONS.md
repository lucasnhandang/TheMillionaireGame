# Hướng dẫn Nhanh: Thêm Câu Hỏi từ File Sample

## Kiểm tra câu hỏi hiện có

```bash
# Xem số câu hỏi mỗi level
psql -U postgres millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"

# Xem tất cả câu hỏi
psql -U postgres millionaire_game -c "SELECT id, level, question_text FROM questions ORDER BY level, id;"
```

## Thêm câu hỏi từ file sample

Nếu script đã chạy nhưng bạn muốn thêm lại hoặc thêm thêm:

```bash
cd ~/TheMillionaireGame/database

# Thêm câu hỏi từ file sample
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

**Lưu ý:** Nếu câu hỏi đã tồn tại, có thể bị lỗi duplicate. Để thêm lại, xóa câu hỏi cũ trước:

```bash
# Xóa tất cả câu hỏi cũ (cẩn thận!)
sudo -u postgres psql millionaire_game -c "DELETE FROM questions;"

# Sau đó thêm lại
sudo -u postgres psql millionaire_game < add_sample_questions.sql
```

## Thêm câu hỏi thủ công

### Cách 1: Dùng psql

```bash
psql -U postgres millionaire_game
```

Sau đó trong psql:

```sql
-- Xem câu hỏi hiện có
SELECT id, level, question_text FROM questions ORDER BY level;

-- Thêm một câu hỏi mới
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) 
VALUES (
    'Câu hỏi của bạn?',
    'Đáp án A',
    'Đáp án B',
    'Đáp án C',
    'Đáp án D',
    0,  -- 0=A, 1=B, 2=C, 3=D
    1   -- Level 1-15
);

-- Thoát
\q
```

### Cách 2: Tạo file SQL riêng

Tạo file `my_questions.sql`:

```sql
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('Câu hỏi 1?', 'A1', 'B1', 'C1', 'D1', 0, 1),
('Câu hỏi 2?', 'A2', 'B2', 'C2', 'D2', 1, 2);
```

Sau đó chạy:

```bash
sudo -u postgres psql millionaire_game < my_questions.sql
```

## Kiểm tra sau khi thêm

```bash
# Đếm tổng số câu hỏi
psql -U postgres millionaire_game -c "SELECT COUNT(*) as total_questions FROM questions;"

# Xem câu hỏi theo level
psql -U postgres millionaire_game -c "SELECT level, COUNT(*) as count FROM questions GROUP BY level ORDER BY level;"

# Xem chi tiết một vài câu hỏi
psql -U postgres millionaire_game -c "SELECT id, level, LEFT(question_text, 50) as question FROM questions LIMIT 10;"
```

## Lưu ý quan trọng

1. **Server chưa tích hợp database**: Hiện tại server chưa load questions từ database (có TODO comments). Câu hỏi trong database sẽ sẵn sàng khi server được tích hợp database.

2. **Để game hoạt động ngay**: Cần tích hợp database vào server code (thay các TODO bằng database calls).

3. **Mỗi level nên có ít nhất 2-3 câu hỏi** để server có thể chọn ngẫu nhiên.

## Xem file sample questions

```bash
cat add_sample_questions.sql
```

File này chứa 30 câu hỏi mẫu (2 câu cho mỗi level 1-15).

