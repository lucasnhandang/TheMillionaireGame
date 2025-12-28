# Error Codes Specification

## Overview

This document defines all error codes used in the Millionaire Game protocol. Error codes follow HTTP-like conventions and are returned in the `responseCode` field of server responses.

## Error Response Format

```json
{
  "responseCode": 401,
  "message": "Invalid credentials"
}
```

---

## Success Codes

### 200 - OK
**Name:** OK

**Meaning:** Request completed successfully.

**Typical Use Cases:**
- Generic success notification
- Successful game actions
- Successful data retrieval

**Example:**
```json
{
  "responseCode": 200,
  "data": {
    "message": "Operation successful"
  }
}
```

---

### 201 - CREATED
**Name:** CREATED

**Meaning:** User successfully registered.

**Typical Use Cases:**
- Registration operations
- Resource creation

**Example:**
```json
{
  "responseCode": 201,
  "data": {
    "message": "Registration successful"
  }
}
```

---

## Client Error Codes (4xx)

### 400 - INVALID_DATA
**Name:** INVALID_DATA

**Meaning:** Malformed or invalid input data.

**Typical Use Cases:**
- Wrong data format
- Missing required fields
- Invalid JSON syntax
- Invalid parameters

**Example Messages:**
- "Invalid JSON format"
- "Missing username or password"
- "Missing requestType"

**Example:**
```json
{
  "responseCode": 400,
  "message": "Invalid JSON format"
}
```

---

### 401 - LOGIN_FAILED
**Name:** LOGIN_FAILED

**Meaning:** Wrong username or password.

**Typical Use Cases:**
- Login attempts with incorrect credentials
- Authentication failure

**Example Messages:**
- "Invalid credentials"
- "Wrong username or password"

**Example:**
```json
{
  "responseCode": 401,
  "message": "Invalid credentials"
}
```

---

### 402 - AUTH_ERROR
**Name:** AUTH_ERROR

**Meaning:** Missing or invalid authentication token.

**Typical Use Cases:**
- Token invalid
- Token expired
- Token missing
- Token doesn't belong to this connection

**Example Messages:**
- "Not authenticated or invalid authToken"
- "Missing authentication token"
- "Token expired"

**Example:**
```json
{
  "responseCode": 402,
  "message": "Not authenticated or invalid authToken"
}
```

---

### 403 - FORBIDDEN
**Name:** FORBIDDEN

**Meaning:** User is not allowed to perform this action.

**Typical Use Cases:**
- Forbidden admin actions
- Viewing others' private data
- Insufficient permissions
- Account is banned

**Example Messages:**
- "Access forbidden"
- "Account is banned"
- "Insufficient permissions"

**Example:**
```json
{
  "responseCode": 403,
  "message": "Account is banned"
}
```

---

### 404 - NOT_FOUND
**Name:** NOT_FOUND

**Meaning:** Requested resource does not exist.

**Typical Use Cases:**
- User not found
- Friend not found
- Friend request not found
- Game session missing
- Question not found
- Saved game not found

**Example Messages:**
- "User not found"
- "Friend not found"
- "Friend request not found"
- "No saved game found"
- "Question not found"

**Example:**
```json
{
  "responseCode": 404,
  "message": "No saved game found"
}
```

---

### 405 - USER_ALREADY_IN_GAME
**Name:** USER_ALREADY_IN_GAME

**Meaning:** User is already in an active game.

**Typical Use Cases:**
- Prevent double-joining a game
- Attempting to START while already in game
- Attempting to RESUME while in active game

**Example Messages:**
- "Already in a game"
- "User is already playing"

**Example:**
```json
{
  "responseCode": 405,
  "message": "Already in a game"
}
```

---

### 406 - USER_NOT_IN_GAME
**Name:** USER_NOT_IN_GAME

**Meaning:** User is not in any game session.

**Typical Use Cases:**
- Calling ANSWER when not in a game
- Calling GIVE_UP when not in a game
- Calling LIFELINE when not in a game

**Example Messages:**
- "Not in a game"
- "No active game session"

**Example:**
```json
{
  "responseCode": 406,
  "message": "Not in a game"
}
```

---

### 407 - LIFELINE_ALREADY_CHOSEN
**Name:** LIFELINE_ALREADY_CHOSEN

**Meaning:** User already used this lifeline.

**Typical Use Cases:**
- Lifeline limit enforcement
- Attempting to use 50/50 twice
- Attempting to use PHONE twice
- Attempting to use AUDIENCE twice

**Example Messages:**
- "Lifeline already used"
- "50/50 already used"

**Example:**
```json
{
  "responseCode": 407,
  "message": "Lifeline already used"
}
```

---

### 408 - QUESTION_TIMEOUT
**Name:** QUESTION_TIMEOUT

**Meaning:** User answered too late; question expired. Game ends with status "lost", player receives safe checkpoint prize from last passed checkpoint.

