# Communication Protocol Configuration Verification

This document verifies the communication protocol configuration claims against the actual implementation in the codebase.

## Verification Results

### 1. Communication Type: Unicast communication between each client and the server

**Status: ✅ CORRECT**

**Evidence:**
- `server/server_core.cpp:40`: Creates TCP socket using `socket(AF_INET, SOCK_STREAM, 0)`
- TCP sockets provide unicast (one-to-one) communication between each client and the server
- Each client establishes its own separate TCP connection

**Code Reference:**
```cpp
// server/server_core.cpp:40
server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
```

---

### 2. Connection Model: One TCP socket per connected player

**Status: ✅ CORRECT**

**Evidence:**
- `server/event_loop.cpp:254-290`: Each client connection is accepted and assigned a unique file descriptor (`client_fd`)
- `server/event_loop.cpp:290`: Each client gets its own session: `SessionManager::getInstance().createSession(client_fd, string(client_ip));`
- `server/event_loop.cpp:281-287`: Each client socket is added to the poll file descriptor set separately
- The server maintains one socket per connected client

**Code References:**
```cpp
// server/event_loop.cpp:254
int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

// server/event_loop.cpp:290
SessionManager::getInstance().createSession(client_fd, string(client_ip));

// server/event_loop.cpp:281-287
struct pollfd client_pfd;
client_pfd.fd = client_fd;
client_pfd.events = POLLIN;
fd_to_index_[client_fd] = poll_fds_.size();
poll_fds_.push_back(client_pfd);
```

---

### 3. Maximum Message Size: 4096 bytes (4 KB) per message

**Status: ⚠️ PARTIALLY CORRECT / NEEDS CLARIFICATION**

**Evidence:**
- `server/stream_handler.h:37`: StreamHandler constructor uses default buffer size of 4096 bytes
- `server/stream_handler.cpp:49`: Buffer can grow dynamically: `size_t new_size = max(data.size() * 2, write_pos + needed);`
- `server/event_loop.cpp:304`: Read buffer is 4096 bytes: `char buffer[4096];`
- `client/socket_client.cpp:134`: Client read buffer is 4096 bytes: `char buffer[4096];`

**Important Note:** 
- 4096 bytes is the **initial buffer size**, not a hard maximum
- The buffer can grow dynamically to accommodate larger messages
- Messages can exceed 4 KB as the buffer automatically expands

**Code References:**
```cpp
// server/stream_handler.h:37
explicit StreamHandler(int socket_fd, size_t buffer_size = 4096);

// server/stream_handler.cpp:49
size_t new_size = max(data.size() * 2, write_pos + needed);
data.resize(new_size);

// server/event_loop.cpp:304
char buffer[4096];

// client/socket_client.cpp:134
char buffer[4096];
```

---

### 4. Timeout: 30 seconds of inactivity before the connection is considered inactive

**Status: ❌ INCORRECT**

**Evidence:**
- `server/config.json:7`: `"connection_timeout_seconds": 300` - This is **300 seconds (5 minutes)**, not 30 seconds
- `server/config.cpp:89`: Default value is 300 seconds: `config.connection_timeout_seconds = extractIntValue(json, "connection_timeout_seconds", 300);`
- `server/client_handler.cpp:18`: Uses the configured timeout: `handler->setReadTimeout(config.connection_timeout_seconds, 0);`

**Actual Configuration:**
- Connection timeout: **300 seconds (5 minutes)**
- This is the timeout for read operations on the socket
- The report incorrectly states 30 seconds

**Code References:**
```json
// server/config.json:7
"connection_timeout_seconds": 300
```

```cpp
// server/config.cpp:89
config.connection_timeout_seconds = extractIntValue(json, "connection_timeout_seconds", 300);

// server/client_handler.cpp:18
handler->setReadTimeout(config.connection_timeout_seconds, 0);
```

---

### 5. Retry Policy: The client retries up to 3 times before disconnecting

**Status: ❌ INCORRECT**

**Evidence:**
- No retry logic found in the client code
- `client/socket_client.cpp:22-60`: The `connect()` function has no retry mechanism
- If `connect()` fails, it immediately returns `false` and closes the socket
- No loop or retry counter exists in the connection logic

**Actual Behavior:**
- Client attempts connection **once**
- On failure, client disconnects immediately
- **No retry mechanism is implemented**

**Code Reference:**
```cpp
// client/socket_client.cpp:22-60
bool SocketClient::connect() {
    // ... socket creation ...
    if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        close(sockfd_);
        sockfd_ = -1;
        return false;  // No retry, returns immediately
    }
    // ...
}
```

---

## Summary Table

| Claim | Status | Actual Value | File + Line |
|-------|--------|--------------|-------------|
| Unicast communication | ✅ CORRECT | TCP unicast | `server_core.cpp:40` |
| One TCP socket per player | ✅ CORRECT | One socket per client | `event_loop.cpp:254-290` |
| Maximum message size 4096 bytes | ⚠️ PARTIALLY CORRECT | Initial buffer 4096, can grow | `stream_handler.h:37` |
| Timeout 30 seconds | ❌ INCORRECT | **300 seconds (5 minutes)** | `config.json:7` |
| Retry policy (3 times) | ❌ INCORRECT | **No retry logic** | `socket_client.cpp:22-60` |

---

## Recommendations

1. **Timeout Configuration**: Update the report to reflect the actual timeout of 300 seconds (5 minutes), not 30 seconds.

2. **Retry Policy**: Either:
   - Remove the retry policy claim from the report, OR
   - Implement retry logic in the client code (`client/socket_client.cpp`) if this feature is required

3. **Message Size**: Clarify that 4096 bytes is the initial buffer size, and messages can exceed this limit as the buffer grows dynamically.

---

## Files Examined

### Server Side:
- `server/server_core.cpp` - Server initialization and socket creation
- `server/event_loop.cpp` - Client connection handling
- `server/client_handler.cpp` - Client request handling
- `server/stream_handler.h` - Stream handler interface
- `server/stream_handler.cpp` - Stream handler implementation
- `server/config.h` - Configuration structure
- `server/config.cpp` - Configuration loading
- `server/config.json` - Configuration file

### Client Side:
- `client/socket_client.h` - Socket client interface
- `client/socket_client.cpp` - Socket client implementation

---

*Generated on: 2026-01-29*
*Verification based on actual codebase implementation*
