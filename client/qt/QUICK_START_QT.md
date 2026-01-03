# Quick Start - Qt GUI Client

## Bước 1: Cài đặt Dependencies

```bash
# Cài đặt Qt và build tools
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-base-dev-tools cmake build-essential

# Hoặc Qt5
sudo apt-get install qt5-default qtbase5-dev qtbase5-dev-tools cmake build-essential
```

## Bước 2: Setup Database (nếu chưa có)

```bash
cd database

# Thêm các trường lifeline vào questions table
sudo -u postgres psql millionaire_game -f add_lifeline_fields.sql

# Kiểm tra câu hỏi
./check_questions_by_level.sh
```

## Bước 3: Build Qt Client

```bash
cd client/qt
mkdir -p build
cd build
cmake ..
make
```

## Bước 4: Chạy Server

Mở terminal mới:
```bash
cd server
make
./bin/server
```

## Bước 5: Chạy Qt Client

```bash
cd client/qt/build/bin
./client_qt
```

## Các Tính Năng Đã Implement

### ✅ Login/Register Page
- Form login đè lên page chính
- Form register đè lên page chính
- Nút Quit để thoát

### ✅ Landing Page
- Logo "Who Wants to Be a Millionaire"
- Welcome message với username
- Menu buttons: Play, Instruction, Leaderboard, Friend, Quit
- Logout button (icon ⚙) → đăng xuất sau 2s

### ✅ InGame Page
- Hiển thị câu hỏi và 4 đáp án
- Timer đếm ngược 30 giây (màu thay đổi theo thời gian)
- Prize tree bên phải (highlight câu hỏi hiện tại)
- 3 lifelines: 50:50, Phone a Friend, Ask the Audience
- Walk Away button (góc phải dưới)
- Timer pause khi dùng lifeline hoặc Walk Away dialog
- Xử lý đáp án: highlight vàng khi chọn, đợi 5s, hiển thị kết quả (xanh/đỏ)

### ✅ Result Page
- Hiển thị kết quả game (thắng/thua)
- Giải thưởng cuối cùng
- Số câu hỏi đã trả lời đúng
- Nút "Back to Menu" và "Play Again"

### ✅ Instruction Dialog
- Hiển thị hướng dẫn chơi game
- Nút X để đóng

### ✅ Walk Away Dialog
- Confirmation dialog "Are you sure?"
- Nút Yes/No

## Các Tính Năng Cần Hoàn Thiện

### ⚠️ Lifeline Data từ Database
Hiện tại lifeline data chưa được load từ database. Cần:
1. Update server để gửi lifeline data trong QUESTION_INFO notification
2. Hoặc query database khi nhận lifeline request

### ⚠️ Leaderboard Page
Chưa implement, hiện chỉ hiển thị message "coming soon"

### ⚠️ Friend Page
Chưa implement, hiện chỉ hiển thị message "coming soon"

## Troubleshooting

### Lỗi: "Cannot find Qt"
```bash
sudo apt-get install qt6-base-dev
```

### Lỗi: "CMake cannot find Qt"
```bash
cmake -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/qt6 ..
```

### Lỗi: "Cannot connect to server"
1. Kiểm tra server đang chạy: `ps aux | grep server`
2. Kiểm tra port: `netstat -tuln | grep 8080`

### Lỗi: "No question found"
1. Kiểm tra database có câu hỏi: `./database/check_questions_by_level.sh`
2. Kiểm tra server log xem có lỗi database không

## Notes

- Tất cả network operations chạy trong background thread (không block UI)
- UI updates qua Qt signals/slots
- Dark theme áp dụng cho tất cả pages
- Timer tự động pause khi dùng lifeline hoặc Walk Away dialog

