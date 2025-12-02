# Database Integration Test Guide

## Prerequisites

1. **PostgreSQL Running**: 
   ```bash
   # Check if PostgreSQL is running
   psql -U postgres -d postgres -c "SELECT version();"
   ```

2. **Database Setup**:
   ```bash
   cd "/path/to/your/repo"
   
   # Reset database (if needed)
   psql -U postgres -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"
   psql -U postgres -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"
   createdb -U postgres millionaire_game
   psql -U postgres -d millionaire_game < database/schema.sql
   psql -U postgres -d millionaire_game < database/mock_data.sql
   ```

3. **Build Server**:
   ```bash
   cd server
   make clean
   make
   ```

4. **Start Server**:
   ```bash
   cd server
   ./bin/server
   # Should see: "Database connected successfully" and "Server started on port 8080"
   ```

5. **Test Client Tool**: Use `nc` (netcat) or `telnet` to connect:
   ```bash
   nc localhost 8080
   # or
   telnet localhost 8080
   ```

---

## Test Case 1: Database Connection Works ✅

### Test Steps:
1. Start the server (see Prerequisites)
2. Check server logs for: `"Database connected successfully"`

### Expected Result:
- Server starts without errors
- Log shows: `[INFO] Database connected successfully`
- Server listens on port 8080

### Verification:
```bash
# Check if server is listening
lsof -i :8080

# Should show server process
```

---

## Test Case 2: User Authentication Works ✅

### Test 2.1: User Registration

**Request:**
```json
{"requestType":"REGISTER","data":{"username":"testuser1","password":"TestPass123"}}
```

**Expected Response:**
```json
{"responseCode":201,"data":{"username":"testuser1","message":"Registration successful. Please login to continue."}}
```

**Verification:**
```bash
# Check database
psql -U postgres -d millionaire_game -c "SELECT id, username, role, is_banned FROM users WHERE username = 'testuser1';"
# Should show: id, username='testuser1', role='user', is_banned=false
```

### Test 2.2: User Login

**Request:**
```json
{"requestType":"LOGIN","data":{"username":"testuser1","password":"TestPass123"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"authToken":"<32-char-hex-token>","username":"testuser1","role":"user","message":"Login successful"}}
```

**Verification:**
- Save the `authToken` for subsequent requests
- Check database:
```bash
psql -U postgres -d millionaire_game -c "SELECT id, username, last_login FROM users WHERE username = 'testuser1';"
# Should show last_login timestamp updated
```

### Test 2.3: Invalid Login

**Request:**
```json
{"requestType":"LOGIN","data":{"username":"testuser1","password":"WrongPass"}}
```

**Expected Response:**
```json
{"responseCode":401,"error":"Invalid credentials"}
```

### Test 2.4: Login with Non-existent User

**Request:**
```json
{"requestType":"LOGIN","data":{"username":"nonexistent","password":"TestPass123"}}
```

**Expected Response:**
```json
{"responseCode":401,"error":"Invalid credentials"}
```

---

## Test Case 3: Game Session Creation Works ✅

### Test 3.1: Start New Game

**Prerequisites:** User must be logged in (get authToken from Test 2.2)

**Request:**
```json
{"requestType":"START","data":{"authToken":"<your-auth-token>"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Game started","gameId":<number>,"timestamp":<unix-timestamp>}}
```

**Verification:**
```bash
# Check database
psql -U postgres -d millionaire_game -c "SELECT id, user_id, status, current_question_number, current_level, current_prize, total_score FROM game_sessions ORDER BY id DESC LIMIT 1;"
# Should show:
# - id: matches gameId from response
# - user_id: matches testuser1's user_id
# - status: 'active'
# - current_question_number: 1
# - current_level: 0 (easy)
# - current_prize: 1000000
# - total_score: 0
```

### Test 3.2: Start Game When Already In Game

**Request:** (Send START again with same authToken)

