# Server Notification Types

## Overview

This document defines all notification types that the server can push to clients. Notifications use the `type` field (not `responseCode`) to distinguish them from request responses.

## Notification Structure

```json
{
  "type": "NOTIFICATION_TYPE",
  "data": {
    // Notification-specific data
  }
}
```

---

## Connection Notifications

### CONNECTION
**Sent when:** Client first connects to server (before any authentication)

**Example:**
```json
{
  "type": "CONNECTION",
  "data": {
    "serverName": "Millionaire Game Server",
    "timestamp": 1705320000
  }
}
```

---

## Game Notifications

### GAME_START
**Sent when:** Game session begins (after START request is processed)

**Example:**
```json
{
  "type": "GAME_START",
  "data": {
    "gameId": 12345,
    "timestamp": 1705320000
  }
}
```

### QUESTION_INFO
**Sent when:** 
- After GAME_START (first question)
- After correct ANSWER (next question)
- After RESUME request (resumed question)

**Example:**
```json
{
  "type": "QUESTION_INFO",
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

### LIFELINE_INFO
**Sent when:** After LIFELINE request is processed (after delay)

**Delay times:**
- 50/50: 5 seconds
- PHONE: 10 seconds
- AUDIENCE: 5 seconds

**Example (50/50):**
```json
{
  "type": "LIFELINE_INFO",
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

**Example (PHONE):**
```json
{
  "type": "LIFELINE_INFO",
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

**Example (AUDIENCE):**
```json
{
  "type": "LIFELINE_INFO",
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

### GAME_END
**Sent when:**
- Player answers incorrectly
- Player times out on a question
- Player completes all 15 questions (wins)
- Player gives up (GIVE_UP)

**Example (Won):**
```json
{
  "type": "GAME_END",
  "data": {
    "gameId": 12345,
    "status": "won",
    "finalLevel": 15,
    "finalQuestionNumber": 15,
    "finalPrize": 1000000000,
    "totalScore": 450,
    "isWinner": true
  }
}
```

**Example (Lost):**
```json
{
  "type": "GAME_END",
  "data": {
    "gameId": 12345,
    "status": "lost",
    "finalLevel": 8,
    "finalQuestionNumber": 8,
    "safeCheckpointPrize": 10000000,
    "safeCheckpointScore": 150,
    "finalPrize": 10000000,
    "totalScore": 150,
    "isWinner": false
  }
}
```

**Example (Gave Up):**
```json
{
  "type": "GAME_END",
  "data": {
    "gameId": 12345,
    "status": "gave_up",
    "finalLevel": 5,
    "finalQuestionNumber": 5,
    "finalPrize": 10000000,
    "totalScore": 120
  }
}
```

---

## Social Notifications

### FRIEND_REQUEST_RECEIVED
**Sent when:** Another user sends a friend request to this client

**Example:**
```json
{
  "type": "FRIEND_REQUEST_RECEIVED",
  "data": {
    "from": "player2",
    "timestamp": 1705320000
  }
}
```

### FRIEND_REQUEST_ACCEPTED
**Sent when:** Your friend request is accepted by another user

**Example:**
```json
{
  "type": "FRIEND_REQUEST_ACCEPTED",
  "data": {
    "username": "player2",
    "timestamp": 1705320000
  }
}
```

### FRIEND_ONLINE
**Sent when:** A friend comes online

**Example:**
```json
{
  "type": "FRIEND_ONLINE",
  "data": {
    "username": "player2",
    "timestamp": 1705320000
  }
}
```

### FRIEND_OFFLINE
**Sent when:** A friend goes offline

**Example:**
```json
{
  "type": "FRIEND_OFFLINE",
  "data": {
    "username": "player2",
    "timestamp": 1705320000
  }
}
```

### CHAT_MESSAGE
**Sent when:** Receiving a chat message from another user

**Example:**
```json
{
  "type": "CHAT_MESSAGE",
  "data": {
    "from": "player2",
    "message": "Good game!",
    "timestamp": 1705320000
  }
}
```

---

## Admin Notifications

### USER_BANNED
**Sent when:** Admin bans a user
**Sent to:** The banned user (force disconnect)

**Example (to banned user):**
```json
{
  "type": "USER_BANNED",
  "data": {
    "reason": "Cheating",
    "timestamp": 1705320000
  }
}
```

---

## System Notifications

### SERVER_SHUTDOWN
**Sent when:** Server is shutting down gracefully
**Sent to:** All connected clients

**Example:**
```json
{
  "type": "SERVER_SHUTDOWN",
  "data": {
    "reason": "Maintenance",
    "timestamp": 1705320000,
    "estimatedDowntime": 3600
  }
}
```

### SERVER_MAINTENANCE
**Sent when:** Server enters maintenance mode
**Sent to:** All connected clients

**Example:**
```json
{
  "type": "SERVER_MAINTENANCE",
  "data": {
    "message": "Server maintenance in progress",
    "timestamp": 1705320000
  }
}
```

---

## Summary

**Response (has `responseCode`):**
- Direct reply to client request
- Has HTTP-like response codes (200, 400, 401, etc.)
- Success: `{"responseCode": 200, "data": {...}}`
- Error: `{"responseCode": 401, "message": "..."}`

**Notification (has `type`):**
- Server push to client (unsolicited)
- No response code
- Format: `{"type": "NOTIFICATION_TYPE", "data": {...}}`
- Examples: GAME_START, QUESTION_INFO, CHAT_MESSAGE, etc.
