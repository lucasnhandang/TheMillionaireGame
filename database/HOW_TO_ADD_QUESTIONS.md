# Hướng Dẫn Thêm Câu Hỏi vào Database

## Cách Nhanh Nhất (Khuyến nghị)

```bash
cd database
chmod +x add_questions.sh
./add_questions.sh
```

Script này sẽ:
1. Tự động thêm các trường lifeline nếu chưa có
2. Thêm câu hỏi từ file `add_questions_with_lifelines.sql` (hoặc `add_sample_questions.sql`)
3. Tự động fill lifeline data cho các câu hỏi
4. Hiển thị số lượng câu hỏi đã thêm

## Cách Thủ Công

### Bước 1: Thêm lifeline fields (nếu chưa có)

```bash
cd database
sudo -u postgres psql millionaire_game -f add_lifeline_fields.sql
```

### Bước 2: Thêm câu hỏi

```bash
# Cách 1: Dùng file có đầy đủ lifeline data
sudo -u postgres psql millionaire_game < add_questions_with_lifelines.sql

# Cách 2: Dùng file mẫu cũ, sau đó fill lifeline
sudo -u postgres psql millionaire_game < add_sample_questions.sql
sudo -u postgres psql millionaire_game -c "
UPDATE questions SET lifeline_5050_info = '0,1' WHERE lifeline_5050_info IS NULL;
UPDATE questions SET lifeline_call_info = 'Ban cua toi nghi rang dap an dung la A.' WHERE lifeline_call_info IS NULL;
UPDATE questions SET lifeline_ask_info = '25252525' WHERE lifeline_ask_info IS NULL;
"
```

## Thêm Câu Hỏi Mới Bằng SQL

### Cấu trúc câu hỏi:

```sql
INSERT INTO questions (
    question_text,      -- Nội dung câu hỏi
    option_a,           -- Đáp án A
    option_b,           -- Đáp án B
    option_c,           -- Đáp án C
    option_d,           -- Đáp án D
    correct_answer,     -- 0=A, 1=B, 2=C, 3=D
    level,              -- Level 1-15
    lifeline_5050_info, -- "0,1" = loại bỏ A và B
    lifeline_call_info, -- Text gợi ý từ bạn
    lifeline_ask_info   -- "70001005" = 70% A, 1% B, 0% C, 5% D
) VALUES (
    'Câu hỏi của bạn?',
    'Đáp án A',
    'Đáp án B',
    'Đáp án C',
    'Đáp án D',
    0,                  -- Đáp án đúng là A
    1,                  -- Level 1
    '1,2',              -- Loại bỏ B và C
    'Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này.',
    '70001005'         -- 70% chọn A
);
```

### Ví dụ:

```sql
INSERT INTO questions (
    question_text, option_a, option_b, option_c, option_d, 
    correct_answer, level, lifeline_5050_info, lifeline_call_info, lifeline_ask_info
) VALUES (
    'Ai là tác giả của "Truyện Kiều"?',
    'Nguyễn Du',
    'Nguyễn Trãi',
    'Hồ Xuân Hương',
    'Nguyễn Bỉnh Khiêm',
    0,  -- Đáp án A (Nguyễn Du)
    5,  -- Level 5
    '1,2',  -- Loại bỏ B và C
    'Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này.',
    '70001005'  -- 70% chọn A
);
```

## Giải Thích Lifeline Fields

### 1. `lifeline_5050_info` (VARCHAR(10))
- Format: `"0,1"` hoặc `"1,2"` (comma-separated)
- Ý nghĩa: Chỉ định 2 đáp án SAI sẽ bị loại bỏ
- Ví dụ: `"1,2"` = loại bỏ đáp án B và C

### 2. `lifeline_call_info` (TEXT)
- Format: Text mô tả gợi ý từ bạn
- Nên kết thúc bằng đáp án đúng
- Ví dụ: `"Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này."`

### 3. `lifeline_ask_info` (VARCHAR(8))
- Format: 8 chữ số (2 chữ số cho mỗi đáp án)
- Ý nghĩa: Phần trăm khán giả chọn từng đáp án
- Ví dụ: `"70001005"` = 70% A, 1% B, 0% C, 5% D

## Kiểm Tra Câu Hỏi

```bash
# Xem tất cả câu hỏi
sudo -u postgres psql millionaire_game -c "SELECT id, level, question_text FROM questions ORDER BY level;"

# Đếm câu hỏi theo level
sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) FROM questions GROUP BY level ORDER BY level;"

# Xem câu hỏi level 1
sudo -u postgres psql millionaire_game -c "SELECT * FROM questions WHERE level = 1;"
```

## File Quan Trọng

- `add_questions_with_lifelines.sql` - File SQL với đầy đủ lifeline data (KHUYẾN NGHỊ)
- `add_sample_questions.sql` - File SQL mẫu cũ (cần fill lifeline sau)
- `add_questions.sh` - Script tự động thêm câu hỏi
- `clean_questions.sh` - Script clean và fill lifeline cho câu hỏi hiện có

## Lưu Ý

1. **Mỗi level nên có ít nhất 2-3 câu hỏi** để server có thể chọn ngẫu nhiên
2. **correct_answer** phải là 0, 1, 2, hoặc 3
3. **level** phải từ 1 đến 15
4. **Lifeline fields** nên được fill đầy đủ để game hoạt động tốt