**Expected Response:**
```json
{"responseCode":405,"error":"Already in a game"}
```

---

## Test Case 4: Question Retrieval Works ✅

### Test 4.1: Verify Question Levels

**Verification:**
```bash
# Check questions by level
psql -U postgres -d millionaire_game -c "SELECT level, COUNT(*) FROM questions WHERE is_active = true GROUP BY level ORDER BY level;"
# Should show:
# level | count
#   0   |  X    (easy questions)
#   1   |  Y    (medium questions)
#   2   |  Z    (hard questions)
```

---

## Test Case 5: Answer Validation Works ✅

### Test 5.1: Correct Answer

**Prerequisites:** 
- User logged in
- Game started (gameId from Test 3.1)

**Step 1: Get a question to know the correct answer**
```bash
# After starting a game, check if questions exist, then correct_answer
# Remember to change the game_id
psql -U postgres -d millionaire_game -c "SELECT q.question_text, q.option_a, q.option_b, q.option_c, q.option_d, q.correct_answer FROM game_questions gq JOIN questions q ON gq.question_id = q.id WHERE gq.game_id = 8 AND gq.question_order = 1;"
```

**Request:**
```json
{"requestType":"ANSWER","data":{"authToken":"<your-auth-token>","gameId":<game-id>,"questionNumber":1,"answerIndex":<correct-answer>}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"gameId":<game-id>,"correct":true,"questionNumber":1,"timeRemaining":<0-15>,"pointsEarned":<points>,"totalScore":<score>,"currentPrize":2000000,"gameOver":false,"isWinner":false}}
```

**Verification:**
```bash
# Check game session updated
psql -U postgres -d millionaire_game -c "SELECT current_question_number, total_score, current_prize FROM game_sessions WHERE id = <game-id>;"
# Should show:
# - current_question_number: 2 (incremented)
# - total_score: > 0 (points earned)
# - current_prize: 2000000 (doubled)

# Check game_answers table
psql -U postgres -d millionaire_game -c "SELECT game_id, question_order, selected_option, is_correct FROM game_answers WHERE game_id = <game-id>;"
# Should show answer recorded with is_correct = true
```

### Test 5.2: Wrong Answer

**Request:** (Use wrong answerIndex, e.g., if correct is 1, use 0)

```json
{"requestType":"ANSWER","data":{"authToken":"<your-auth-token>","gameId":<game-id>,"questionNumber":2,"answerIndex":0}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"gameId":<game-id>,"correct":false,"questionNumber":2,"correctAnswer":<actual-correct>,"pointsEarned":0,"safeCheckpointPrize":0,"safeCheckpointScore":<previous-score>,"totalScore":<score>,"finalPrize":0,"gameOver":true,"isWinner":false}}
```

**Verification:**
```bash
# Check game session ended
psql -U postgres -d millionaire_game -c "SELECT status, ended_at, final_prize FROM game_sessions WHERE id = <game-id>;"
# Should show:
# - status: 'lost'
# - ended_at: timestamp set
# - final_prize: 0 (no safe checkpoint before question 5)

# Check answer recorded
psql -U postgres -d millionaire_game -c "SELECT is_correct FROM game_answers WHERE game_id = <game-id> AND question_order = 2;"
# Should show: is_correct = false
```

### Test 5.3: Answer After Safe Checkpoint (Question 6+)

**Steps:**
1. Start new game
2. Answer questions 1-5 correctly (or simulate by updating database)
3. Answer question 6 incorrectly

**Verification:**
```bash
# After wrong answer at question 6
psql -U postgres -d millionaire_game -c "SELECT final_prize FROM game_sessions WHERE id = <game-id>;"
# Should show: final_prize = 10000000 (safe checkpoint at question 5)
```

---

## Test Case 6: Scoring Calculation Works ✅

### Test 6.1: Score Calculation with Time

**Prerequisites:** Game started

