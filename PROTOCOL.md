# Game Protocol Specification

## Overview

This document defines the communication protocol between Client, Server, and Admin for the Millionaire Game application. All messages are JSON formatted and sent over TCP sockets with newline ('\n') as delimiter.

## Message Format

### Request Format (Client to Server)
```json
{
  "requestType": "REQUEST_TYPE",
  "data": {
    // Request-specific data
  }
}
```

### Response Format (Server to Client)
```json
{
  "responseCode": 200,
  "data": {
    // Response data
  }
}
```

Or for errors:
```json
{
  "responseCode": 401,
  "message": "Error message"
}
```

## Authentication

Most requests (except LOGIN, REGISTER, and CONNECTION) require an authentication token. Include `authToken` in the `data` field:

```json
{
  "requestType": "START",
  "data": {
    "authToken": "your_token_here",
    // other fields
  }
}
```

---

## Connection and Reconnection Flow

### Initial Connection
1. Client connects to server via TCP socket
2. Server sends CONNECTION notification to confirm connection
3. Client can proceed with LOGIN/REGISTER

### After Login/Register
1. Client receives `authToken` after successful LOGIN
2. Client should check if user has auto-saved game (optional, can check via RESUME)
3. Client can START new game or RESUME saved game

### Reconnection After Disconnect
1. Client reconnects to server (same or new connection)
2. Client uses same `authToken` to authenticate
3. Server maintains user's game state (if any)
4. Client has two options:
   - **Check for saved game**: Send RESUME request (returns 404 if no saved game)
   - **Start fresh**: Send START request with `overrideSavedGame: true`
5. If user has auto-saved game and tries START without override, server returns error 412

### Game State Persistence
- Game state persists across disconnections (auto-saved)
- Auto-saved game remains until:
  - User resumes it successfully
  - User starts new game with override
  - Game ends (if resumed and ended)
- Each user can have maximum one auto-saved game

---

## Client to Server Requests

### Authentication

#### LOGIN
Authenticate user credentials to log into the game. Works for both regular users and admin accounts.

**Request:**
```json
{
  "requestType": "LOGIN",
  "data": {
    "username": "player1",
    "password": "password123"
  }
}
```

**Success Response (200) - Regular User:**
```json
{
  "responseCode": 200,
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "username": "player1",
    "role": "user",
    "message": "Login successful"
  }
}
```

**Success Response (200) - Admin:**
```json
{
  "responseCode": 200,
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "username": "admin1",
    "role": "admin",
    "message": "Login successful"
  }
}
```

**Response Fields:**
- `authToken`: Authentication token for all requests (game requests and admin operations). Same token is used for both regular users and admins.
- `username`: Username of logged-in account
- `role`: User role - `"user"` for regular players, `"admin"` for admin accounts

**Note:**
- Both regular users and admins use the same `authToken` format.
- Client should check `role` field to determine UI/functionality (show admin panel if role is "admin").
- For admin requests, server validates `authToken` and checks if the username has admin role. If not admin, returns error 403 (FORBIDDEN).
- Admin accounts are separate from regular user accounts in the database (different role assignment).

**Error Responses:**
- 400: Invalid data format or missing username/password (INVALID_DATA)
- 401: Invalid credentials (LOGIN_FAILED)
- 403: Account is banned (FORBIDDEN)

---

#### REGISTER
Create a new account for a player.

**Request:**
```json
{
  "requestType": "REGISTER",
  "data": {
    "username": "newplayer",
    "password": "securepass123"
  }
}
```

**Success Response (201):**
```json
{
  "responseCode": 201,
  "data": {
    "username": "newplayer",
    "message": "Registration successful. Please login to continue."
  }
}
```

**Note:** After successful registration, user must use LOGIN to get authToken.

**Error Responses:**
- 400: Invalid data format or missing fields
- 409: Username already exists (CONFLICT)
- 410: Weak password (WEAK_PASSWORD)

---

#### LOGOUT
End the user session and disconnect from the server.

**Request:**
```json
{
  "requestType": "LOGOUT",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Logout successful"
  }
}
```

---

### Game Actions

#### START
Request to start a new game session.

**Request:**
```json
{
  "requestType": "START",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "overrideSavedGame": false
  }
}
```

**Optional Field:**
- `overrideSavedGame`: If `true` and user has an auto-saved game, it will be discarded and a new game starts. Default: `false`

**Success Response (200):**
Server sends GAME_START notification followed by QUESTION_INFO with the first question.

**Game State Management:**
- If user has an active game (in progress), returns error 405
- If user has an auto-saved game and `overrideSavedGame` is `false`, returns error 412 with message "You have a saved game. Use RESUME to continue or set overrideSavedGame=true to start new game"
- If previous game ended (won/lost/GIVE_UP), game state is automatically cleared and new game can start
- Starting a new game clears any auto-saved game

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 405: User already in a game (USER_ALREADY_IN_GAME)
- 412: Invalid game state - user has auto-saved game (INVALID_GAME_STATE)

