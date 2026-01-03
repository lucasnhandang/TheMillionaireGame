-- Script to add sample questions WITH lifeline fields
-- Run this after creating the database schema and adding lifeline fields

-- First, ensure lifeline fields exist
ALTER TABLE questions 
ADD COLUMN IF NOT EXISTS lifeline_5050_info VARCHAR(10),
ADD COLUMN IF NOT EXISTS lifeline_call_info TEXT,
ADD COLUMN IF NOT EXISTS lifeline_ask_info VARCHAR(8);

-- Sample questions for level 1 (Easy)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level, 
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Thủ đô của Việt Nam là gì?', 'Hà Nội', 'Thành phố Hồ Chí Minh', 'Đà Nẵng', 'Huế', 0, 1, 
 '1,2', 'Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này.', '70001005'),
('2 + 2 bằng bao nhiêu?', '3', '4', '5', '6', 1, 1,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Hành tinh nào gần Mặt Trời nhất?', 'Sao Kim', 'Sao Thủy', 'Trái Đất', 'Sao Hỏa', 1, 1,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Đại dương nào lớn nhất trên Trái Đất?', 'Đại Tây Dương', 'Thái Bình Dương', 'Ấn Độ Dương', 'Bắc Băng Dương', 1, 1,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Có bao nhiêu châu lục trên Trái Đất?', '5', '6', '7', '8', 2, 1,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700');

-- Sample questions for level 2
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level,
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Ký hiệu hóa học của vàng là gì?', 'Go', 'Gd', 'Au', 'Ag', 2, 2,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Ai là tác giả của "Romeo và Juliet"?', 'Charles Dickens', 'William Shakespeare', 'Jane Austen', 'Mark Twain', 1, 2,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Tốc độ ánh sáng trong chân không là bao nhiêu?', '300,000 km/s', '150,000 km/s', '450,000 km/s', '600,000 km/s', 0, 2,
 '1,2', 'Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này.', '70001005'),
('Khí nào chiếm phần lớn bầu khí quyển Trái Đất?', 'Oxy', 'Nitơ', 'Carbon Dioxide', 'Argon', 1, 2,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Số nguyên tố nhỏ nhất là gì?', '0', '1', '2', '3', 2, 2,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700');

-- Sample questions for level 3
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level,
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Chiến tranh Thế giới thứ 2 kết thúc vào năm nào?', '1943', '1944', '1945', '1946', 2, 3,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Động vật có vú lớn nhất thế giới là gì?', 'Voi', 'Cá voi xanh', 'Hươu cao cổ', 'Hà mã', 1, 3,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Ai đã vẽ bức tranh Mona Lisa?', 'Vincent van Gogh', 'Pablo Picasso', 'Leonardo da Vinci', 'Michelangelo', 2, 3,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Chất cứng nhất trong tự nhiên trên Trái Đất là gì?', 'Vàng', 'Sắt', 'Kim cương', 'Bạch kim', 2, 3,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Có bao nhiêu xương trong cơ thể người trưởng thành?', '196', '206', '216', '226', 1, 3,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005');

-- Sample questions for level 4
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level,
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Sông nào dài nhất thế giới?', 'Amazon', 'Sông Nile', 'Dương Tử', 'Mississippi', 1, 4,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Ai đã phát minh ra điện thoại?', 'Thomas Edison', 'Alexander Graham Bell', 'Nikola Tesla', 'Guglielmo Marconi', 1, 4,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Quốc gia nhỏ nhất thế giới là gì?', 'Monaco', 'Vatican', 'San Marino', 'Liechtenstein', 1, 4,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Nguyên tố nào có số nguyên tử là 1?', 'Heli', 'Hydro', 'Lithi', 'Carbon', 1, 4,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Thủ đô của Australia là gì?', 'Sydney', 'Melbourne', 'Canberra', 'Brisbane', 2, 4,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700');

-- Sample questions for level 5
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level,
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Căn bậc hai của 144 là bao nhiêu?', '10', '11', '12', '13', 2, 5,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Ai đã viết "1984"?', 'Aldous Huxley', 'George Orwell', 'Ray Bradbury', 'H.G. Wells', 1, 5,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Công thức hóa học của nước là gì?', 'H2O', 'CO2', 'O2', 'NaCl', 0, 5,
 '1,2', 'Bạn của tôi nghĩ rằng đáp án đúng là A. Tôi khá chắc chắn về điều này.', '70001005'),
('Hành tinh nào được gọi là Hành tinh Đỏ?', 'Sao Kim', 'Sao Mộc', 'Sao Hỏa', 'Sao Thổ', 2, 5,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Sa mạc nào lớn nhất thế giới?', 'Gobi', 'Sahara', 'Nam Cực', 'Bắc Cực', 2, 5,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700');

-- Sample questions for levels 6-15 (add more as needed)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level,
                        lifeline_5050_info, lifeline_call_info, lifeline_ask_info) VALUES
('Thủ đô của Brazil là gì?', 'Rio de Janeiro', 'Sao Paulo', 'Brasilia', 'Salvador', 2, 6,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Ai đã phát hiện ra penicillin?', 'Louis Pasteur', 'Alexander Fleming', 'Robert Koch', 'Joseph Lister', 1, 7,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Hòn đảo lớn nhất thế giới là gì?', 'Borneo', 'Greenland', 'Madagascar', 'New Guinea', 1, 8,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Bức tường Berlin sụp đổ vào năm nào?', '1987', '1988', '1989', '1990', 2, 9,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Rãnh đại dương sâu nhất là gì?', 'Puerto Rico Trench', 'Java Trench', 'Mariana Trench', 'Tonga Trench', 2, 10,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là C. Tôi khá chắc chắn về điều này.', '01050700'),
('Ai đã viết "The Great Gatsby"?', 'Ernest Hemingway', 'F. Scott Fitzgerald', 'Mark Twain', 'John Steinbeck', 1, 11,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Nguyên tố nào có số nguyên tử 26?', 'Mangan', 'Sắt', 'Coban', 'Niken', 1, 12,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Ai đã vẽ "The Starry Night"?', 'Pablo Picasso', 'Vincent van Gogh', 'Claude Monet', 'Salvador Dalí', 1, 13,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005'),
('Quốc gia nào có diện tích lớn nhất thế giới?', 'Canada', 'Trung Quốc', 'Hoa Kỳ', 'Nga', 3, 14,
 '0,1', 'Bạn của tôi nghĩ rằng đáp án đúng là D. Tôi khá chắc chắn về điều này.', '01050070'),
('Ai đã phát minh ra máy in?', 'Thomas Edison', 'Johannes Gutenberg', 'Alexander Graham Bell', 'Nikola Tesla', 1, 15,
 '0,2', 'Bạn của tôi nghĩ rằng đáp án đúng là B. Tôi khá chắc chắn về điều này.', '01070005');

-- Verify questions were added
SELECT level, COUNT(*) as count 
FROM questions 
GROUP BY level 
ORDER BY level;