**Typical Use Cases:**
- Answer submitted after 30-second time limit
- Question timeout occurred even if still in game
- Distinguish from 406: timeout occurs even if still in game
- Game ends immediately, same as wrong answer (status "lost")

**Example Messages:**
- "Question timeout"
- "Answer submitted too late"

**Note:** When timeout occurs, game ends with status "lost" (same as wrong answer). Player receives safe checkpoint prize and score from the last safe checkpoint passed (questions 5, 10, or 15).

**Example:**
```json
{
  "responseCode": 408,
  "message": "Question timeout"
}
```

---

### 409 - CONFLICT
**Name:** CONFLICT

**Meaning:** Conflicting state or duplicate resource.

**Typical Use Cases:**
- Username already taken
- Adding an existing friend
- Admin replacing a question ID that already exists
- Duplicate resource creation

**Example Messages:**
- "Username already exists"
- "Friend already exists"

**Example:**
```json
{
  "responseCode": 409,
  "message": "Username already exists"
}
```

---

### 410 - WEAK_PASSWORD
**Name:** WEAK_PASSWORD

**Meaning:** Password does not meet security requirements.

**Typical Use Cases:**
- Password policy enforcement
- Password too short
- Password doesn't meet complexity requirements

**Password Validation Rules:**
A password is considered weak if it fails to meet any of the following requirements:
- **Minimum Length:** Password must be at least 8 characters long
- **Character Requirements:** Password must contain at least:
  - One uppercase letter (A-Z)
  - One lowercase letter (a-z)
  - One digit (0-9)

**Validation Process:**
1. Check minimum length (>= 8 characters)
2. Verify character requirements (uppercase, lowercase, digit)

**Example Messages:**
- "Password is too weak"
- "Password must be at least 8 characters"
- "Password must contain at least one uppercase letter, one lowercase letter, and one digit"

**Example:**
```json
{
  "responseCode": 410,
  "message": "Password must contain at least one uppercase letter, one lowercase letter, and one digit"
}
```

---

### 412 - INVALID_GAME_STATE
**Name:** INVALID_GAME_STATE

**Meaning:** Request is valid but game state does not allow it.

**Typical Use Cases:**
- Calling START twice
- RESUME before START
- GIVE_UP after game ended
- Invalid state transitions

**Example Messages:**
- "Invalid game state"
- "Cannot perform this action in current state"

**Example:**
```json
{
  "responseCode": 412,
  "message": "Invalid game state"
}
```

---

### 415 - UNKNOWN_REQUEST_TYPE
**Name:** UNKNOWN_REQUEST_TYPE

**Meaning:** Request "type" is not supported or unknown.

**Typical Use Cases:**
- Unsupported command name
- Protocol version mismatch
- Typo in requestType field

**Example Messages:**
- "Unknown request type: INVALID_TYPE"
- "Unsupported request type"

**Example:**
```json
{
  "responseCode": 415,
  "message": "Unknown request type: INVALID"
}
```

---

### 422 - UNPROCESSABLE_DATA
**Name:** UNPROCESSABLE_DATA

**Meaning:** JSON parsed successfully but data fields have invalid values.

**Typical Use Cases:**
- answerIndex out of range (must be 0-3)
- lifelineType invalid
- Invalid pagination sizes
- Invalid enum values
- Question number mismatch (e.g., answering question 2 when on question 1)
- Cannot add yourself as friend

**Example Messages:**
- "Invalid answerIndex: must be 0-3"
- "Invalid lifelineType"
- "Page number must be positive"
- "Question number mismatch: expected 1, got 2"
- "Cannot add yourself as friend"

**Example:**
```json
{
  "responseCode": 422,
  "message": "Invalid answerIndex: must be 0-3"
}
```

---

### 429 - TOO_MANY_REQUESTS
**Name:** TOO_MANY_REQUESTS

**Meaning:** Client is sending too many requests in a short period.

**Typical Use Cases:**
- Anti-spam protection
- Rate limiting enforcement
- DDoS protection

**Rate Limiting Rules:**
The server implements rate limiting to prevent abuse and ensure fair resource usage. Rate limits are enforced based on the following criteria:

**Per-IP Address Limits:**
- **General Requests:** Maximum 100 requests per minute per IP address
- **Authentication Requests (LOGIN, REGISTER):** Maximum 5 requests per minute per IP address
- **Game Actions (START, ANSWER, LIFELINE, etc.):** Maximum 30 requests per minute per authenticated user
- **Admin Requests:** Maximum 50 requests per minute per admin account

**Per-User Limits (Authenticated):**
- **General API Calls:** Maximum 200 requests per minute per authenticated user
- **Friend Operations:** Maximum 20 requests per minute per user
- **Leaderboard Queries:** Maximum 10 requests per minute per user

**Time Windows:**
- Rate limits are calculated using a sliding window algorithm
- Each time window is 60 seconds (1 minute)
- Requests are counted within the current window
- When a limit is exceeded, the client must wait until the window resets