---

#### ANSWER
Send user's selected answer to the current question.

**Request:**
```json
{
  "requestType": "ANSWER",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "gameId": 12345,
    "questionNumber": 1,
    "answerIndex": 0
  }
}
```

**Required Fields:**
- `authToken`: Authentication token
- `gameId`: Game session identifier (from GAME_START or QUESTION_INFO)
- `questionNumber`: Current question number (1-15)
- `answerIndex`: Selected answer index (0-3)

**Server Validation:**
- Server validates `gameId` matches user's active game
- Server validates `questionNumber` matches current question in the game
- If `gameId` doesn't match active game → error 412 (INVALID_GAME_STATE)
- If `questionNumber` doesn't match → error 422 (UNPROCESSABLE_DATA)

**Success Response (200) - Correct Answer:**
```json
{
  "responseCode": 200,
  "data": {
    "gameId": 12345,
    "correct": true,
    "questionNumber": 1,
    "timeRemaining": 15,
    "pointsEarned": 15,
    "totalScore": 15,
    "currentPrize": 2000000,
    "gameOver": false,
    "isWinner": false
  }
}
```

**For incorrect answer:**
```json
{
  "responseCode": 200,
  "data": {
    "gameId": 12345,
    "correct": false,
    "questionNumber": 8,
    "correctAnswer": 2,
    "pointsEarned": 0,
    "safeCheckpointPrize": 10000000,
    "safeCheckpointScore": 150,
    "totalScore": 150,
    "finalPrize": 10000000,
    "gameOver": true,
    "isWinner": false
  }
}
```

**Safe Checkpoint Rules:**
- Safe checkpoints are at questions: 5, 10, 15 (typical Millionaire game structure)
- When user answers incorrectly or times out:
  - `safeCheckpointPrize`: Prize from the last safe checkpoint passed (e.g., if failed at question 8, checkpoint is question 5)
  - `safeCheckpointScore`: Total score accumulated up to and including the last safe checkpoint
  - `finalPrize`: Same as `safeCheckpointPrize` (prize user receives)
  - `totalScore`: Same as `safeCheckpointScore` (final score for this game)
- If user fails before first safe checkpoint (question 5), all values are 0

**For winning (answered all 15 questions correctly):**
```json
{
  "responseCode": 200,
  "data": {
    "gameId": 12345,
    "correct": true,
    "questionNumber": 15,
    "timeRemaining": 10,
    "pointsEarned": 10,
    "totalScore": 450,
    "currentPrize": 1000000000,
    "gameOver": true,
    "isWinner": true
  }
}
```

**Scoring Rules:**
- Maximum 30 points per question
- Each second elapsed reduces points by 1
- Using any lifeline reduces points by 5 (minimum 0)
- Points = max(0, timeRemaining - lifelinePenalty)
- Example: 15s remaining, used 1 lifeline → 15 - 5 = 10 points

**Game State After ANSWER:**
- If answer is correct and game continues: User remains "in game"
- If answer is correct and all 15 questions answered: Game ends with status "won", state cleared, user can START new game
- If answer is incorrect: Game ends with status "lost", player receives safe checkpoint prize and score, GAME_END notification sent, state cleared, user can START new game
- If timeout occurs: Game ends with status "lost" (same as wrong answer), player receives safe checkpoint prize and score, GAME_END notification sent, state cleared, user can START new game

**Game Session Identification:**
- Each game session has a unique `gameId` assigned by server when game starts
- Client receives `gameId` in GAME_START and QUESTION_INFO notifications
- Client MUST include `gameId` in all game-related requests (ANSWER, LIFELINE, GIVE_UP)
- Server validates `gameId` to ensure request belongs to the correct game session
- This prevents confusion when user plays multiple games (e.g., Game 1 question 1 vs Game 2 question 1)
- If client sends request with wrong/expired `gameId`, server returns error 412

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 406: User not in a game (USER_NOT_IN_GAME)
- 408: Question timeout (QUESTION_TIMEOUT) - Game ends with status "lost", player receives safe checkpoint prize, state cleared
- 412: Invalid gameId - gameId doesn't match active game (INVALID_GAME_STATE)
- 422: Invalid answerIndex or questionNumber mismatch (UNPROCESSABLE_DATA)

---

#### LIFELINE
Request to use a lifeline.

**Request:**
```json
{
  "requestType": "LIFELINE",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "gameId": 12345,
    "questionNumber": 1,
    "lifelineType": "5050"
  }
}
```

