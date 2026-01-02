# Protocol Implementation Guide

## Overview

This guide provides implementation details for the client-side protocol handler. See `../docs/PROTOCOL.md` for the complete protocol specification.

## Request Format

All requests follow this JSON structure:

```json
{
  "requestType": "REQUEST_TYPE",
  "data": {
    // Request-specific fields
  }
}
```

## Response Format

### Success Response
```json
{
  "responseCode": 200,
  "data": {
    // Response data
  }
}
```

### Error Response
```json
{
  "responseCode": 401,
  "message": "Error message"
}
```

## Notification Format

Server can send unsolicited notifications:

```json
{
  "type": "NOTIFICATION_TYPE",
  "data": {
    // Notification data
  }
}
```

## Key Request Types

### Authentication

- **LOGIN**: Authenticate user
- **REGISTER**: Create new account
- **LOGOUT**: End session

### Game Actions

- **START**: Start new game
- **ANSWER**: Submit answer
- **LIFELINE**: Use lifeline (5050, PHONE, AUDIENCE)
- **GIVE_UP**: Quit game and take prize
- **RESUME**: Continue saved game
- **LEAVE_GAME**: Leave game (auto-saved)

### Social Features

- **LEADERBOARD**: Get rankings (global/friend)
- **FRIEND_STATUS**: Get friend online status
- **ADD_FRIEND**: Send friend request
- **ACCEPT_FRIEND**: Accept friend request
- **DECLINE_FRIEND**: Decline friend request
- **FRIEND_REQ_LIST**: List pending requests
- **DEL_FRIEND**: Remove friend
- **CHAT**: Send chat message

### User Information

- **USER_INFO**: Get user profile
- **VIEW_HISTORY**: Get game history
- **CHANGE_PASS**: Change password

### Connection

- **PING**: Keep-alive ping

## Key Notifications

- **CONNECTION**: Initial connection established
- **GAME_START**: Game session started
- **QUESTION_INFO**: Question details
- **LIFELINE_INFO**: Lifeline results
- **GAME_END**: Game ended
- **FRIEND_REQUEST_RECEIVED**: Friend request notification
- **CHAT_MESSAGE**: Chat message received

## Implementation Notes

1. **Authentication Token**: Most requests require `authToken` in the `data` field
2. **Game ID**: Game-related requests require `gameId` from GAME_START or QUESTION_INFO
3. **Message Delimiter**: Messages are delimited by newline (`\n`)
4. **Error Handling**: Always check `responseCode` before processing `data`
5. **Notifications**: Use callback mechanism to handle server push notifications

## Error Codes

See `../docs/ERROR_CODES.md` for complete error code reference.

Common error codes:
- 200: Success
- 400: Invalid data
- 401: Login failed
- 402: Auth error (invalid/missing token)
- 403: Forbidden
- 404: Not found
- 405: User already in game
- 406: User not in game
- 408: Question timeout
- 412: Invalid game state

## Example Usage

```cpp
// Login
ProtocolHandler::LoginResponse loginResp = protocol.login("username", "password");
if (loginResp.success) {
    string authToken = loginResp.authToken;
    
    // Start game
    ProtocolHandler::StartResponse startResp = protocol.startGame(authToken, false);
    if (startResp.success) {
        // Wait for QUESTION_INFO notification
        // Then answer
        ProtocolHandler::AnswerResponse answerResp = protocol.answerQuestion(
            authToken, gameId, questionNumber, answerIndex);
    }
}
```

## Testing

Test all request types and error handling:
- Connection errors
- Authentication failures
- Game state transitions
- Notification handling
- Error code parsing