**Request:** (Answer quickly, within 15 seconds)
```json
{"requestType":"ANSWER","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"answerIndex":<correct>}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"pointsEarned":<10-15>,"totalScore":<score>}}
```

**Verification:**
- `pointsEarned` should be between 0-15 (based on time remaining)
- `totalScore` should accumulate

### Test 6.2: Score Calculation with Lifelines

**Steps:**
1. Start game
2. Use a lifeline (see Test 7.1)
3. Answer question

**Expected:** `pointsEarned` reduced by 5 points per lifeline used

**Verification:**
```bash
# Check total_score in game_sessions
psql -U postgres -d millionaire_game -c "SELECT total_score FROM game_sessions WHERE id = <game-id>;"
# Score should reflect lifeline penalty
```

### Test 6.3: Prize Progression

**Verification:**
```bash
# After answering questions correctly, check prize progression
psql -U postgres -d millionaire_game -c "SELECT current_question_number, current_prize FROM game_sessions WHERE id = <game-id> ORDER BY current_question_number;"
# Should show:
# Question 1: 1,000,000
# Question 2: 2,000,000
# Question 3: 4,000,000
# ...
# Question 15: 1,000,000,000
```

---

## Test Case 7: Lifeline Processing Works ✅

### Test 7.1: Use 50/50 Lifeline

**Prerequisites:** Game started, on question 1

**Request:**
```json
{"requestType":"LIFELINE","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"lifelineType":"5050"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Lifeline processed","lifelineType":"5050","delaySeconds":2,"result":{"remainingOptions":[<correct-index>,<one-wrong-index]}}}
```

**Verification:**
- Response should contain exactly 2 options (one correct, one wrong)
- Options should be sorted
- Check session:
```bash
# Verify lifeline tracked in session (would need to check server logs or add database tracking)
```

### Test 7.2: Use Phone a Friend

**Request:**
```json
{"requestType":"LIFELINE","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"lifelineType":"PHONE"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Lifeline processed","lifelineType":"PHONE","delaySeconds":5,"result":{"suggestion":<0-3>,"label":"<A-D>","confidence":"<message>"}}}
```

**Verification:**
- `suggestion` should be 0-3
- `label` should be A, B, C, or D
- `confidence` should contain suggestion text

### Test 7.3: Use Ask the Audience

**Request:**
```json
{"requestType":"LIFELINE","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"lifelineType":"AUDIENCE"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Lifeline processed","lifelineType":"AUDIENCE","delaySeconds":3,"result":{"percentages":{"A":<0-100>,"B":<0-100>,"C":<0-100>,"D":<0-100>}}}}
```

**Verification:**
- All percentages (A, B, C, D) should sum to 100
- Correct answer should have highest percentage (40-60%)
- Other options should have lower percentages

### Test 7.4: Try Using Same Lifeline Twice

**Request:** (Use same lifeline again)

**Expected Response:**
```json
{"responseCode":407,"error":"Lifeline already used"}
```

---

## Test Case 8: Leaderboard Queries Work ✅

### Test 8.1: Global Leaderboard

**Prerequisites:** 
- Multiple users with completed games
- Some games with scores

**Request:**
```json
{"requestType":"LEADERBOARD","data":{"authToken":"<token>","type":"global","page":1,"limit":10}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"rankings":[{"username":"<user>","finalQuestionNumber":<1-15>,"totalScore":<score>,"rank":<1-N>,"isWinner":<true/false>},...],"total":<count>,"page":1,"limit":10}}
```

**Verification:**
```bash
# Check leaderboard table
psql -U postgres -d millionaire_game -c "SELECT user_id, final_question_number, total_score FROM leaderboard ORDER BY total_score DESC LIMIT 10;"
# Should match response data

# Verify ranking order (highest score first)
psql -U postgres -d millionaire_game -c "SELECT username, total_score FROM leaderboard l JOIN users u ON l.user_id = u.id ORDER BY total_score DESC;"
```