**Required Fields:**
- `authToken`: Authentication token
- `gameId`: Game session identifier (from GAME_START or QUESTION_INFO)
- `questionNumber`: Current question number (1-15)
- `lifelineType`: Type of lifeline to use ("5050", "PHONE", "AUDIENCE")

**Server Validation:**
- Server validates `gameId` matches user's active game
- Server validates `questionNumber` matches current question in the game
- If `gameId` doesn't match active game → error 412 (INVALID_GAME_STATE)
- If `questionNumber` doesn't match → error 422 (UNPROCESSABLE_DATA)

**Lifeline Types:**
- `5050`: Remove two wrong answers (50/50)
- `PHONE`: Call a friend
- `AUDIENCE`: Ask the audience

**Success Response (200):**
Server sends LIFELINE_INFO notification.

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 406: User not in a game (USER_NOT_IN_GAME)
- 407: Lifeline already used (LIFELINE_ALREADY_CHOSEN)
- 412: Invalid gameId - gameId doesn't match active game (INVALID_GAME_STATE)
- 422: Invalid lifelineType or questionNumber mismatch (UNPROCESSABLE_DATA)

---

#### GIVE_UP
Player quits the game and takes current winnings. Game ends and cannot be resumed.

**Request:**
```json
{
  "requestType": "GIVE_UP",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "gameId": 12345,
    "questionNumber": 1
  }
}
```

**Required Fields:**
- `authToken`: Authentication token
- `gameId`: Game session identifier (from GAME_START or QUESTION_INFO)
- `questionNumber`: Current question number (1-15)

**Server Validation:**
- Server validates `gameId` matches user's active game
- Server validates `questionNumber` matches current question in the game
- If `gameId` doesn't match active game → error 412 (INVALID_GAME_STATE)
- If `questionNumber` doesn't match → error 422 (UNPROCESSABLE_DATA)

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "finalPrize": 1000000,
    "finalQuestionNumber": 1,
    "totalScore": 25,
    "gameId": 12345,
    "message": "You gave up and took the prize."
  }
}
```

**Note:** Game ends when GIVE_UP is called. The game state is finalized and saved to leaderboard, but cannot be resumed.

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 406: User not in a game (USER_NOT_IN_GAME)
- 412: Invalid gameId - gameId doesn't match active game (INVALID_GAME_STATE)
- 422: Invalid questionNumber (UNPROCESSABLE_DATA)

---

#### RESUME
Request to continue a previously paused game (auto-saved by server when connection lost or client closed unexpectedly).

**Request:**
```json
{
  "requestType": "RESUME",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "questionNumber": 3,
    "prize": 4000000,
    "gameId": 12345,
    "totalScore": 50,
    "message": "Game resumed successfully"
  }
}
```

**Flow:**
1. Client sends RESUME request
2. Server validates and loads auto-saved game state
3. Server responds with game state (questionNumber, prize, score, etc.)
4. Server immediately sends QUESTION_INFO notification for the resumed question
5. Timer starts at 30 seconds for the resumed question
6. Client can continue playing from where they left off

**Auto-Save Mechanism:**
- Server automatically saves game state when:
  - Client connection is lost unexpectedly
  - Client closes/disconnects while in-game
  - Network timeout occurs during gameplay
- Game state is saved periodically during gameplay
- Only one saved game per user (latest game state)
- Game can only be resumed if auto-saved. If game ended due to wrong answer, timeout (treated as lost), or GIVE_UP, it cannot be resumed

**Game State After RESUME:**
- After successful RESUME, user is considered "in game"
- User cannot START new game until current game ends
- Auto-saved game is cleared after RESUME succeeds

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 404: No saved game found (NOT_FOUND)
- 405: User already in a game (USER_ALREADY_IN_GAME) - user has active game session
- 412: Invalid game state (INVALID_GAME_STATE)

---

#### LEAVE_GAME
Leave the current game session. Game state is auto-saved and can be resumed later.

**Request:**
```json
{
  "requestType": "LEAVE_GAME",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Left game successfully. Game state saved. Use RESUME to continue later."
  }
}
```

**Game State After LEAVE_GAME:**
- Game state is auto-saved (same as disconnect)
- User is no longer "in game"
- User can RESUME later to continue
- User can START new game with `overrideSavedGame: true` to discard saved game

---

### Social Features

#### LEADERBOARD
Request the global or friend ranking list.

**Request:**
```json
{
  "requestType": "LEADERBOARD",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "type": "global",
    "page": 1,
    "limit": 20
  }
}
```

**Types:**
- `global`: Global leaderboard
- `friend`: Friends leaderboard

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "rankings": [
      {
        "username": "player1",
        "finalQuestionNumber": 15,
        "totalScore": 450,
        "rank": 1,
        "isWinner": true
      },
      {
        "username": "player2",
        "finalQuestionNumber": 15,
        "totalScore": 420,
        "rank": 2,
        "isWinner": true
      },
      {
        "username": "player3",
        "finalQuestionNumber": 14,
        "totalScore": 380,
        "rank": 3,
        "isWinner": false
      }
    ],
    "total": 1000,
    "page": 1,
    "limit": 20
  }
}
```

