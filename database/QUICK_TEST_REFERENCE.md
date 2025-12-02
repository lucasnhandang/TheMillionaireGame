# Quick Test Reference

## Setup Commands

```bash
# 1. Reset Database
psql -U postgres -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"
psql -U postgres -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"
createdb -U postgres millionaire_game
psql -U postgres -d millionaire_game < database/schema.sql
psql -U postgres -d millionaire_game < database/mock_data.sql

# 2. Build Server
cd server
make clean && make

# 3. Start Server
./bin/server
```

## Quick Test Commands (using nc/telnet)

### Connect to Server
```bash
nc localhost 8080
# or
telnet localhost 8080
```

### 1. Register User
```json
{"requestType":"REGISTER","data":{"username":"testuser","password":"TestPass123"}}
```

### 2. Login
```json
{"requestType":"LOGIN","data":{"username":"testuser","password":"TestPass123"}}
```
**Save the `authToken` from response!**

### 3. Start Game
```json
{"requestType":"START","data":{"authToken":"<your-token>"}}
```
**Save the `gameId` from response!**

### 4. Use Lifeline (50/50)
```json
{"requestType":"LIFELINE","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"lifelineType":"5050"}}
```

### 5. Answer Question
```json
{"requestType":"ANSWER","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"answerIndex":1}}
```

### 6. View Leaderboard
```json
{"requestType":"LEADERBOARD","data":{"authToken":"<token>","type":"global","page":1,"limit":10}}
```

### 7. Add Friend
```json
{"requestType":"ADD_FRIEND","data":{"authToken":"<token>","friendUsername":"frienduser"}}
```

## Database Verification Commands

### Check User Created
```bash
psql -U postgres -d millionaire_game -c "SELECT id, username, role, is_banned FROM users WHERE username = 'testuser';"
```

### Check Game Session
```bash
psql -U postgres -d millionaire_game -c "SELECT id, status, current_question_number, current_prize, total_score FROM game_sessions ORDER BY id DESC LIMIT 1;"
```

### Check Answer Recorded
```bash
psql -U postgres -d millionaire_game -c "SELECT game_id, question_order, selected_option, is_correct FROM game_answers ORDER BY id DESC LIMIT 5;"
```

### Check Questions Available
```bash
psql -U postgres -d millionaire_game -c "SELECT level, COUNT(*) FROM questions WHERE is_active = true GROUP BY level;"
```

### Check Friend Requests
```bash
psql -U postgres -d millionaire_game -c "SELECT from_user_id, to_user_id, status FROM friend_requests;"
```

### Check Leaderboard
```bash
psql -U postgres -d millionaire_game -c "SELECT user_id, final_question_number, total_score FROM leaderboard ORDER BY total_score DESC LIMIT 10;"
```

## Automated Test

```bash
# Run automated test script
cd database
./test_integration.sh
```

## Common Issues

### Server won't start
```bash
# Check if port is in use
lsof -i :8080
# Kill process if needed
kill -9 <PID>
```

### Database connection error
```bash
# Check PostgreSQL is running
psql -U postgres -d postgres -c "SELECT 1;"
# Check database exists
psql -U postgres -d postgres -c "\l millionaire_game;"
```

### No questions found
```bash
# Reload mock data
psql -U postgres -d millionaire_game < database/mock_data.sql
```

cd server
make clean
make
lsof -ti :8080 | xargs kill -9
./bin/server

Client
{"requestType":"LOGIN","data":{"username":"testuser1","password":"TestPass123"}}

{"requestType":"START","data":{"authToken":"74622228f50fb1791b0629cc670eaed1"}}

psql -U postgres -d millionaire_game -c "SELECT q.question_text, q.option_a, q.option_b, q.option_c, q.option_d, q.correct_answer FROM game_questions gq JOIN questions q ON gq.question_id = q.id WHERE gq.game_id = 12 AND gq.question_order = 1;"

{"requestType":"ANSWER","data":{"authToken":"74622228f50fb1791b0629cc670eaed1","gameId":12,"questionNumber":1,"answerIndex":1}}