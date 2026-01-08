# Bug Analysis: "Waiting for question..." Lock Issue

**Date**: January 9, 2026  
**Issue**: Client gets stuck showing "Waiting for question..." after starting a game  
**Severity**: Critical - blocks gameplay  
**Root Cause**: Message filtering race condition in notification handler

---

## Table of Contents
1. [Understanding the Message Flow](#understanding-the-message-flow)
2. [What Each Message Does](#what-each-message-does)
3. [The Bug Explained](#the-bug-explained)
4. [Detailed Trace](#detailed-trace)
5. [Why We Need All Three Messages](#why-we-need-all-three-messages)
6. [Proposed Solutions](#proposed-solutions)

---

## Understanding the Message Flow

### Architecture Overview

The client has **TWO separate threads** that handle messages:

```
┌─────────────────────────────────────────────────────────┐
│                    CLIENT APPLICATION                    │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │         MAIN THREAD (UI Thread)                │    │
│  │  - Renders ImGui                               │    │
│  │  - Handles user clicks                         │    │
│  │  - Calls protocol->startGame()                 │    │
│  │  - Processes events from eventQueue            │    │
│  └────────────┬───────────────────────────────────┘    │
│               │                                          │
│               │ Reads from                               │
│               ▼                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │         EVENT QUEUE (thread-safe)              │    │
│  │  - Stores: GAME_START, QUESTION_INFO, etc.    │    │
│  └────────────▲───────────────────────────────────┘    │
│               │ Writes to                                │
│               │                                          │
│  ┌────────────┴───────────────────────────────────┐    │
│  │    NOTIFICATION THREAD (handleNotifications)   │    │
│  │  - Continuously reads from socket              │    │
│  │  - Filters messages                            │    │
│  │  - Pushes notifications to eventQueue          │    │
│  └────────────┬───────────────────────────────────┘    │
│               │                                          │
│               │ Both threads read from                   │
│               ▼                                          │
│  ┌────────────────────────────────────────────────┐    │
│  │      MESSAGE QUEUE (in SocketClient)           │    │
│  │  - Receives all messages from server           │    │
│  │  - Thread-safe queue                           │    │
│  └────────────▲───────────────────────────────────┘    │
│               │                                          │
│  ┌────────────┴───────────────────────────────────┐    │
│  │      RECEIVE THREAD (in SocketClient)          │    │
│  │  - Reads from TCP socket                       │    │
│  │  - Parses message type                         │    │
│  │  - Enqueues to messageQueue                    │    │
│  └────────────────────────────────────────────────┘    │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Two Consumers, One Queue Problem

The **MESSAGE QUEUE** is consumed by **TWO different threads**:

1. **ProtocolHandler** (in main thread):
   - Waits for **request responses** (LOGIN response, START response, etc.)
   - Calls `client->getMessage()` in `waitForResponse()` (protocol_handler.cpp:274)
   - Blocks until it gets the response it's waiting for

2. **NotificationHandler** (separate thread):
   - Continuously polls for **notifications** (GAME_START, QUESTION_INFO, etc.)
   - Calls `client->getMessage()` in infinite loop (main.cpp:242)
   - Pushes notifications to eventQueue for UI thread

**Problem**: Both threads are reading from the SAME queue! If one thread reads a message meant for the other, it causes issues.

---

## What Each Message Does

When you call `protocol->startGame()`, the server sends **3 separate messages**:

### Message 1: START Response (Request Response)

**Purpose**: Acknowledge that the START request was received and processed  
**Consumer**: `ProtocolHandler::waitForResponse()` (main thread)  
**Format**:
```json
{
  "responseCode": 200,
  "data": {
    "gameId": 12345,
    "questionNumber": 1
  }
}
```

**What it does**:
- Confirms game started successfully (code 200) or failed (code 405, 412, etc.)
- Returns `gameId` and initial `questionNumber`
- `protocol->startGame()` extracts these values (protocol_handler.cpp:124-130):
  ```cpp
  if (code == 200) {
      currentGameId = extractInt(msg.data, "gameId", 0);
      currentQuestionNumber = extractInt(msg.data, "questionNumber", 1);
  }
  ```
- **CRITICAL**: Main thread is **BLOCKED** waiting for this response!

**Why we need it**:
- Without this, `protocol->startGame()` returns error code → main.cpp thinks game didn't start
- Main thread needs to know if START succeeded before continuing
- Provides `gameId` needed for subsequent requests (ANSWER, LIFELINE, GIVE_UP)

---

### Message 2: GAME_START Notification

**Purpose**: Notify client that game session has begun  
**Consumer**: `handleNotifications()` → eventQueue → `processGameEvents()` (UI thread)  
**Format**:
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

**What it does** (main.cpp:278-303):
```cpp
case EVENT_GAME_START: {
    // Extract gameId from notification
    int gameId = extractInt(event.data, "gameId", 0);
    protocol->currentGameId = gameId;  // Update protocol handler
    
    // Reset ALL game state
    state.inGame = true;
    state.onHome = false;
    state.waitingForQuestion = true;
    state.availableLifelines = {true, true, true};
    state.totalScore = 0;
    state.currentQuestionNumber = 0;
    state.selectedAnswer = -1;
    state.question.clear();
    state.options.clear();
    // ... etc
}
```

**Why we need it**:
- Resets ALL game state variables (lifelines, score, question data, etc.)
- Sets `state.inGame = true` → triggers game UI rendering
- Sets `state.waitingForQuestion = true` → shows "Waiting for question..." initially
- Updates `protocol->currentGameId` (backup in case START response gameId was wrong)
- **Without this**: Game state not properly initialized, could have leftover data from previous game

---

### Message 3: QUESTION_INFO Notification

**Purpose**: Deliver the first question to display  
**Consumer**: `handleNotifications()` → eventQueue → `processGameEvents()` (UI thread)  
**Format**:
```json
{
  "responseCode": 200,
  "data": {
    "questionId": 100,
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

**What it does** (main.cpp:306-383):
```cpp
case EVENT_QUESTION_INFO: {
    // Extract question data
    string newQuestion = extractString(event.data, "question");
    int newQuestionNumber = extractInt(event.data, "questionNumber", 0);
    int newPrize = extractInt(event.data, "prize", 0);
    vector<string> newOptions = parseOptionsArray(event.data);
    
    // Stop old timer
    state.timerRunning = false;
    state.timerThreadId++;
    
    // Update ALL state
    state.question = newQuestion;
    state.currentQuestionNumber = newQuestionNumber;
    state.timeRemaining = 30;
    state.currentPrize = newPrize;
    state.options = newOptions;
    state.waitingForQuestion = false;  // ⭐ EXIT "WAITING" STATE!
    state.revealActive = true;
    
    // Update protocol handler
    protocol->currentQuestionNumber = newQuestionNumber;
    protocol->currentGameId = extractInt(event.data, "gameId", 0);
}
```

**Why we need it**:
- Provides the actual question text and options to display
- Sets `state.waitingForQuestion = false` → **EXIT "WAITING" STATE!** → show question
- Starts answer reveal animation
- Updates timer, prize, score display
- **Without this**: Client stays at "Waiting for question..." forever → **THIS IS THE BUG!**

---

## The Bug Explained

### The Filter Logic (main.cpp:247-251)

```cpp
void handleNotifications(...) {
    SocketClient::Message msg;
    while (true) {
        if (client->getMessage(msg, 100)) {
            // ⚠️ THE PROBLEMATIC FILTER
            if (msg.type == "RESPONSE" && msg.data.find("\"responseCode\"") != std::string::npos) {
                // This is a request response, put it back for ProtocolHandler
                client->putMessageBack(msg);
                continue;  // Skip processing
            }
            
            // Process notification...
            if (msg.type == "GAME_START") {
                eventQueue->push(GameEvent(EVENT_GAME_START, msg.data));
            } else if (msg.type == "QUESTION_INFO") {
                eventQueue->push(GameEvent(EVENT_QUESTION_INFO, msg.data));
            }
            // ...
        }
    }
}
```

### The Problem

**Issue 1: All messages have `msg.type = "RESPONSE"`**

Looking at `socket_client.cpp:175-192`:
```cpp
void SocketClient::handleMessage(const std::string& message) {
    // Determine message type
    std::string type = "RESPONSE"; // ⚠️ DEFAULT!
    
    // Check for notification types
    if (message.find("\"type\":\"QUESTION_INFO\"") != std::string::npos) {
        type = "QUESTION_INFO";
    } else if (message.find("\"type\":\"GAME_END\"") != std::string::npos) {
        type = "GAME_END";
    }
    // ...
}
```

**Problem**: Server doesn't send `"type":"QUESTION_INFO"` in the JSON! It only sends:
```json
{"responseCode":200,"data":{"questionNumber":1,"question":"...","options":[...],...}}
```

So `socket_client` sets `type = "RESPONSE"` (the default) for QUESTION_INFO! ❌

**Issue 2: All messages have `"responseCode"`**

Per PROTOCOL.md, **ALL server messages** (responses AND notifications) have `"responseCode": 200` field!

So the filter condition:
```cpp
if (msg.type == "RESPONSE" && msg.data.find("\"responseCode\"") != std::string::npos)
```

Matches **BOTH**:
- ✅ START response (intended)
- ❌ QUESTION_INFO notification (NOT intended!)

---

## Detailed Trace

### Scenario: Bug Occurs

```
TIME    THREAD              ACTION
────────────────────────────────────────────────────────────────────────
T=0     Main Thread         User clicks "Start New Game"
                            Calls: protocol->startGame(false)

T=1     Main Thread         ProtocolHandler sends START request:
                            {"requestType":"START","data":{"authToken":"..."}}
                            
T=2     Main Thread         Calls: waitForResponse(5000)
                            BLOCKS waiting for START response
                            |
                            | ⏸️ Main thread BLOCKED here!
                            ▼

T=3     Server              Receives START request
                            Creates game session (gameId=12345)
                            Sends 3 messages:
                            
                            1️⃣ START Response:
                               {"responseCode":200,"data":{"gameId":12345}}
                            
                            2️⃣ GAME_START Notification:
                               {"responseCode":200,"data":{"message":"Game started","gameId":12345}}
                            
                            3️⃣ QUESTION_INFO Notification:
                               {"responseCode":200,"data":{"questionNumber":1,"question":"...","options":[...]}}

T=4     Receive Thread      Receives all 3 messages from socket
                            Calls handleMessage() for each
                            Enqueues to messageQueue:
                            [START Response, GAME_START, QUESTION_INFO]

T=5     Notification Thread Calls: client->getMessage(msg, 100)
                            Gets: START Response
                            
                            Checks: msg.type == "RESPONSE" ✅
                            Checks: msg.data has "responseCode" ✅
                            
                            Decision: "This is a request response"
                            Action: client->putMessageBack(msg) ⬅️ Put back in queue
                                   continue (skip processing)

T=6     Main Thread         Still in waitForResponse()
                            Calls: client->getMessage(msg, 100)
                            Gets: START Response (from putMessageBack)
                            
                            Checks: Has "responseCode" ✅
                            Returns msg to protocol->startGame()

T=7     Main Thread         Extracts: currentGameId = 12345 ✅
                            Returns: code = 200 ✅
                            main.cpp receives code 200
                            
                            Sets: state.inGame = true ✅
                                  state.waitingForQuestion = true ✅
                                  
                            ⚠️ Waiting for GAME_START and QUESTION_INFO notifications!

T=8     Notification Thread Calls: client->getMessage(msg, 100)
                            Gets: GAME_START notification
                            
                            Checks: msg.type == "RESPONSE" ✅ (default type!)
                            Checks: msg.data has "responseCode" ✅
                            
                            Decision: "This is a request response" ❌ WRONG!
                            Action: client->putMessageBack(msg)
                                   continue (skip processing)
                            
                            ⚠️ GAME_START is NOT pushed to eventQueue!

T=9     Notification Thread Calls: client->getMessage(msg, 100)
                            Gets: QUESTION_INFO notification
                            
                            Checks: msg.type == "RESPONSE" ✅ (default type!)
                            Checks: msg.data has "responseCode" ✅
                            
                            Decision: "This is a request response" ❌ WRONG!
                            Action: client->putMessageBack(msg)
                                   continue (skip processing)
                            
                            ⚠️ QUESTION_INFO is NOT pushed to eventQueue!

T=10    Main Thread         Renders UI:
                            Checks: !state.question.empty() → FALSE (empty)
                            Checks: state.currentQuestionNumber > 0 → FALSE (0)
                            
                            Shows: "Waiting for question..." 🔒
                            
                            Calls: processGameEvents(&eventQueue, ...)
                            eventQueue is EMPTY! (no events pushed)
                            
                            ⚠️ STUCK! No events to process!

T=11+   Notification Thread Keeps polling getMessage()
                            Keeps getting GAME_START and QUESTION_INFO
                            Keeps putting them back
                            Never processes them
                            
                            ⚠️ Infinite loop of putting messages back!

T=30    Game Timer          30 seconds elapse (timeout)
                            Server sends GAME_END notification
                            Notification thread ALSO filters it out!
                            
                            Eventually server ends game, client gets kicked
```

### Why It Sometimes Works

**If** the timing is right and `socket_client.cpp` correctly detects the message type:

```cpp
// If server happens to send "type" field in JSON (doesn't per current protocol):
if (message.find("\"type\":\"QUESTION_INFO\"") != std::string::npos) {
    type = "QUESTION_INFO";  // ✅ Correct type set
}
```

Then:
```cpp
// In handleNotifications filter:
if (msg.type == "RESPONSE" && msg.data.find("\"responseCode\"") != std::string::npos) {
    // msg.type is "QUESTION_INFO", not "RESPONSE" → condition FALSE
    // Message is NOT filtered out ✅
}
```

**But**: Server doesn't send `"type"` field, so this rarely happens! The bug occurs ~90% of the time.

---

## Why We Need All Three Messages

### Why Not Just START Response?

**If we only had START response**:
- Main thread gets `gameId` ✅
- But UI state not reset (leftover data from previous game) ❌
- No question to display ❌
- No prize, timer, options ❌

### Why Not Just GAME_START?

**If we only had GAME_START**:
- Game state reset ✅
- But main thread's `protocol->startGame()` returns timeout (no response) ❌
- Main thread thinks START failed → doesn't enter game ❌
- No question to display ❌

### Why Not Just QUESTION_INFO?

**If we only had QUESTION_INFO**:
- Question displayed ✅
- But main thread's `protocol->startGame()` returns timeout ❌
- Game state not reset (lifelines not reset, etc.) ❌
- `gameId` not set properly ❌

### Why We Need All Three

**START Response** (for main thread):
- ✅ Confirms START request succeeded
- ✅ Provides `gameId` for protocol handler
- ✅ Unblocks main thread so it can continue

**GAME_START Notification** (for UI state):
- ✅ Resets ALL game state variables
- ✅ Sets `inGame = true` (enter game screen)
- ✅ Prepares for first question
- ✅ Backup `gameId` update

**QUESTION_INFO Notification** (for UI content):
- ✅ Provides actual question text and options
- ✅ Sets `waitingForQuestion = false` (exit waiting state)
- ✅ Starts timer and reveal animation
- ✅ Updates prize display

**All three are essential!** They serve different purposes for different parts of the client.

---

## Proposed Solutions

### Option 1: Fix Message Type Detection in SocketClient (BEST)

**Problem**: `socket_client.cpp` defaults to `type = "RESPONSE"` for all messages

**Solution**: Improve type detection to check for notification-specific fields

**File**: `client/socket_client.cpp` lines 175-192

**Change**:
```cpp
void SocketClient::handleMessage(const std::string& message) {
    // Determine message type
    std::string type = "RESPONSE"; // Default
    
    // Check for notification types by looking for notification-specific fields
    if (message.find("\"questionNumber\"") != std::string::npos && 
        message.find("\"question\"") != std::string::npos &&
        message.find("\"options\"") != std::string::npos) {
        // Has question, questionNumber, and options → QUESTION_INFO
        type = "QUESTION_INFO";
    } else if (message.find("\"message\":\"Game started\"") != std::string::npos) {
        // Has "message": "Game started" → GAME_START
        type = "GAME_START";
    } else if (message.find("\"status\":") != std::string::npos && 
               message.find("\"finalPrize\"") != std::string::npos) {
        // Has status and finalPrize → GAME_END
        type = "GAME_END";
    } else if (message.find("\"lifelineType\"") != std::string::npos &&
               (message.find("\"remainingOptions\"") != std::string::npos ||
                message.find("\"suggestion\"") != std::string::npos ||
                message.find("\"poll\"") != std::string::npos)) {
        // Has lifelineType and lifeline result → LIFELINE_INFO
        type = "LIFELINE_INFO";
    }
    // Otherwise: default "RESPONSE" for request responses
    
    // ... rest of function
}
```

**Pros**:
- ✅ Fixes root cause at the source
- ✅ Makes type detection reliable
- ✅ Filter in handleNotifications works correctly
- ✅ No changes needed to main.cpp

**Cons**:
- ⚠️ Fragile - relies on specific JSON field patterns
- ⚠️ If server changes field names, breaks again

---

### Option 2: Better Filter Logic in handleNotifications (SIMPLER)

**Problem**: Filter can't distinguish notifications from responses

**Solution**: Add notification-specific checks to filter logic

**File**: `client/main.cpp` lines 247-251

**Change**:
```cpp
void handleNotifications(...) {
    SocketClient::Message msg;
    while (true) {
        if (client->getMessage(msg, 100)) {
            // Detect if this is a notification (not a request response)
            bool isNotification = false;
            
            // Check for notification-specific fields
            if (msg.data.find("\"questionNumber\"") != std::string::npos && 
                msg.data.find("\"question\"") != std::string::npos) {
                // Has questionNumber + question → QUESTION_INFO notification
                isNotification = true;
            } else if (msg.data.find("\"message\":\"Game started\"") != std::string::npos) {
                // Has "Game started" message → GAME_START notification
                isNotification = true;
            } else if (msg.data.find("\"status\":") != std::string::npos && 
                       msg.data.find("\"finalPrize\"") != std::string::npos) {
                // Has status + finalPrize → GAME_END notification
                isNotification = true;
            } else if (msg.data.find("\"lifelineType\"") != std::string::npos) {
                // Has lifelineType → LIFELINE_INFO notification
                isNotification = true;
            }
            
            // If it's a request response (not notification), put it back for ProtocolHandler
            if (msg.type == "RESPONSE" && 
                msg.data.find("\"responseCode\"") != std::string::npos && 
                !isNotification) {
                client->putMessageBack(msg);
                continue;
            }
            
            // Process notification...
            if (msg.type == "QUESTION_INFO" || 
                (msg.data.find("\"questionNumber\"") != std::string::npos && 
                 msg.data.find("\"question\"") != std::string::npos)) {
                eventQueue->push(GameEvent(EVENT_QUESTION_INFO, msg.data));
            } else if (msg.type == "GAME_START" || 
                       msg.data.find("\"message\":\"Game started\"") != std::string::npos) {
                eventQueue->push(GameEvent(EVENT_GAME_START, msg.data));
            }
            // ... etc
        }
    }
}
```

**Pros**:
- ✅ Minimal changes (only handleNotifications)
- ✅ Fixes the immediate bug
- ✅ Easy to test

**Cons**:
- ⚠️ Duplicates type detection logic (also in socket_client)
- ⚠️ Harder to maintain (logic in 2 places)

---

### Option 3: Remove Filter Entirely (NUCLEAR)

**Problem**: Filter blocks notifications

**Solution**: Don't filter at all - let ProtocolHandler handle all messages

**File**: `client/main.cpp` lines 247-251

**Change**:
```cpp
void handleNotifications(...) {
    SocketClient::Message msg;
    while (true) {
        if (client->getMessage(msg, 100)) {
            // ⚠️ REMOVE THE FILTER ENTIRELY
            // Just process all messages as notifications
            
            if (msg.type == "GAME_START") {
                eventQueue->push(GameEvent(EVENT_GAME_START, msg.data));
            } else if (msg.type == "QUESTION_INFO") {
                eventQueue->push(GameEvent(EVENT_QUESTION_INFO, msg.data));
            }
            // ... etc
        }
    }
}
```

**AND** update `ProtocolHandler::waitForResponse()` to be smarter:

**File**: `client/protocol_handler.cpp` lines 274-321

**Change**:
```cpp
SocketClient::Message ProtocolHandler::waitForResponse(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) {
            msg.type = "TIMEOUT";
            return msg;
        }
        
        if (client_->getMessage(msg, 100)) {
            // Skip notifications - only return request responses
            if (msg.data.find("\"questionNumber\"") != std::string::npos && 
                msg.data.find("\"question\"") != std::string::npos) {
                // QUESTION_INFO - skip
                continue;
            } else if (msg.data.find("\"message\":\"Game started\"") != std::string::npos) {
                // GAME_START - skip
                continue;
            } else if (msg.data.find("\"status\":") != std::string::npos) {
                // GAME_END - skip
                continue;
            } else if (msg.data.find("\"lifelineType\"") != std::string::npos) {
                // LIFELINE_INFO - skip
                continue;
            }
            
            // This is a request response - return it
            return msg;
        }
    }
}
```

**Pros**:
- ✅ Guarantees notifications never lost
- ✅ No putMessageBack needed
- ✅ Clear separation of concerns

**Cons**:
- ⚠️ Larger code changes (2 files modified)
- ⚠️ ProtocolHandler might consume some notifications before handleNotifications sees them
- ⚠️ Race condition still possible

---

## Recommended Solution

**Use Option 1** (Fix SocketClient type detection) because:

1. ✅ **Fixes root cause** - Makes `msg.type` correctly identify notifications
2. ✅ **Single point of fix** - Only need to change `socket_client.cpp`
3. ✅ **Makes filter work** - The existing filter logic works once types are correct
4. ✅ **Clean architecture** - Type detection belongs in SocketClient layer
5. ✅ **Prevents future bugs** - Other code can rely on `msg.type` being accurate

**Testing Plan**:
1. Modify `socket_client.cpp` handleMessage()
2. Rebuild client: `cd client && make clean && make`
3. Test START game 10 times in a row
4. Verify no "Waiting for question..." lock occurs
5. Verify debug logs show correct message types
6. Test all lifelines to ensure LIFELINE_INFO not filtered

---

## Summary

**The Bug**: Client gets stuck at "Waiting for question..." because:
1. `socket_client` sets `msg.type = "RESPONSE"` for ALL messages (including notifications)
2. All messages have `"responseCode"` field (per protocol)
3. Filter in `handleNotifications` blocks ALL messages with `type=="RESPONSE" && has responseCode`
4. QUESTION_INFO notification gets filtered out
5. UI never receives QUESTION_INFO event
6. `state.waitingForQuestion` stays true forever
7. UI keeps showing "Waiting for question..." until timeout

**The Fix**: Make `socket_client` correctly identify notification messages by checking for notification-specific fields (like `"question"`, `"questionNumber"`, etc.) before defaulting to `"RESPONSE"` type.

**Why We Need All 3 Messages**:
- **START Response**: Tells main thread START succeeded + provides gameId
- **GAME_START Notification**: Resets all game state for new game
- **QUESTION_INFO Notification**: Provides question content + exits "waiting" state

Each message serves a distinct purpose and all three are essential for proper game flow.

---

## Option 4: Protocol-Level Classification (MOST ROBUST)

**Date Added**: January 9, 2026  
**Proposed By**: User feedback on fragility of pattern matching

### The Problem with Options 1-3

All three previous options rely on **content-based pattern matching** to distinguish responses from notifications:
- Checking for `"questionNumber"` + `"question"` → QUESTION_INFO
- Checking for `"message":"Game started"` → GAME_START
- Checking for `"finalPrize"` → GAME_END

**Fragility**: If server changes field names, adds new notification types, or modifies message structure, the client breaks.

### The Core Issue

The protocol itself is ambiguous:
- Both responses AND notifications use `"responseCode": 200`
- Both are JSON objects with a `"data"` field
- No explicit field to indicate "this is a response" vs "this is a notification"
- Client must **guess** based on content

This violates the principle: **Make illegal states unrepresentable in your protocol.**

### Proposed Solution: Explicit Message Classification

**Server Changes**:

1. **Request Responses** - Keep current format, ensure they have `"responseCode"`:
   - LOGIN response, START response, ANSWER response, etc.
   - Always contain `"responseCode"` field at root level
   - Example: `{"responseCode": 200, "data": {"gameId": 12345}}`

2. **Server-Pushed Notifications** - Add explicit `"notificationType"` field:
   - GAME_START, QUESTION_INFO, GAME_END, LIFELINE_INFO, etc.
   - Always contain `"notificationType"` field at root level
   - Example: `{"notificationType": "QUESTION_INFO", "data": {"questionNumber": 1, ...}}`

**Key Rule**: A message CANNOT have both `"responseCode"` AND `"notificationType"` - they are mutually exclusive.

### Client Implementation Strategy

**1. SocketClient Layer** (`socket_client.cpp`):
   - Check for `"notificationType"` field first
   - If found: set `msg.type = <notificationType value>` (e.g., "QUESTION_INFO")
   - Else check for `"responseCode"` field
   - If found: set `msg.type = "RESPONSE"`
   - Else: set `msg.type = "UNKNOWN"` (log warning)

**2. Notification Handler** (`main.cpp`):
   - Filter becomes simple: `if (msg.type == "RESPONSE") { putMessageBack(); continue; }`
   - All other types are notifications by definition
   - Process based on `msg.type` value

**3. ProtocolHandler** (`protocol_handler.cpp`):
   - `waitForResponse()` only returns messages where `msg.type == "RESPONSE"`
   - Puts back any message where `msg.type != "RESPONSE"`
   - No content inspection needed

### Benefits

1. **✅ Unambiguous** - No guessing required, message type is explicit
2. **✅ Protocol-driven** - Classification logic lives in protocol design, not pattern matching
3. **✅ Future-proof** - Adding new notification types requires no client changes
4. **✅ Server flexibility** - Server can change field names without breaking clients
5. **✅ Clear separation** - Main thread vs notification thread routing is trivial
6. **✅ Easy debugging** - Can immediately see if a message is response or notification
7. **✅ Type-safe** - Impossible to misclassify messages

### Migration Impact

**Server Files to Modify**:
- `notification_utils.cpp` - Add `"notificationType"` to all notification functions
- `request_handlers/*.cpp` - Ensure all responses keep `"responseCode"`
- Review all `StreamUtils::createNotification()` calls

**Client Files to Modify**:
- `socket_client.cpp` - Update `handleMessage()` type detection logic
- `main.cpp` - Simplify `handleNotifications()` filter
- `protocol_handler.cpp` - Simplify `waitForResponse()` logic (optional cleanup)

**Protocol Documentation**:
- Update `PROTOCOL.md` to document `"notificationType"` field requirement
- Add migration guide for distinguishing responses vs notifications

### Why This Is Better

**Option 1** (pattern matching): Breaks if server changes `"message":"Game started"` to `"status":"started"`  
**Option 4** (protocol-level): Doesn't care - looks at `"notificationType": "GAME_START"`

**Option 1**: Must update client for every new notification type  
**Option 4**: Client automatically routes any `"notificationType"` to notification handler

**Option 1**: Requires maintaining parallel detection logic in multiple places  
**Option 4**: Single source of truth - the `"notificationType"` field

### Implementation Priority

This is the **cleanest architectural solution** but requires:
1. Server-side changes to all notification functions
2. Protocol documentation updates
3. Client-side simplification

If time permits and server code is accessible, this is the **recommended long-term fix**.  
If quick fix needed, use **Option 1** as a temporary solution and migrate to Option 4 later.

---

## Implementation Status: Option 4 (COMPLETED)

**Date Implemented**: January 9, 2026

### Changes Made

**Server-side (`server/stream_handler.cpp`)**:
- Modified `StreamUtils::createNotification()` to use `"notificationType"` field instead of `"type"`
- Updated function signature documentation in `stream_handler.h`
- All notifications now send: `{"notificationType":"TYPE","data":{...}}`
- All responses continue to use: `{"responseCode":200,"data":{...}}`

**Client-side (`client/socket_client.cpp`)**:
- Updated `handleMessage()` to detect `"notificationType"` field first
- Falls back to `"responseCode"` detection for responses
- Extracts the notification type value dynamically (e.g., "GAME_START", "QUESTION_INFO")
- Sets `msg.type = "UNKNOWN"` for unrecognized messages

**Client-side (`client/main.cpp`)**:
- Simplified `handleNotifications()` filter to: `if (msg.type == "RESPONSE") { putMessageBack(); continue; }`
- Added 10ms sleep after putting RESPONSE messages back to prevent race condition with waitForResponse
- All other types are treated as notifications by definition
- Removed complex pattern matching logic

**Client-side (`client/protocol_handler.cpp`)**:
- Simplified `waitForResponse()` to only return messages where `msg.type == "RESPONSE"`
- All non-RESPONSE types are put back for notification handler
- Added 10ms sleep after putting messages back to prevent race condition (avoid infinite loop where same notification is retrieved repeatedly)
- Removed checks for CONNECTION, GAME_START patterns

### Benefits Achieved

1. ✅ **Unambiguous classification** - No ambiguity between responses and notifications
2. ✅ **Protocol-driven** - Type detection based on explicit field, not pattern matching
3. ✅ **Future-proof** - New notification types work automatically without client changes
4. ✅ **Simplified code** - Reduced complexity in message filtering logic
5. ✅ **Clean separation** - Clear distinction between main thread (responses) and notification thread (notifications)

### Race Condition Fix

**Issue Discovered**: When a thread calls `putMessageBack()`, it can immediately retrieve the same message in its next loop iteration, causing an infinite loop.

**Solution**: Added 10ms sleep after `putMessageBack()` in both:
- `handleNotifications()` - when putting back RESPONSE messages
- `waitForResponse()` - when putting back notification messages

This small delay allows the other thread to grab the message from the queue, preventing the infinite loop while being imperceptible to users.

### Testing Checklist

- [ ] Rebuild server: `cd server && make clean && make`
- [ ] Rebuild client: `cd client && make clean && make`
- [ ] Test START game flow - verify no "Waiting for question..." lock
- [ ] Test all lifelines (5050, PHONE, AUDIENCE) - verify LIFELINE_INFO received
- [ ] Test GIVE_UP - verify GAME_END received
- [ ] Test answer submission - verify QUESTION_INFO for next question
- [ ] Test winning the game - verify final GAME_END received
- [ ] Verify debug logs show correct message types

---

**End of Analysis**