**Ranking Rules:**
1. Primary: Sort by `finalQuestionNumber` (descending)
2. Secondary: Sort by `totalScore` (descending)

**Fields:**
- `finalQuestionNumber`: Number of questions answered correctly in the user's best game (1-15)
- `totalScore`: Total score from the user's best game (sum of points from all correct answers in that game)
- `isWinner`: true if `finalQuestionNumber` is 15 (answered all 15 questions correctly)

---

#### FRIEND_STATUS
Request the online/offline status of all friends in the user's friend list.

**Request:**
```json
{
  "requestType": "FRIEND_STATUS",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "friends": [
      {"username": "friend1", "status": "online"},
      {"username": "friend2", "status": "ingame"},
      {"username": "friend3", "status": "offline"}
    ]
  }
}
```

**Status Values:**
- `online`: Friend is online but not in a game
- `ingame`: Friend is currently playing
- `offline`: Friend is offline

---

#### ADD_FRIEND
Send a friend request to another player.

**Request:**
```json
{
  "requestType": "ADD_FRIEND",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "friendUsername": "player2"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Friend request sent successfully"
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 404: Friend not found (NOT_FOUND)
- 409: Friend request already sent or friend already exists (CONFLICT)
- 422: Cannot add yourself as friend (UNPROCESSABLE_DATA)

---

#### ACCEPT_FRIEND
Accept a friend request from another player.

**Request:**
```json
{
  "requestType": "ACCEPT_FRIEND",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "friendUsername": "player2"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Friend request accepted successfully",
    "friendUsername": "player2"
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 404: Friend request not found (NOT_FOUND)
- 409: Friend already exists (CONFLICT)

---

#### DECLINE_FRIEND
Decline a friend request from another player.

**Request:**
```json
{
  "requestType": "DECLINE_FRIEND",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "friendUsername": "player2"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Friend request declined successfully"
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 404: Friend request not found (NOT_FOUND)

---

#### FRIEND_REQ_LIST
Show list of pending friend requests.

**Request:**
```json
{
  "requestType": "FRIEND_REQ_LIST",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "friendRequests": [
      {"username": "player2", "sentAt": 1705320000},
      {"username": "player3", "sentAt": 1705315000}
    ]
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)

---

#### DEL_FRIEND
Remove a friend from the user's friend list.

**Request:**
```json
{
  "requestType": "DEL_FRIEND",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "friendUsername": "player2"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Friend removed successfully"
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 404: Friend not found (NOT_FOUND)

---

#### CHAT
Send a chat message to another player.

**Request:**
```json
{
  "requestType": "CHAT",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "recipient": "player2",
    "message": "Hello!"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Message sent successfully"
  }
}
```

**Note:** Message is delivered if recipient is online, or stored for later delivery if recipient is offline.

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 400: Missing recipient or message field (INVALID_DATA)
- 404: Recipient user not found (NOT_FOUND)
- 422: Invalid message format or empty message (UNPROCESSABLE_DATA)

---

### User Information

#### USER_INFO
Request profile details of a player.

**Request:**
```json
{
  "requestType": "USER_INFO",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "username": "player2"
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "username": "player2",
    "totalGames": 10,
    "highestPrize": 5000000,
    "finalQuestionNumber": 5,
    "totalScore": 150
  }
}
```

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 400: Missing username field (INVALID_DATA)
- 404: User not found (NOT_FOUND)

---

#### VIEW_HISTORY
Retrieve player's past game records (last 20 games).

**Request:**
```json
{
  "requestType": "VIEW_HISTORY",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "games": [
      {
        "gameId": 1001,
        "date": "2024-01-15T10:30:00",
        "finalQuestionNumber": 15,
        "totalScore": 450,
        "finalPrize": 1000000000,
        "status": "won"
      },
      {
        "gameId": 1000,
        "date": "2024-01-15T09:15:00",
        "finalQuestionNumber": 8,
        "totalScore": 200,
        "finalPrize": 10000000,
        "status": "lost"
      }
    ]
  }
}
```

**Response Details:**
- Returns maximum 20 most recent games
- Games are sorted by date (most recent first)
- If user has played fewer than 20 games, returns all available games
- Each game includes final results (questionNumber, score, prize, status)

**Status Values:**
- `won`: Player answered all 15 questions correctly - receives maximum prize
- `lost`: Player gave wrong answer or timed out - receives safe checkpoint prize from last passed checkpoint (questions 5, 10, or 15)
- `quit`: Player voluntarily gave up the game using GIVE_UP - receives current prize at time of giving up

**Differences:**
- **`won`**: Only status where player successfully completed all questions - receives maximum prize
- **`lost`**: Includes both wrong answer and timeout - receives safe checkpoint prize (not 0)
- **`quit`**: Voluntary action - receives current prize at time of giving up
- **Prize handling**: 
  - `won`: Maximum prize (1,000,000,000)
  - `lost`: Safe checkpoint prize (from last passed checkpoint: 5, 10, or 15)
  - `quit`: Current prize at time of GIVE_UP

**Error Responses:**
- 402: Missing or invalid authToken (AUTH_ERROR)

---

#### CHANGE_PASS
Change current account password. Works for both regular users and admin accounts.

**Request:**
```json
{
  "requestType": "CHANGE_PASS",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "oldPassword": "oldpass123",
    "newPassword": "newpass456"
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (from user or admin account)
- `oldPassword`: Current password
- `newPassword`: New password

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Password changed successfully"
  }
}
```

**Note:** 
- Works for both regular users and admins using the same request
- Server identifies account type from `authToken` and updates the corresponding account (user or admin)
- No role validation needed - any authenticated user can change their own password

**Error Responses:**
- 400: Missing oldPassword or newPassword (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 401: Wrong old password (LOGIN_FAILED)
- 410: Weak new password (WEAK_PASSWORD)

---

### Connection Management

#### PING
Maintain connection or check server availability.

**Request:**
```json
{
  "requestType": "PING",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "PONG"
  }
}
```

---

## Server to Client Notifications

### QUESTION_INFO
Send the next question and possible answers to the player.

**Notification:**
```json
{
  "responseCode": 200,
  "data": {
    "questionId": 1,
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
    "gameId": 12345,
    "totalScore": 0
  }
}
```

**Fields:**
- `questionId`: Unique identifier for the question in database
- `questionNumber`: Current question number (1-15)
- `totalQuestions`: Total number of questions in the game (15)
- `timeLimit`: Maximum time to answer in seconds (30)
- `timeRemaining`: Remaining time in seconds (starts at 30, decreases each second)
- `gameId`: Unique identifier for this game session
- `totalScore`: Cumulative score from previous questions

---

### LIFELINE_INFO
Send result or data after a lifeline is used.

**For 5050 (50/50):**
```json
{
  "responseCode": 200,
  "data": {
    "lifelineType": "5050",
    "questionNumber": 1,
    "remainingOptions": [0, 2],
    "lifelinesLeft": ["PHONE", "AUDIENCE"],
    "timeRemaining": 25,
    "lifelinePenalty": 5,
    "maxPointsAfterLifeline": 25
  }
}
```

**For PHONE:**
```json
{
  "responseCode": 200,
  "data": {
    "lifelineType": "PHONE",
    "questionNumber": 1,
    "suggestion": "I think the answer is A",
    "lifelinesLeft": ["AUDIENCE"],
    "timeRemaining": 20,
    "lifelinePenalty": 5,
    "maxPointsAfterLifeline": 25
  }
}
```

**For AUDIENCE:**
```json
{
  "responseCode": 200,
  "data": {
    "lifelineType": "AUDIENCE",
    "questionNumber": 1,
    "poll": {
      "A": 65,
      "B": 15,
      "C": 10,
      "D": 10
    },
    "lifelinesLeft": [],
    "timeRemaining": 25,
    "lifelinePenalty": 5,
    "maxPointsAfterLifeline": 25
  }
}
```

**Lifeline Processing Times:**
- 5050 (50/50): 5 seconds delay
- AUDIENCE: 5 seconds delay
- PHONE: 10 seconds delay

After lifeline delay, timer continues from remaining time. Lifeline penalty (5 points) is applied when lifeline is used.

---

### GAME_START
Notify client that the game session has started.

**Notification:**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Game started",
    "gameId": 12345,
    "timestamp": 1705320000
  }
}
```