**Rate Limit Headers (if supported):**
When a 429 error is returned, the response may include headers indicating:
- `X-RateLimit-Limit`: Maximum number of requests allowed
- `X-RateLimit-Remaining`: Number of requests remaining in current window
- `X-RateLimit-Reset`: Unix timestamp when the rate limit resets

**Rate Limit Exceeded Behavior:**
- First violation: Returns 429 error with message
- Repeated violations: May result in temporary IP ban (5-15 minutes)
- Severe abuse: May result in permanent IP ban

**Example Messages:**
- "Too many requests, please slow down"
- "Rate limit exceeded: Maximum 100 requests per minute"
- "Too many login attempts. Please try again in 60 seconds"
- "Rate limit exceeded. Please wait before making more requests"

**Example:**
```json
{
  "responseCode": 429,
  "message": "Rate limit exceeded: Maximum 100 requests per minute per IP address"
}
```

---

## Server Error Codes (5xx)

### 500 - SERVER_ERROR
**Name:** SERVER_ERROR

**Meaning:** Internal server error or unsupported request.

**Typical Use Cases:**
- Generic fallback for unexpected errors
- Unhandled exceptions
- Internal processing errors

**Example Messages:**
- "Internal server error"
- "An unexpected error occurred"

**Example:**
```json
{
  "responseCode": 500,
  "message": "Internal server error"
}
```

---

### 501 - DATABASE_ERROR
**Name:** DATABASE_ERROR

**Meaning:** Database connection failure or invalid DB query.

**Typical Use Cases:**
- DB offline
- SQL exception
- Connection timeout
- Query execution failure

**Example Messages:**
- "Database connection failed"
- "Database error occurred"

**Example:**
```json
{
  "responseCode": 501,
  "message": "Database connection failed"
}
```

---

### 502 - SERVICE_UNAVAILABLE
**Name:** SERVICE_UNAVAILABLE

**Meaning:** A required service is temporarily unavailable.

**Typical Use Cases:**
- Sub-services (scoring, queue, file storage) down
- Service restarting
- Maintenance mode

**Example Messages:**
- "Service temporarily unavailable"
- "Service is under maintenance"

**Example:**
```json
{
  "responseCode": 502,
  "message": "Service temporarily unavailable"
}
```

---

## Error Code Summary

| Code | Name | Category | Description |
|------|------|----------|-------------|
| 200 | OK | Success | Request completed successfully |
| 201 | CREATED | Success | User successfully registered |
| 400 | INVALID_DATA | Client Error | Malformed or invalid input data |
| 401 | LOGIN_FAILED | Client Error | Wrong username or password |
| 402 | AUTH_ERROR | Client Error | Missing or invalid authentication token |
| 403 | FORBIDDEN | Client Error | User not allowed to perform action |
| 404 | NOT_FOUND | Client Error | Requested resource does not exist |
| 405 | USER_ALREADY_IN_GAME | Client Error | User already in an active game |
| 406 | USER_NOT_IN_GAME | Client Error | User is not in any game session |
| 407 | LIFELINE_ALREADY_CHOSEN | Client Error | User already used this lifeline |
| 408 | QUESTION_TIMEOUT | Client Error | User answered too late (game ends as "lost", receives safe checkpoint prize) |
| 409 | CONFLICT | Client Error | Conflicting state or duplicate resource |
| 410 | WEAK_PASSWORD | Client Error | Password does not meet requirements |
| 412 | INVALID_GAME_STATE | Client Error | Request valid but game state doesn't allow it |
| 415 | UNKNOWN_REQUEST_TYPE | Client Error | Request type is not supported |
| 422 | UNPROCESSABLE_DATA | Client Error | Data fields have invalid values |
| 429 | TOO_MANY_REQUESTS | Client Error | Too many requests in short period |
| 500 | SERVER_ERROR | Server Error | Internal server error |
| 501 | DATABASE_ERROR | Server Error | Database connection failure |
| 502 | SERVICE_UNAVAILABLE | Server Error | Required service temporarily unavailable |

---

## Error Handling Best Practices

1. **Client Side:**
   - Always check `responseCode` before processing `data`
   - Display user-friendly error messages based on error codes
   - Implement retry logic for 5xx errors
   - Handle authentication errors (401, 402) by redirecting to login
   - Respect rate limiting (429) with exponential backoff

2. **Server Side:**
   - Always return appropriate error codes
   - Provide clear, descriptive error messages
   - Log all errors for debugging
   - Don't expose sensitive information in error messages
   - Validate input before processing

3. **Common Error Scenarios:**
   - Network timeout → Client should retry or show error
   - Invalid JSON → 400 INVALID_DATA
   - Missing authToken → 402 AUTH_ERROR
   - Game state conflicts → 405, 406, 412
   - Resource not found → 404 NOT_FOUND

