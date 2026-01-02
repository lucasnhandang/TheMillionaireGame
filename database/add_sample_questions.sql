-- Script to add sample questions to the database
-- Run this after creating the database schema

-- Sample questions for level 1 (Easy)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('What is the capital of Vietnam?', 'Hanoi', 'Ho Chi Minh City', 'Da Nang', 'Hue', 0, 1),
('What is 2 + 2?', '3', '4', '5', '6', 1, 1),
('Which planet is closest to the Sun?', 'Venus', 'Mercury', 'Earth', 'Mars', 1, 1),
('What is the largest ocean on Earth?', 'Atlantic', 'Pacific', 'Indian', 'Arctic', 1, 1),
('How many continents are there?', '5', '6', '7', '8', 2, 1);

-- Sample questions for level 2
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('What is the chemical symbol for gold?', 'Go', 'Gd', 'Au', 'Ag', 2, 2),
('Who wrote "Romeo and Juliet"?', 'Charles Dickens', 'William Shakespeare', 'Jane Austen', 'Mark Twain', 1, 2),
('What is the speed of light in vacuum?', '300,000 km/s', '150,000 km/s', '450,000 km/s', '600,000 km/s', 0, 2),
('Which gas makes up most of Earth''s atmosphere?', 'Oxygen', 'Nitrogen', 'Carbon Dioxide', 'Argon', 1, 2),
('What is the smallest prime number?', '0', '1', '2', '3', 2, 2);

-- Sample questions for level 3
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('In which year did World War II end?', '1943', '1944', '1945', '1946', 2, 3),
('What is the largest mammal in the world?', 'Elephant', 'Blue Whale', 'Giraffe', 'Hippopotamus', 1, 3),
('Who painted the Mona Lisa?', 'Vincent van Gogh', 'Pablo Picasso', 'Leonardo da Vinci', 'Michelangelo', 2, 3),
('What is the hardest natural substance on Earth?', 'Gold', 'Iron', 'Diamond', 'Platinum', 2, 3),
('How many bones are in an adult human body?', '196', '206', '216', '226', 1, 3);

-- Sample questions for level 4
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('What is the longest river in the world?', 'Amazon', 'Nile', 'Yangtze', 'Mississippi', 1, 4),
('Who invented the telephone?', 'Thomas Edison', 'Alexander Graham Bell', 'Nikola Tesla', 'Guglielmo Marconi', 1, 4),
('What is the smallest country in the world?', 'Monaco', 'Vatican City', 'San Marino', 'Liechtenstein', 1, 4),
('Which element has the atomic number 1?', 'Helium', 'Hydrogen', 'Lithium', 'Carbon', 1, 4),
('What is the capital of Australia?', 'Sydney', 'Melbourne', 'Canberra', 'Brisbane', 2, 4);

-- Sample questions for level 5
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('What is the square root of 144?', '10', '11', '12', '13', 2, 5),
('Who wrote "1984"?', 'Aldous Huxley', 'George Orwell', 'Ray Bradbury', 'H.G. Wells', 1, 5),
('What is the chemical formula for water?', 'H2O', 'CO2', 'O2', 'NaCl', 0, 5),
('Which planet is known as the Red Planet?', 'Venus', 'Jupiter', 'Mars', 'Saturn', 2, 5),
('What is the largest desert in the world?', 'Gobi', 'Sahara', 'Antarctic', 'Arctic', 2, 5);

-- Sample questions for levels 6-15 (you can add more)
INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, correct_answer, level) VALUES
('What is the capital of Brazil?', 'Rio de Janeiro', 'Sao Paulo', 'Brasilia', 'Salvador', 2, 6),
('Who discovered penicillin?', 'Louis Pasteur', 'Alexander Fleming', 'Robert Koch', 'Joseph Lister', 1, 7),
('What is the largest island in the world?', 'Borneo', 'Greenland', 'Madagascar', 'New Guinea', 1, 8),
('In which year did the Berlin Wall fall?', '1987', '1988', '1989', '1990', 2, 9),
('What is the deepest ocean trench?', 'Puerto Rico Trench', 'Java Trench', 'Mariana Trench', 'Tonga Trench', 2, 10),
('Who composed "The Four Seasons"?', 'Bach', 'Vivaldi', 'Mozart', 'Beethoven', 1, 11),
('What is the largest organ in the human body?', 'Liver', 'Lungs', 'Skin', 'Intestines', 2, 12),
('Which country is home to the kangaroo?', 'New Zealand', 'Australia', 'South Africa', 'Argentina', 1, 13),
('What is the chemical symbol for silver?', 'Si', 'Sv', 'Ag', 'Au', 2, 14),
('Who was the first person to walk on the moon?', 'Buzz Aldrin', 'Neil Armstrong', 'Michael Collins', 'John Glenn', 1, 15);

-- Verify questions were added
SELECT level, COUNT(*) as question_count FROM questions GROUP BY level ORDER BY level;