### Test 8.2: Friend Leaderboard

**Prerequisites:**
- User has friends
- Friends have completed games

**Request:**
```json
{"requestType":"LEADERBOARD","data":{"authToken":"<token>","type":"friend","page":1,"limit":10}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"rankings":[{"username":"<friend>","finalQuestionNumber":<1-15>,"totalScore":<score>,"rank":<1-N>,"isWinner":<true/false>},...],"total":<count>,"page":1,"limit":10}}
```

**Verification:**
- Should only show friends' scores
- Should be ranked by score

---

## Test Case 9: Friend Operations Work ✅

### Test 9.1: Add Friend

**Prerequisites:** 
- Two users registered: `testuser1` and `testuser2`
- `testuser1` logged in

**Request:**
```json
{"requestType":"ADD_FRIEND","data":{"authToken":"<testuser1-token>","friendUsername":"testuser2"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Friend request sent successfully"}}
```

**Verification:**
```bash
# Check friend_requests table
psql -U postgres -d millionaire_game -c "SELECT from_user_id, to_user_id, status FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'testuser1') AND to_user_id = (SELECT id FROM users WHERE username = 'testuser2');"
# Should show: status = 'pending'
```

### Test 9.2: List Friend Requests

**Prerequisites:** Friend request sent (Test 9.1)
**Login as:** `testuser2`

**Request:**
```json
{"requestType":"FRIEND_REQ_LIST","data":{"authToken":"<testuser2-token>"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"friendRequests":[{"username":"testuser1","sentAt":<timestamp>}]}}
```

**Verification:**
- Should show pending requests for `testuser2`
- `username` should be `testuser1`

### Test 9.3: Accept Friend Request

**Request:**
```json
{"requestType":"ACCEPT_FRIEND","data":{"authToken":"<testuser2-token>","friendUsername":"testuser1"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Friend request accepted successfully","friendUsername":"testuser1"}}
```

**Verification:**
```bash
# Check friendships table
psql -U postgres -d millionaire_game -c "SELECT user1_id, user2_id FROM friendships WHERE (user1_id = (SELECT id FROM users WHERE username = 'testuser1') AND user2_id = (SELECT id FROM users WHERE username = 'testuser2')) OR (user1_id = (SELECT id FROM users WHERE username = 'testuser2') AND user2_id = (SELECT id FROM users WHERE username = 'testuser1'));"
# Should show one friendship record

# Check friend_requests status changed
psql -U postgres -d millionaire_game -c "SELECT status FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'testuser1') AND to_user_id = (SELECT id FROM users WHERE username = 'testuser2');"
# Should show: status = 'accepted' (or request deleted)
```

### Test 9.4: Friend Status

**Request:**
```json
{"requestType":"FRIEND_STATUS","data":{"authToken":"<testuser1-token>"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"friends":[{"username":"testuser2","status":"offline"}]}}
```

**Verification:**
- Should list all friends
- Status should reflect if friend is online

### Test 9.5: Delete Friend

**Request:**
```json
{"requestType":"DEL_FRIEND","data":{"authToken":"<testuser1-token>","friendUsername":"testuser2"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"Friend removed successfully"}}
```

**Verification:**
```bash
# Check friendship removed
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM friendships WHERE (user1_id = (SELECT id FROM users WHERE username = 'testuser1') AND user2_id = (SELECT id FROM users WHERE username = 'testuser2'));"
# Should show: 0
```

---

## Test Case 10: Integration with Server Handlers Works ✅

### Test 10.1: Complete Game Flow

**Full Game Session Test:**

1. **Register User:**
   ```json
   {"requestType":"REGISTER","data":{"username":"gametest","password":"GameTest123"}}
   ```

2. **Login:**
   ```json
   {"requestType":"LOGIN","data":{"username":"gametest","password":"GameTest123"}}
   ```
   Save `authToken`

