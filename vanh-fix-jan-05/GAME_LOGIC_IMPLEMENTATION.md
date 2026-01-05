# Game Logic Implementation - Client GUI

## Summary

Implemented full game logic for the client GUI to interact with the server for the "Who Wants to be a Millionaire" game. The client now properly receives questions from the server, displays them with a countdown timer, submits answers, and handles results.

## Date: January 5, 2026

---

## What Was Implemented

### 1. Server Communication Flow

**Game Start:**
- Client sends `START` request to server
- Server responds with success (200)
- Server sends `GAME_START` notification
- Server sends `QUESTION_INFO` notification with first question
- Client displays "Waiting for question..." until QUESTION_INFO arrives

**Question Display:**
- Client receives `QUESTION_INFO` notification containing:
  - Question text
  - 4 options (A, B, C, D)
  - Question number (1-15)
  - Time remaining (30 seconds)
  - Current prize
  - Total score
  - Game ID

**Answer Submission:**
- User selects an answer (button turns green)
- User clicks "Submit Answer"
- Client sends `ANSWER` request with:
  - authToken
  - gameId
  - questionNumber
  - answerIndex (0-3)

**Response Handling:**
- **Correct Answer, Not Game Over:**
  - Show green toast: "✓ Correct! +X points"
  - Display "Waiting for question..."
  - Server sends next `QUESTION_INFO` notification
  - Timer restarts for new question

- **Wrong Answer (Game Over):**
  - Server sends `GAME_END` notification
  - Show red toast: "Game Over! Final Prize: X VND"
  - Return to lobby
  - User can start new game

- **Correct Answer, Game Won (Question 15):**
  - Server sends `GAME_END` notification with isWinner=true
  - Show green toast: "🎉 Congratulations! You WON! Prize: 1,000,000,000 VND"
  - Return to lobby

- **Timeout:**
  - Client-side timer reaches 0, OR
  - Server responds with 408 error code
  - Show red toast: "⏰ Time's up! Game Over!"
  - Return to lobby

**Give Up:**
- User clicks "Give Up" button
- Client sends `GIVE_UP` request
- Server sends `GAME_END` notification with status="quit"
- Show yellow toast: "You gave up. Prize taken: X VND"
- Return to lobby

---

## Key Features Implemented

### 1. Question Parser

```cpp
std::vector<std::string> parseOptionsArray(const std::string& json)
```

- Parses the `options` array from QUESTION_INFO notification
- Extracts the "text" field from each option object
- Returns vector of 4 option strings
- Falls back to placeholder text if parsing fails

**QUESTION_INFO Format:**
```json
{
  "type": "QUESTION_INFO",
  "data": {
    "questionNumber": 1,
    "question": "What is the capital of Vietnam?",
    "options": [
      {"index": 0, "label": "A", "text": "Hanoi"},
      {"index": 1, "label": "B", "text": "Ho Chi Minh City"},
      {"index": 2, "label": "C", "text": "Da Nang"},
      {"index": 3, "label": "D", "text": "Hue"}
    ],
    "prize": 1000000,
    "timeRemaining": 30,
    "gameId": 12345,
    "totalScore": 0
  }
}
```

### 2. Countdown Timer

```cpp
void updateTimer(GameState& state, ProtocolHandler* protocol)
```

- Runs in separate thread
- Counts down from timeRemaining to 0
- Controlled by `state.timerRunning` flag
- Stops when:
  - New question arrives
  - Answer is submitted
  - Game ends
- Triggers timeout message when reaching 0

**Timer Features:**
- Red text when ≤10 seconds
- Orange text when ≤20 seconds
- White text when >20 seconds
- Automatically restarts for each new question

### 3. Notification Handler

```cpp
void handleNotifications(SocketClient* client, GameState& state, ProtocolHandler* protocol)
```

**Handles Three Types of Notifications:**

1. **GAME_START:**
   - Sets `waitingForQuestion = true`
   - Resets lifelines
   - Waits for QUESTION_INFO

2. **QUESTION_INFO:**
   - Parses question and options
   - Extracts game data (questionNumber, prize, score, timer)
   - Stops old timer, starts new timer
   - Sets `waitingForQuestion = false`
   - Resets selected answer

3. **GAME_END:**
   - Stops timer
   - Extracts status, finalPrize, isWinner
   - Shows appropriate result message
   - Returns to lobby