---

### GAME_END
Notify that the game has ended (win, lose, or quit).

**Notification:**
```json
{
  "responseCode": 200,
  "data": {
    "gameId": 12345,
    "status": "lost",
    "finalLevel": 8,
    "finalQuestionNumber": 8,
    "safeCheckpointPrize": 10000000,
    "safeCheckpointScore": 150,
    "finalPrize": 10000000,
    "totalScore": 150,
    "message": "Wrong answer! You receive safe checkpoint prize."
  }
}
```

**Status Values:**
- `won`: Player answered all 15 questions correctly - receives maximum prize
- `lost`: Player gave wrong answer or timed out - receives safe checkpoint prize from last passed checkpoint
- `quit`: Player voluntarily gave up using GIVE_UP - receives current prize at time of giving up

**Win Condition:**
- Player must answer all 15 questions correctly to win
- Only `status: "won"` with `isWinner: true` indicates victory

**Game State After GAME_END:**
- When game ends (any status), game state is automatically cleared
- User is no longer considered "in game"
- User can immediately START a new game after game ends
- Auto-saved game (if exists) is cleared when game ends

---

### FRIEND_LIST
Send player's current friend list.

**Notification:**
```json
{
  "responseCode": 200,
  "data": {
    "friends": [
      {"username": "friend1", "status": "online"},
      {"username": "friend2", "status": "offline"}
    ]
  }
}
```