3. **Start Game:**
   ```json
   {"requestType":"START","data":{"authToken":"<token>"}}
   ```
   Save `gameId`

4. **Use Lifeline (50/50):**
   ```json
   {"requestType":"LIFELINE","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"lifelineType":"5050"}}
   ```

5. **Answer Question 1 Correctly:**
   ```json
   {"requestType":"ANSWER","data":{"authToken":"<token>","gameId":<id>,"questionNumber":1,"answerIndex":<correct>}}
   ```

6. **Answer Question 2 Correctly:**
   ```json
   {"requestType":"ANSWER","data":{"authToken":"<token>","gameId":<id>,"questionNumber":2,"answerIndex":<correct>}}
   ```

7. **Give Up:**
   ```json
   {"requestType":"GIVE_UP","data":{"authToken":"<token>","gameId":<id>,"questionNumber":3}}
   ```

**Verification:**
```bash
# Check complete game session
psql -U postgres -d millionaire_game -c "SELECT id, status, current_question_number, total_score, final_prize, ended_at FROM game_sessions WHERE user_id = (SELECT id FROM users WHERE username = 'gametest') ORDER BY id DESC LIMIT 1;"
# Should show:
# - status: 'quit'
# - current_question_number: 3
# - total_score: > 0
# - final_prize: 4000000 (prize for question 2)
# - ended_at: timestamp set

# Check answers recorded
psql -U postgres -d millionaire_game -c "SELECT question_order, selected_option, is_correct FROM game_answers WHERE game_id = <game-id> ORDER BY question_order;"
# Should show answers for questions 1 and 2
```

### Test 10.2: User Info

**Request:**
```json
{"requestType":"USER_INFO","data":{"authToken":"<token>","username":"gametest"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"username":"gametest","totalGames":<count>,"highestPrize":<prize>,"finalQuestionNumber":<1-15>,"totalScore":<score>}}
```

**Verification:**
```bash
# Check user's game history
psql -U postgres -d millionaire_game -c "SELECT COUNT(*), MAX(final_prize), MAX(current_question_number) FROM game_sessions WHERE user_id = (SELECT id FROM users WHERE username = 'gametest');"
# Should match response data
```

### Test 10.3: View History

**Request:**
```json
{"requestType":"VIEW_HISTORY","data":{"authToken":"<token>"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"games":[{"gameId":<id>,"date":<timestamp>,"finalQuestionNumber":<1-15>,"totalScore":<score>,"finalPrize":<prize>,"status":"<active/won/lost/quit>"},...]}}
```

**Verification:**
```bash
# Check game history
psql -U postgres -d millionaire_game -c "SELECT id, status, current_question_number, total_score, final_prize, started_at FROM game_sessions WHERE user_id = (SELECT id FROM users WHERE username = 'gametest') ORDER BY started_at DESC;"
# Should match response games array
```

---

## Test Case 11: Admin Operations ✅

### Test 11.1: Create Admin User

**Setup:**
```bash
# Create admin user in database
psql -U postgres -d millionaire_game -c "UPDATE users SET role = 'admin' WHERE username = 'testuser1';"
```

### Test 11.2: Add Question (Admin)

**Request:**
```json
{"requestType":"ADD_QUES","data":{"authToken":"<admin-token>","question":"What is 2+2?","options":[{"label":"A","text":"3"},{"label":"B","text":"4"},{"label":"C","text":"5"},{"label":"D","text":"6"}],"correctAnswer":1,"level":0}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"questionId":<id>,"message":"Question added successfully"}}
```

**Verification:**
```bash
# Check question added
psql -U postgres -d millionaire_game -c "SELECT id, question_text, option_a, option_b, option_c, option_d, correct_answer, level FROM questions WHERE id = <question-id>;"
# Should show new question with all fields
```

### Test 11.3: View Questions (Admin)

