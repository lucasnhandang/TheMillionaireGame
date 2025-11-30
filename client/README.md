# Client Module

## Overview

This folder contains the client application code including GUI and client-server communication logic.

## Responsibilities (Member B)

- **Client Protocol Implementation**: Implement client-side protocol handlers for all request types
- **GUI Development**: Create user interface for the game
- **Network Communication**: Handle TCP socket communication with server
- **Game UI Logic**: Implement game flow, question display, lifeline UI, etc.

## Folder Structure

```
client/
├── src/                    # Source code
│   ├── main.cpp           # Entry point
│   ├── client_core.h/cpp  # Client core (socket, connection)
│   ├── protocol_handler.h/cpp  # Protocol request/response handling
│   ├── gui/               # GUI code
│   │   ├── main_window.h/cpp
│   │   ├── login_window.h/cpp
│   │   ├── game_window.h/cpp
│   │   └── leaderboard_window.h/cpp
│   └── utils/             # Utilities
│       └── json_parser.h/cpp
├── include/                # Headers
├── resources/              # GUI resources (images, icons)
├── CMakeLists.txt          # Build configuration (if using CMake)
├── Makefile               # Build configuration
├── README.md              # This file
└── PROTOCOL_GUIDE.md      # Protocol implementation guide
```

## Protocol Reference

See `../PROTOCOL.md` for complete protocol specification.

### Key Request Types to Implement:

**Authentication:**
- LOGIN
- REGISTER
- LOGOUT

**Game Actions:**
- START
- ANSWER
- LIFELINE (5050, PHONE, AUDIENCE)
- GIVE_UP
- RESUME
- LEAVE_GAME

**Social Features:**
- LEADERBOARD
- FRIEND_STATUS
- ADD_FRIEND
- ACCEPT_FRIEND
- DECLINE_FRIEND
- FRIEND_REQ_LIST
- DEL_FRIEND
- CHAT

**User Features:**
- USER_INFO
- VIEW_HISTORY
- CHANGE_PASS

**Connection:**
- PING
- CONNECTION

## Implementation Steps

1. **Review Protocol**: Read `../PROTOCOL.md` and `../ERROR_CODES.md`

2. **Implement Client Core**:
   - TCP socket connection to server
   - Message sending/receiving
   - Connection management

3. **Implement Protocol Handler**:
   - Request building (JSON format)
   - Response parsing
   - Error handling

4. **Implement GUI**:
   - Login/Register screen
   - Main menu
   - Game screen (question display, lifelines, timer)
   - Leaderboard screen
   - Friend management screen

5. **Integrate**:
   - Connect GUI events to protocol handlers
   - Handle server responses and update UI
   - Implement game flow logic

## Server Connection

Default server configuration:
- Host: `localhost`
- Port: `8080` (configurable)

Connection should:
- Handle connection errors gracefully
- Implement reconnection logic
- Show connection status to user

## GUI Requirements

### Login Screen:
- Username input
- Password input
- Login button
- Register button
- Error message display

### Game Screen:
- Question display (large, readable)
- 4 answer options (buttons)
- Timer display
- Lifeline buttons (5050, Phone, Audience)
- Current prize display
- Give up button
- Leave game button

### Leaderboard Screen:
- Global leaderboard
- Friend leaderboard
- Pagination controls

### Friend Management:
- Friend list with online status
- Add friend button
- Friend request list
- Accept/Decline buttons

## Testing

- Test connection to server
- Test all request types
- Test error handling
- Test GUI responsiveness
- Test game flow end-to-end

## Notes

- Use the same JSON format as specified in PROTOCOL.md
- Handle all error codes from ERROR_CODES.md
- Implement proper authentication token management
- Show user-friendly error messages
- Ensure thread-safety for network operations

