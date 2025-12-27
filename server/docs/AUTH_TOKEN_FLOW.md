# Auth Token Flow Documentation

## Overview

Auth token is used to authenticate and authorize client requests throughout the connection. After successful LOGIN or REGISTER, client receives an authToken that MUST be included in every subsequent request.

## Flow

### 1. Initial Connection
- Client connects to server via TCP socket
- Server sends CONNECTION message to confirm connection

### 2. Login/Register
- Client sends LOGIN or REGISTER request (NO authToken needed)
- Server validates credentials
- Server generates unique 32-character hex token
- Server stores mapping: `token → client_fd`, `username → token`
- Server responds with authToken in response

**Request (LOGIN):**
```json
{
  "requestType": "LOGIN",
  "data": {
    "username": "player1",
    "password": "password123"
  }
}
```

**Response:**
```json
{
  "responseCode": 200,
  "data": {
    "authToken": "a1b2c3d4e5f6...",
    "username": "player1",
    "message": "Login successful"
  }
}
```

### 3. Subsequent Requests
- Client MUST include `authToken` in `data` field of every request
- Server validates token on each request
- If token is missing or invalid → returns 401 error

**Example Request (START_GAME):**
```json
{
  "requestType": "START_GAME",
  "data": {
    "authToken": "a1b2c3d4e5f6..."
  }
}
```

### 4. Token Validation Process

Server validates token by checking:
1. Token exists in request JSON (`data.authToken`)
2. Token exists in server's token mapping (`token_to_fd_`)
3. Token belongs to current client's socket file descriptor
4. Token matches session's stored token

If any check fails → Request rejected with 401 error

### 5. Token Cleanup

- When client disconnects, token is removed from mappings
- User is removed from online_users_ set
- Session is cleaned up

## Security Notes

- Token is unique per connection
- Token cannot be reused across different connections
- Token is validated on every request (stateless validation)
- Token is only valid for the socket connection it was issued to

## Error Responses

**Missing or Invalid Token:**
```json
{
  "responseCode": 401,
  "message": "Not authenticated or invalid authToken"
}
```