---

## Admin Requests

### Admin Authentication

Admin requests use the same `authToken` as regular users, but require additional role validation:

**Authentication Flow:**
1. Admin logs in using LOGIN request (same as regular users)
2. Server returns `authToken` and `role: "admin"` in response
3. Admin includes `authToken` in admin requests (same field name as regular requests)
4. Server validates in order:
   - `authToken` is valid (same validation as regular requests) → if invalid, returns 402 (AUTH_ERROR)
   - Extract username from `authToken` (from token mapping)
   - Check username's role in database → if role is not "admin", returns 403 (FORBIDDEN)
   - If both validations pass, process admin request

**Differences from Regular Requests:**
- **Same token**: Admin uses same `authToken` format as regular users
- **Role-based authorization**: Server checks username's role after token validation
- **Field name**: Admin requests use `authToken` field (not `adminToken`)
- **Error handling**: If user is not admin, returns 403 (FORBIDDEN) even with valid token

**Error Responses for Admin Requests:**
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - valid token but user is not admin (FORBIDDEN)
- 400: Invalid data format or missing required fields (INVALID_DATA)
- 404: Resource not found (NOT_FOUND)
- 409: Conflict (e.g., question ID already exists) (CONFLICT)

---

### ADD_QUES
Add a new question to the question database.

**Request:**
```json
{
  "requestType": "ADD_QUES",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "question": "What is the capital of France?",
    "options": [
      {"label": "A", "text": "London"},
      {"label": "B", "text": "Paris"},
      {"label": "C", "text": "Berlin"},
      {"label": "D", "text": "Madrid"}
    ],
    "correctAnswer": 1,
    "level": 1
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (must be from admin account with role "admin")
- `question`: Question text
- `options`: Array of 4 options, each with `label` (A/B/C/D) and `text`
- `correctAnswer`: Index of correct answer (0-3)
- `level`: Question level/difficulty (1-15)

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "questionId": 100,
    "message": "Question added successfully"
  }
}
```