### 4. Toast Notification System

**Features:**
- Centered at top of screen
- Semi-transparent background
- Large font (1.5x)
- Color-coded messages:
  - Green: Correct answers, winning
  - Red: Wrong answers, game over, timeout
  - Yellow: Give up, other info
- Auto-dismisses after configurable time (2-5 seconds)

**Toast Messages:**
- "✓ Correct! +X points" (2 seconds)
- "🎉 Congratulations! You WON! Prize: X VND" (5 seconds)
- "Game Over! Final Prize: X VND" (5 seconds)
- "⏰ Time's up! Game Over!" (5 seconds)
- "You gave up. Prize taken: X VND" (5 seconds)

### 5. UI Improvements

**Question Display:**
- Question number: "Question X of 15"
- Color-coded timer display
- Current prize in VND
- Current score in points
- Question text (wrapped for long questions)

**Answer Buttons:**
- 4 large buttons (A, B, C, D)
- Green highlight when selected
- Full option text displayed

**Game State Indicators:**
- "Waiting for question..." during transitions
- Prize ladder on left side
- Current question highlighted in green
- Safe checkpoints (5, 10, 15) highlighted in yellow

---

## Testing Instructions

### Prerequisites

1. **Start PostgreSQL and setup database:**
   ```bash
   cd "/Users/vanhtran18/Documents/Study at school/Network Programming"
   psql -U postgres -d millionaire_game < database/schema.sql
   psql -U postgres -d millionaire_game < database/mock_data.sql
   ```

2. **Start the server:**
   ```bash
   cd server
   make clean && make
   ./bin/server
   ```

3. **Build the client:**
   ```bash
   cd client
   make clean && make
   ```

### Test Scenario 1: Full Game Flow

1. **Start client:**
   ```bash
   cd client
   ./bin/client
   ```

2. **Register/Login:**
   - Register new user: username="testplayer", password="TestPass123"
   - Login with same credentials

3. **Start Game:**
   - Click "Start New Game" button
   - Should see "Waiting for question..." briefly
   - First question should appear with 30-second timer

4. **Answer Questions:**
   - Question appears with 4 options
   - Timer counts down from 30
   - Timer turns orange at 20s, red at 10s
   - Select answer by clicking button (turns green)
   - Click "Submit Answer"

5. **Expected Results:**

   **Correct Answer:**
   - Green toast: "✓ Correct! +X points"
   - Toast disappears after 2 seconds
   - Shows "Waiting for question..."
   - Next question appears automatically
   - Timer resets to 30 seconds

   **Wrong Answer:**
   - Red toast: "Game Over! Final Prize: X VND"
   - Toast shows for 5 seconds
   - Returns to lobby
   - Can start new game

### Test Scenario 2: Give Up

1. Start game and get to any question
2. Click "Give Up" button
3. Should see yellow toast: "You gave up. Prize taken: X VND"
4. Returns to lobby after 5 seconds

### Test Scenario 3: Timeout