**Request:**
```json
{"requestType":"VIEW_QUES","data":{"authToken":"<admin-token>","page":1,"limit":10,"level":0}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"questions":[{"questionId":<id>,"question":"<text>","level":0},...],"total":<count>,"page":1}}
```

**Verification:**
- Should list questions for level 0
- Count should match database

### Test 11.4: Ban User (Admin)

**Request:**
```json
{"requestType":"BAN_USER","data":{"authToken":"<admin-token>","username":"testuser2","reason":"Test ban"}}
```

**Expected Response:**
```json
{"responseCode":200,"data":{"message":"User banned successfully"}}
```

**Verification:**
```bash
# Check user banned
psql -U postgres -d millionaire_game -c "SELECT is_banned, ban_reason FROM users WHERE username = 'testuser2';"
# Should show: is_banned = true, ban_reason = 'Test ban'

# Try to login as banned user
# Should get: {"responseCode":403,"error":"Account is banned"}
```

---

## Quick Test Script

Save this as `test_client.sh`:

```bash
#!/bin/bash

SERVER="localhost"
PORT="8080"

# Function to send JSON request
send_request() {
    echo "$1" | nc $SERVER $PORT
}

echo "=== Test 1: Register ==="
send_request '{"requestType":"REGISTER","data":{"username":"testuser","password":"TestPass123"}}'

echo -e "\n=== Test 2: Login ==="
TOKEN=$(send_request '{"requestType":"LOGIN","data":{"username":"testuser","password":"TestPass123"}}' | grep -o '"authToken":"[^"]*"' | cut -d'"' -f4)
echo "Token: $TOKEN"

echo -e "\n=== Test 3: Start Game ==="
GAME_ID=$(send_request "{\"requestType\":\"START\",\"data\":{\"authToken\":\"$TOKEN\"}}" | grep -o '"gameId":[0-9]*' | cut -d':' -f2)
echo "Game ID: $GAME_ID"

echo -e "\n=== Test 4: Use Lifeline ==="
send_request "{\"requestType\":\"LIFELINE\",\"data\":{\"authToken\":\"$TOKEN\",\"gameId\":$GAME_ID,\"questionNumber\":1,\"lifelineType\":\"5050\"}}"

echo -e "\n=== Test 5: Answer Question ==="
send_request "{\"requestType\":\"ANSWER\",\"data\":{\"authToken\":\"$TOKEN\",\"gameId\":$GAME_ID,\"questionNumber\":1,\"answerIndex\":1}}"
```

**Usage:**
```bash
chmod +x test_client.sh
./test_client.sh
```

---

## Expected Database State After Tests

After running all tests, verify:

```bash
# Users table
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM users;"
# Should have: testuser1, testuser2, gametest, testuser, etc.

# Game sessions
psql -U postgres -d millionaire_game -c "SELECT COUNT(*), status FROM game_sessions GROUP BY status;"
# Should show various statuses: active, won, lost, quit

# Game answers
psql -U postgres -d millionaire_game -c "SELECT COUNT(*), is_correct FROM game_answers GROUP BY is_correct;"
# Should show both correct and incorrect answers

# Friendships
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM friendships;"
# Should show friend relationships

# Leaderboard
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM leaderboard;"
# Should show users with game scores
```

---

## Troubleshooting

### Server won't start
- Check PostgreSQL is running: `psql -U postgres -d postgres -c "SELECT 1;"`
- Check config.json exists and has correct credentials
- Check port 8080 is not in use: `lsof -i :8080`

### Database connection fails
- Verify postgres user exists: `psql -U postgres -d postgres -c "\du postgres;"`
- Check database exists: `psql -U postgres -d postgres -c "\l millionaire_game;"`

### No response from server
- Check server logs: `tail -f server.log`
- Verify JSON format is correct
- Check authToken is valid

### Questions not found
- Verify mock data loaded: `psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM questions;"`
- Check questions are active: `psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM questions WHERE is_active = true;"`