**Error Responses:**
- 400: Missing or invalid required fields (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - not an admin account (FORBIDDEN)
- 409: Question ID already exists if auto-generated ID conflicts (CONFLICT)
- 422: Invalid correctAnswer (must be 0-3), invalid options array (must have 4 options), or invalid level (must be 1-15) (UNPROCESSABLE_DATA)

---

### CHANGE_QUES
Modify an existing question.

**Request:**
```json
{
  "requestType": "CHANGE_QUES",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "questionId": 100,
    "question": "Updated question text",
    "options": [
      {"label": "A", "text": "Option A"},
      {"label": "B", "text": "Option B"},
      {"label": "C", "text": "Option C"},
      {"label": "D", "text": "Option D"}
    ],
    "correctAnswer": 1
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (must be from admin account with role "admin")
- `questionId`: ID of question to modify
- `question`: Updated question text (optional but recommended)
- `options`: Array of 4 options (optional but recommended)
- `correctAnswer`: Index of correct answer 0-3 (optional but recommended)

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Question updated successfully"
  }
}
```

**Error Responses:**
- 400: Missing questionId or invalid data format (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - not an admin account (FORBIDDEN)
- 404: Question not found (NOT_FOUND)
- 422: Invalid correctAnswer (must be 0-3), invalid options array (must have 4 options) (UNPROCESSABLE_DATA)

---

### VIEW_QUES
View or list all stored questions.

**Request:**
```json
{
  "requestType": "VIEW_QUES",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "page": 1,
    "limit": 20,
    "level": 1
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (must be from admin account with role "admin")

**Optional Fields:**
- `page`: Page number (default: 1)
- `limit`: Number of questions per page (default: 20)
- `level`: Filter by question level (1-15), omit to get all levels

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "questions": [
      {
        "questionId": 100,
        "question": "Question text",
        "level": 1
      }
    ],
    "total": 100,
    "page": 1
  }
}
```

**Error Responses:**
- 400: Invalid data format (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - not an admin account (FORBIDDEN)
- 422: Invalid page (must be positive), invalid limit (must be positive), or invalid level (must be 1-15) (UNPROCESSABLE_DATA)

---

### DEL_QUES
Delete a question from the database.

**Request:**
```json
{
  "requestType": "DEL_QUES",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "questionId": 100
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (must be from admin account with role "admin")
- `questionId`: ID of question to delete

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Question deleted successfully"
  }
}
```

**Error Responses:**
- 400: Missing questionId (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - not an admin account (FORBIDDEN)
- 404: Question not found (NOT_FOUND)

---

### BAN_USER
Permanently block a user account.

**Request:**
```json
{
  "requestType": "BAN_USER",
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "username": "player1",
    "reason": "Violation of terms"
  }
}
```

**Required Fields:**
- `authToken`: Authentication token (must be from admin account with role "admin")
- `username`: Username of user to ban
- `reason`: Reason for banning (for logging/records)

**Note:** All bans are permanent. Once banned, user cannot login and will receive error 403 on login attempts.

**Success Response (200):**
```json
{
  "responseCode": 200,
  "data": {
    "message": "User banned successfully"
  }
}
```

**Error Responses:**
- 400: Missing username or reason (INVALID_DATA)
- 402: Missing or invalid authToken (AUTH_ERROR)
- 403: Access forbidden - not an admin account (FORBIDDEN)
- 404: User not found (NOT_FOUND)
- 422: Cannot ban yourself (UNPROCESSABLE_DATA)

---

## Scoring System

### Point Calculation Per Question

**Base Rules:**
- Maximum points per question: **30 points**
- Time limit per question: **30 seconds**
- Points decrease by **1 point per second** elapsed
- Using any lifeline: **-5 points penalty** (applied once per lifeline)
- Minimum points per question: **0 points**

**Formula:**
```
pointsEarned = max(0, timeRemaining - (lifelinePenalty * numberOfLifelinesUsed))
```

**Examples:**
1. Answer in 15 seconds, no lifeline: `30 - 15 = 15 points`
2. Answer in 10 seconds, used 1 lifeline: `30 - 10 - 5 = 15 points`
3. Answer in 25 seconds, used 2 lifelines: `30 - 25 - 10 = 0 points` (minimum 0)
4. Answer in 5 seconds, no lifeline: `30 - 5 = 25 points`

### Lifeline Processing Times

When a lifeline is used, there is a processing delay before the result is returned:
- **5050 (50/50)**: 5 seconds delay
- **AUDIENCE**: 5 seconds delay  
- **PHONE**: 10 seconds delay

During the delay, the timer continues counting down. After the delay, the timer resumes from the remaining time.

### Total Score

- Total score = Sum of all points earned from all questions answered correctly up to the last safe checkpoint (if lost)
- Only correct answers contribute to total score
- When game ends with wrong answer or timeout (status "lost"):
  - Total score = Score accumulated up to and including the last safe checkpoint passed
  - If no safe checkpoint passed (failed before question 5), total score = 0

### Win Condition

- Player must answer **all 15 questions correctly** to win
- Any wrong answer or timeout results in loss
- Final score is calculated from all 15 questions

### Leaderboard Ranking

Players are ranked by their best game performance:
1. **Primary**: `finalQuestionNumber` - Number of questions answered correctly in best game (descending, 1-15)
2. **Secondary**: `totalScore` - Total score from best game (descending)

**Note:** `finalQuestionNumber` represents the highest number of questions answered correctly in a single game by the user. This is from their best game, not necessarily their current/latest game.

---

## Game Session Management

### Game ID (gameId)

Each game session has a unique identifier (`gameId`) assigned by the server when the game starts. This ensures that game-related requests can be correctly associated with the right game session.

**Purpose:**
- Prevents confusion when a user plays multiple games in sequence
- Example: User plays Game 1 (question 1) → loses → plays Game 2 (also question 1)
- Without `gameId`, server cannot distinguish between Game 1's question 1 and Game 2's question 1
- With `gameId`, server validates that ANSWER for question 1 belongs to the correct game

**How it works:**
1. Server generates unique `gameId` when START is called
2. Server returns `gameId` in GAME_START notification
3. Server includes `gameId` in all QUESTION_INFO notifications
4. Client stores `gameId` and includes it in all game requests (ANSWER, LIFELINE, GIVE_UP)
5. Server validates `gameId` matches user's active game before processing

**Validation Rules:**
- `gameId` must match the user's currently active game
- If `gameId` is missing → error 422 (UNPROCESSABLE_DATA)
- If `gameId` doesn't match active game → error 412 (INVALID_GAME_STATE)
- If `gameId` refers to ended/expired game → error 412 (INVALID_GAME_STATE)

**Example Scenario:**
```
User plays Game 1:
- START → gameId: 1001
- ANSWER(gameId: 1001, questionNumber: 1) → correct
- ANSWER(gameId: 1001, questionNumber: 2) → wrong → Game 1 ends

User plays Game 2:
- START → gameId: 1002 (new gameId)
- ANSWER(gameId: 1002, questionNumber: 1) → correct
- ANSWER(gameId: 1001, questionNumber: 1) → ERROR 412 (wrong gameId)
```

---

## Game State Management

### Game States

A user can be in one of the following states:
1. **No Game**: User has no active or saved game. Can START new game.
2. **In Game**: User has an active game session. Cannot START new game, can RESUME if disconnected.
3. **Game Ended**: Previous game finished (won/lost/GIVE_UP). Game state cleared, can START new game. Note: timeout is treated as "lost" status.
4. **Auto-Saved Game**: User disconnected during gameplay, game auto-saved. Can RESUME or START new (with override).

### State Transitions

**Scenario 1: User loses game → wants to play again**
1. User answers incorrectly → GAME_END with status "lost"
2. Game state automatically cleared
3. User can immediately START a new game

**Scenario 2: User disconnects → reconnects**
1. User playing game → connection lost
2. Server auto-saves game state
3. User reconnects with same authToken
4. User has two options:
   - **Option A**: RESUME saved game
   - **Option B**: START new game (with `overrideSavedGame: true`)

**Scenario 3: User resumes saved game**
1. User sends RESUME request
2. Server loads auto-saved game state
3. User is now "in game"
4. User continues playing from saved position
5. If user wants to start fresh, must first END current game or wait for it to end

**Scenario 4: Multiple games in sequence**
1. User plays Game 1 → loses → Game 1 state cleared
2. User plays Game 2 → loses → Game 2 state cleared
3. User can play Game 3 immediately

### Server Behavior

**Game ID Assignment:**
- Server generates unique `gameId` when START is called
- Each new game session gets a new `gameId`
- `gameId` is unique per game, even for same user
- Server maintains mapping: `username → activeGameId`

**Game ID Validation:**
- When client sends ANSWER/LIFELINE/GIVE_UP request with `gameId`
- Server validates:
  1. User has active game session
  2. `gameId` in request matches user's active `gameId`
  3. If mismatch → error 412 (INVALID_GAME_STATE)
- This prevents processing requests from wrong/expired game sessions

**When Game Ends:**
- Server automatically clears game state
- Server clears active `gameId` mapping for user
- User status changes from "in game" to "no game"
- Auto-saved game (if exists) is cleared
- User can START new game immediately (new `gameId` will be assigned)

**When Connection Lost:**
- Server detects disconnection
- Server auto-saves current game state including `gameId` (if in game)
- User status remains "has auto-saved game"
- When user reconnects, must choose: RESUME or START new

**When User Reconnects:**
- Server maintains user's game state including `gameId`
- If user has auto-saved game, it persists with original `gameId` until:
  - User resumes it (RESUME) - uses same `gameId`
  - User starts new game with override - new `gameId` assigned
  - Game ends (if resumed and ended) - `gameId` cleared

**Conflict Resolution:**
- If user tries START while has auto-saved game → error 412
- User must either RESUME or START with `overrideSavedGame: true`
- Starting new game with override discards auto-saved game and assigns new `gameId`

**Example: Multiple Games Scenario**
```
User Login → authToken: "token123"

Game 1:
- START → gameId: 1001 assigned
- ANSWER(gameId: 1001, questionNumber: 1) → correct
- ANSWER(gameId: 1001, questionNumber: 2) → wrong
- Game 1 ends → gameId 1001 cleared

Game 2 (immediately after):
- START → gameId: 1002 assigned (NEW gameId)
- ANSWER(gameId: 1002, questionNumber: 1) → correct
- Server validates: gameId 1002 matches active game ✓
- ANSWER(gameId: 1001, questionNumber: 1) → ERROR 412 (old gameId)
```

---

## Notes

- All timestamps are Unix epoch time (seconds since 1970-01-01)
- All monetary values are in the smallest currency unit
- Message delimiter is newline character (`\n`)
- Client should implement timeout handling for all requests
- Server may send unsolicited notifications (e.g., GAME_END, FRIEND_STATUS_INFO)
- Question timeout: If 30 seconds elapse without an answer, game ends with status "lost" (same as wrong answer), player receives safe checkpoint prize
- Auto-save: Server automatically saves game state when connection is lost or client closes unexpectedly
- Only one auto-saved game per user, can be resumed with RESUME
- Game can only be resumed if auto-saved by server, not if ended due to wrong answer, timeout, or GIVE_UP
- GIVE_UP ends the game permanently and cannot be resumed
- When game ends (any status), game state is automatically cleared and user can start new game
- When user reconnects after disconnect, they must choose to RESUME or START new game