1. Start game and get to any question
2. Wait for timer to reach 0 (don't answer)
3. Should see red toast: "⏰ Time's up! Game Over!"
4. Returns to lobby after 5 seconds

### Test Scenario 4: Win Game

1. Answer all 15 questions correctly (use database to check correct answers)
2. After question 15, should see:
   - Green toast: "🎉 Congratulations! You WON! Prize: 1,000,000,000 VND"
   - Returns to lobby after 5 seconds

### Verify Questions from Database

To know the correct answers, check the database:

```bash
# Get questions assigned to a game
psql -U postgres -d millionaire_game -c "
SELECT 
    gq.question_order,
    q.question_text,
    q.option_a,
    q.option_b,
    q.option_c,
    q.option_d,
    q.correct_answer
FROM game_questions gq
JOIN questions q ON gq.question_id = q.id
WHERE gq.game_id = <your-game-id>
ORDER BY gq.question_order;
"
```

Replace `<your-game-id>` with the game ID shown in server logs or from START response.

---

## Debug Output

The implementation includes debug output to stderr for troubleshooting:

```
[DEBUG] Received notification: type=QUESTION_INFO, data=...
[DEBUG] Processing QUESTION_INFO notification
[DEBUG] Question loaded: What is the capital of Vietnam?
[DEBUG] Options: Hanoi, Ho Chi Minh City, Da Nang, Hue
[DEBUG] Submitting answer: 0
[DEBUG] Answer response code: 200
[DEBUG] Processing GAME_END notification
```

Monitor the terminal running the client to see these debug messages.

---

## Files Modified

### client/main.cpp

**Changes:**

1. **Added to GameState struct:**
   - `waitingForQuestion` - flag for waiting for QUESTION_INFO
   - `showResultMessage` - flag to show toast
   - `resultMessage` - message to display in toast
   - `resultMessageTime` - countdown for toast auto-dismiss
   - `timerRunning` - flag to control timer thread

2. **New function: `parseOptionsArray()`**
   - Parses options array from QUESTION_INFO JSON
   - Extracts "text" field from each option

3. **Updated function: `updateTimer()`**
   - Now controlled by `timerRunning` flag
   - Handles timeout properly
   - Cleaner thread management

4. **Updated function: `handleNotifications()`**
   - Properly parses QUESTION_INFO
   - Extracts real question text and options
   - Manages timer lifecycle
   - Handles GAME_START notification
   - Handles GAME_END notification with status
   - Added debug output

5. **Updated UI: Game Screen**
   - Shows "Waiting for question..." during transitions
   - Displays actual question from server
   - Color-coded timer display
   - Better answer button styling
   - Proper answer submission handling
   - Waits for next question after correct answer

6. **New UI: Toast Notification System**
   - Centered at top of screen
   - Color-coded based on message type
   - Auto-dismisses after timer
   - Large font for visibility

7. **Updated: START button handler**
   - Initializes game state properly
   - Sets waitingForQuestion flag
   - Clears previous game data

---

## Protocol Compliance

All interactions now follow the protocol specification exactly:

✅ **START Request:**
- Sends: `{"requestType":"START","data":{"authToken":"..."}}`
- Receives: `{"responseCode":200,"data":{"gameId":123,"timestamp":...}}`
- Waits for: GAME_START notification → QUESTION_INFO notification

✅ **ANSWER Request:**
- Sends: `{"requestType":"ANSWER","data":{"authToken":"...","gameId":123,"questionNumber":1,"answerIndex":0}}`
- Receives: `{"responseCode":200,"data":{"correct":true,"gameOver":false,...}}`
- Waits for: QUESTION_INFO notification (if not game over)

✅ **GIVE_UP Request:**
- Sends: `{"requestType":"GIVE_UP","data":{"authToken":"...","gameId":123,"questionNumber":1}}`
- Receives: `{"responseCode":200,"data":{"finalPrize":...}}`
- Waits for: GAME_END notification

✅ **Notifications Handled:**
- GAME_START
- QUESTION_INFO
- GAME_END
- LIFELINE_INFO (stub for future)

---

## Known Limitations

1. **Lifelines:** UI buttons exist but functionality is not fully implemented (waiting for future enhancement)
2. **Resume Game:** Button exists but not fully tested with saved games
3. **Prize Ladder:** Display is hardcoded, should match server's prize calculation
4. **Network Errors:** Basic error handling, could be more robust

---

## Next Steps (Future Enhancements)

1. **Implement Lifelines:**
   - 50/50: Remove 2 wrong answers
   - Phone a Friend: Show suggestion
   - Ask Audience: Show poll results

2. **Add More UI Polish:**
   - Sound effects
   - Animations for correct/wrong answers
   - Better transition effects
   - Progress bar for timer

3. **Add Resume Game Support:**
   - Test with saved games
   - Show saved game details before resuming

4. **Network Error Handling:**
   - Reconnection logic
   - Better timeout handling
   - Show connection status

5. **Game History:**
   - View past games
   - Statistics display
   - Leaderboard integration

---

## Conclusion

The client now has a fully functional game loop that:
- ✅ Connects to server
- ✅ Authenticates users
- ✅ Starts games
- ✅ Receives questions from server
- ✅ Displays questions with proper formatting
- ✅ Shows countdown timer
- ✅ Submits answers to server
- ✅ Handles correct/wrong answers properly
- ✅ Shows result messages (toasts)
- ✅ Transitions between questions
- ✅ Handles game over scenarios
- ✅ Returns to lobby after game ends

The implementation follows the protocol specification exactly and has been tested with the server implementation referenced in `database/TEST_GUIDE.md`.

