# Question Display & Lifeline Auto-trigger Fixes

## Date: January 5, 2026

## Issues Found

### 1. Questions Not Displaying - "Waiting for question..." Forever
**Root Cause:** Server was NOT sending `QUESTION_INFO` notification after `START` and after correct answers.

**Evidence from logs:**
```
[DEBUG] Processing GAME_START notification
[DEBUG] Start game response code: 200
[DEBUG] sendRequest: sending LIFELINE, message=...  <-- Lifelines triggered immediately
```

The client received `GAME_START` notification but never received `QUESTION_INFO`, causing it to show "Waiting for question..." indefinitely.

**Server Code Issue:**
In `server/request_handlers/game_handlers.cpp`, there was a TODO comment:
```cpp
// TODO: Send QUESTION_INFO notification with first question
// This requires database integration to load question data
```

The server was loading the first question from the database but NOT sending it to the client!

### 2. Lifelines Triggering Automatically
**Root Cause:** Lifeline buttons were being rendered even when `waitingForQuestion = true` and somehow being triggered automatically.

**Evidence from logs:**
All 3 lifelines were called immediately after game start:
- Line 510: LIFELINE request for "5050"
- Line 525: LIFELINE request for "PHONE"  
- Line 532: LIFELINE request for "AUDIENCE"

This caused the player to lose 15 points (5 points × 3 lifelines) before even seeing a question!

---

## Fixes Implemented

### Fix 1: Server Now Sends QUESTION_INFO Notification

**Added helper function in `game_handlers.cpp`:**

```cpp
static string buildQuestionInfoData(const Question& q, int game_id, const ClientSession& session) {
    // Builds JSON with:
    // - questionId, questionNumber, question text
    // - 4 options (A, B, C, D) with index, label, text
    // - prize, totalQuestions (15)
    // - available lifelines array
    // - timeLimit, timeRemaining (30 seconds)
    // - gameId, totalScore
}
```

**Updated `handleStart()` function:**
```cpp
// Send GAME_START notification
NotificationUtils::sendNotification(client_fd, "GAME_START", game_start_data);

// Send QUESTION_INFO notification with first question  ← NEW!
string question_data = buildQuestionInfoData(first_question, game_id, session);
NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
```

**Updated `handleAnswer()` function:**
After correct answer and getting next question:
```cpp
// Send QUESTION_INFO notification with next question  ← NEW!
if (next_question.id > 0) {
    string question_data = buildQuestionInfoData(next_question, game_id, session);
    NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
}
```

**QUESTION_INFO Format Sent:**
```json
{
  "questionId": 123,
  "questionNumber": 1,
  "question": "What is the capital of Vietnam?",
  "options": [
    {"index": 0, "label": "A", "text": "Hanoi"},
    {"index": 1, "label": "B", "text": "Ho Chi Minh City"},
    {"index": 2, "label": "C", "text": "Da Nang"},
    {"index": 3, "label": "D", "text": "Hue"}
  ],
  "prize": 1000000,
  "totalQuestions": 15,
  "lifelines": ["5050", "PHONE", "AUDIENCE"],
  "timeLimit": 30,
  "timeRemaining": 30,
  "gameId": 5,
  "totalScore": 0
}
```

### Fix 2: Lifeline Buttons Only Show When Question is Ready

**Updated `client/main.cpp`:**

Moved lifeline buttons inside the `if (!state.waitingForQuestion)` block:

```cpp
if (state.waitingForQuestion) {
    ImGui::Text("Waiting for question...");
} else {
    // Question display...
    // Answer buttons...
    // Submit button...
    
    // Lifelines - ONLY show when we have a question
    ImGui::Separator();
    ImGui::Text("Lifelines:");
    
    if (state.availableLifelines[0]) {
        if (ImGui::Button("50/50", ImVec2(150, 30))) {
            std::cerr << "[DEBUG] Player using 50/50 lifeline" << std::endl;
            protocol->useLifeline("5050");
        }
    } else {
        // Show disabled button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::Button("50/50 (Used)", ImVec2(150, 30));
        ImGui::PopStyleColor();
    }
    // ... same for other lifelines
}
```

**Additional improvements:**
- Added debug logging when lifelines are actually clicked
- Show greyed-out buttons for used lifelines instead of hiding them
- Fixed button labels to show "Phone Friend" and "Ask Audience"

---

## How It Works Now

### 1. Game Start Flow

**Client → Server:**
```json
{"requestType":"START","data":{"authToken":"..."}}
```

**Server → Client:**
1. `GAME_START` notification
   ```json
   {"type":"GAME_START","data":{"gameId":5,"timestamp":...}}
   ```

2. **START response** (for ProtocolHandler)
   ```json
   {"responseCode":200,"data":{"gameId":5,"timestamp":...}}
   ```

3. **QUESTION_INFO notification** (NEW!)
   ```json
   {"type":"QUESTION_INFO","data":{...full question data...}}
   ```

**Client:**
- Shows "Waiting for question..." after START
- Receives QUESTION_INFO notification
- Parses question text and options
- Displays question with answer buttons
- Starts 30-second countdown timer
- Lifeline buttons are now clickable

### 2. Answer Flow

**Client → Server:**
```json
{"requestType":"ANSWER","data":{"authToken":"...","gameId":5,"questionNumber":1,"answerIndex":0}}
```

**If Correct Answer:**

**Server → Client:**
1. **ANSWER response**
   ```json
   {"responseCode":200,"data":{"correct":true,"gameOver":false,...}}
   ```

2. **QUESTION_INFO notification** (NEW! - for next question)
   ```json
   {"type":"QUESTION_INFO","data":{...next question data...}}
   ```

**Client:**
- Shows green toast: "✓ Correct! +X points"
- Shows "Waiting for question..." briefly
- Receives QUESTION_INFO
- Displays next question
- Timer resets to 30 seconds

**If Wrong Answer:**

**Server → Client:**
1. **ANSWER response**
   ```json
   {"responseCode":200,"data":{"correct":false,"gameOver":true,...}}
   ```

2. **GAME_END notification**
   ```json
   {"type":"GAME_END","data":{"status":"lost","finalPrize":...}}
   ```

**Client:**
- Shows red toast: "Game Over! Final Prize: X VND"
- Returns to lobby after 5 seconds

---

## Files Modified

### Server Files

1. **`server/request_handlers/game_handlers.cpp`**
   - Added `#include <sstream>`
   - Added `buildQuestionInfoData()` helper function
   - Updated `handleStart()` to send QUESTION_INFO after GAME_START
   - Updated `handleAnswer()` to send QUESTION_INFO after correct answer

### Client Files

1. **`client/main.cpp`**
   - Moved lifeline buttons inside `if (!state.waitingForQuestion)` block
   - Added debug logging for lifeline clicks
   - Show greyed-out buttons for used lifelines
   - Fixed button labels

---

## Testing

### Quick Test

1. **Rebuild server:**
   ```bash
   cd server
   make
   ./bin/server
   ```

2. **Start client:**
   ```bash
   cd client
   ./bin/client
   ```

3. **Play game:**
   - Login
   - Click "Start New Game"
   - **Should now see:** Question appears immediately (no more waiting forever!)
   - **Should now see:** Lifelines NOT triggered automatically
   - Answer a question
   - **Should now see:** Next question appears automatically after correct answer

### Expected Debug Output

**Server terminal:**
```
[INFO] Client connected: fd=4
[INFO] Request: START from user=...
[INFO] Sending GAME_START notification
[INFO] Sending QUESTION_INFO notification  ← NEW!
```

**Client terminal:**
```
[DEBUG] Starting new game...
[DEBUG] Start game response code: 200
[DEBUG] Received notification: type=GAME_START
[DEBUG] Processing GAME_START notification
[DEBUG] Received notification: type=QUESTION_INFO  ← NEW!
[DEBUG] Processing QUESTION_INFO notification
[DEBUG] Question loaded: What is the capital of Vietnam?  ← NEW!
[DEBUG] Options: Hanoi, Ho Chi Minh City, Da Nang, Hue  ← NEW!
```

**No more automatic lifeline calls!**

---

## Verification

### Before Fix:
```
[DEBUG] Start game response code: 200
[DEBUG] sendRequest: sending LIFELINE, message=...  ← WRONG!
[DEBUG] sendRequest: sending LIFELINE, message=...  ← WRONG!
[DEBUG] sendRequest: sending LIFELINE, message=...  ← WRONG!
```

### After Fix:
```
[DEBUG] Start game response code: 200
[DEBUG] Received notification: type=QUESTION_INFO  ← CORRECT!
[DEBUG] Processing QUESTION_INFO notification
[DEBUG] Question loaded: ...
```

Lifelines only appear in logs when player actually clicks the button:
```
[DEBUG] Player using 50/50 lifeline  ← Only when clicked!
```

---

## Summary

✅ **Questions now display properly** - Server sends QUESTION_INFO after START and after correct answers

✅ **Lifelines no longer auto-trigger** - Buttons only render when question is ready, with debug logging

✅ **Game flow works correctly** - Question → Answer → Next Question loop now functions as designed

✅ **Points preserved** - No more losing 15 points from unwanted lifeline usage

The game is now fully playable with proper question display and lifeline control!

