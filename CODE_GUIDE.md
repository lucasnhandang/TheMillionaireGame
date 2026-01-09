# 📚 HƯỚNG DẪN ĐỌC HIỂU CODE

> **Mục đích:** Giúp bạn nhanh chóng nắm bắt các phần code đã implement và có thể giải thích với thầy giáo khi phản biện.

---

## 📌 TỔNG QUAN KIẾN TRÚC

```
┌─────────────────────────────────────────────────────────────────────┐
│                          CLIENT (Qt GUI)                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │  LOGIN   │  │   GAME   │  │ LEADERBOARD│ │  ADMIN   │            │
│  │  Screen  │  │  Screen  │  │   Screen  │  │  Panel   │            │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘            │
│       └──────────────┴────────────┴────────────┴─────────┐          │
│                                                           │          │
│                    ┌──────────────────┐                   │          │
│                    │  Socket Client   │ ← JSON/TCP       │          │
│                    │  (socket_client) │                   │          │
│                    └────────┬─────────┘                   │          │
└─────────────────────────────┼─────────────────────────────┘          
                              │ '\n' delimiter                          
                              ▼                                         
┌─────────────────────────────────────────────────────────────────────┐
│                           SERVER (C++)                               │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │                    Stream Handler                           │     │
│  │  • Đọc/ghi TCP stream  • Buffer management  • JSON parsing  │     │
│  └──────────────────────────┬─────────────────────────────────┘     │
│                              ▼                                       │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │                   Session Manager                           │     │
│  │  • Quản lý session  • Track online users  • Auth tokens    │     │
│  └──────────────────────────┬─────────────────────────────────┘     │
│                              ▼                                       │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │                   Request Router                            │     │
│  │  • Route requests  • Authentication check  • Handler dispatch│    │
│  └──────────────────────────┬─────────────────────────────────┘     │
│                              ▼                                       │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐          │
│  │  Auth    │  Game    │  Social  │  Admin   │Connection│          │
│  │ Handlers │ Handlers │ Handlers │ Handlers │ Handlers │          │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 1️⃣ STREAM HANDLING - XỬ LÝ LUỒNG DỮ LIỆU

### 🎯 Vấn đề cần giải quyết
TCP là **byte stream**, không có ranh giới message. Khi gửi 2 message liên tiếp, có thể:
- Nhận cả 2 trong 1 lần `recv()`
- Nhận 1.5 message trong lần đầu, 0.5 còn lại ở lần sau
- Message bị **"dính"** hoặc **"xé"**

### 💡 Brainstorm: Tại sao chọn Newline Delimiter?

| Phương pháp | Ưu điểm | Nhược điểm |
|-------------|---------|------------|
| **Fixed length** | Đơn giản | Lãng phí bandwidth, không linh hoạt |
| **Length prefix** | Hiệu quả | Phức tạp hơn, cần serialize length |
| **Newline delimiter** ✅ | Dễ debug, human-readable | Cần escape nếu message chứa '\n' |

**→ Chọn Newline delimiter** vì JSON không chứa newline nếu không pretty-print.

### 📁 File chính: `server/stream_handler.cpp`

```cpp
// Cấu trúc buffer để quản lý partial messages
struct StreamBuffer {
    vector<char> data;    // Buffer data
    size_t read_pos;      // Vị trí đã đọc
    size_t write_pos;     // Vị trí ghi tiếp
    
    // Compact buffer khi đã đọc quá nửa
    void compact() {
        if (read_pos > 0 && read_pos < write_pos) {
            size_t data_size = write_pos - read_pos;
            memmove(data.data(), data.data() + read_pos, data_size);
            read_pos = 0;
            write_pos = data_size;
        }
    }
};
```

**Giải thích:** Buffer này giống như một "ống nước" - data vào từ socket (`write_pos`), ta đọc từ `read_pos`. Khi đọc xong thì compact lại để không bị tràn.

### 🔑 Hàm cốt lõi: `readMessage()`

```cpp
string StreamHandler::readMessage(int timeout_seconds) {
    // 1. Thử lấy message hoàn chỉnh từ buffer trước (đã nhận trước đó)
    string message = extractMessage();
    if (!message.empty()) {
        return message;  // Có message hoàn chỉnh trong buffer → trả về ngay
    }
    
    // 2. Nếu chưa có, đọc thêm từ socket
    while (connected_) {
        ssize_t bytes_read = readToBuffer(timeout_seconds);
        
        if (bytes_read < 0) {
            connected_ = false;  // Lỗi thật sự
            return "";
        } else if (bytes_read == 0) {
            return "";  // Timeout, vẫn connected
        }
        
        // 3. Thử extract message lần nữa
        message = extractMessage();
        if (!message.empty()) {
            return message;
        }
    }
    return "";
}
```

### 🔑 Hàm tách message từ buffer:

```cpp
string StreamHandler::extractMessage() {
    // Tìm ký tự '\n' trong buffer
    char* start = buffer_->data.data() + buffer_->read_pos;
    char* end = buffer_->data.data() + buffer_->write_pos;
    char* newline = find(start, end, '\n');  // Tìm delimiter
    
    if (newline == end) {
        return "";  // Chưa có message hoàn chỉnh
    }
    
    // Extract message (không bao gồm '\n')
    size_t message_length = newline - start;
    string message(start, message_length);
    
    // Update read position (skip qua '\n')
    buffer_->read_pos = (newline - buffer_->data.data()) + 1;
    
    return message;
}
```

### 📊 Minh họa:

```
Tình huống: Nhận 2.5 message trong 1 lần recv()

Buffer sau recv(): [{"type":"LOGIN"...}\n{"type":"START"...}\n{"type":"ANS]
                    ↑ read_pos                                              ↑ write_pos

extractMessage() lần 1:
→ Tìm '\n' đầu tiên
→ Trả về: {"type":"LOGIN"...}
→ read_pos di chuyển sau '\n'

Buffer: [{"type":"LOGIN"...}\n{"type":"START"...}\n{"type":"ANS]
                              ↑ read_pos                        ↑ write_pos

extractMessage() lần 2:
→ Trả về: {"type":"START"...}

Buffer: [{"type":"LOGIN"...}\n{"type":"START"...}\n{"type":"ANS]
                                                    ↑ read_pos  ↑ write_pos

extractMessage() lần 3:
→ Không tìm thấy '\n' → return ""
→ Chờ recv() tiếp để nhận phần còn lại
```

---

## 2️⃣ SOCKET I/O - GIAO TIẾP MẠNG

### 🎯 Vấn đề cần giải quyết
- Non-blocking I/O để server không bị "đứng" khi chờ client
- Xử lý các trường hợp: timeout, connection lost, partial write

### 💡 Brainstorm: Tại sao dùng `select()` và timeout?

| Phương pháp | Ưu điểm | Nhược điểm |
|-------------|---------|------------|
| **Blocking I/O** | Đơn giản | Server bị block, 1 client chậm → tất cả chờ |
| **Non-blocking + busy loop** | Responsive | CPU 100% usage |
| **select() + timeout** ✅ | CPU-efficient, có timeout | Hơi phức tạp |

### 📁 Hàm kiểm tra data có sẵn:

```cpp
bool StreamHandler::hasDataAvailable(int timeout_seconds) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd_, &read_fds);  // Theo dõi socket này
    
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    // select() chờ tối đa timeout_seconds
    // - result > 0: có data
    // - result = 0: timeout
    // - result < 0: error
    int result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    
    return result > 0 && FD_ISSET(socket_fd_, &read_fds);
}
```

**Giải thích:** `select()` như một "bảo vệ" - nó chờ và báo khi socket có data, hoặc hết thời gian chờ.

### 📁 Hàm ghi message với retry:

```cpp
bool StreamHandler::writeMessage(const string& message) {
    string message_with_newline = message + '\n';  // Thêm delimiter
    
    const char* data = message_with_newline.c_str();
    size_t total_bytes = message_with_newline.length();
    size_t bytes_sent = 0;
    
    while (bytes_sent < total_bytes) {
        ssize_t result = send(socket_fd_, data + bytes_sent, 
                              total_bytes - bytes_sent, 0);
        
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Buffer đầy, chờ một chút rồi thử lại
                usleep(1000);  // 1ms
                continue;
            }
            return false;  // Lỗi thật sự
        }
        
        bytes_sent += result;  // Đã gửi được một phần
    }
    return true;
}
```

**Điểm quan trọng:** `send()` có thể chỉ gửi được một phần data (kernel buffer đầy) → cần loop để gửi hết.

---

## 3️⃣ PROTOCOL DESIGN - THIẾT KẾ GIAO THỨC

### 🎯 Vấn đề cần giải quyết
- Client và Server cần "nói chung ngôn ngữ"
- Dễ mở rộng, thêm feature mới
- Debug được

### 💡 Brainstorm: Tại sao chọn JSON over TCP?

| Protocol | Ưu điểm | Nhược điểm |
|----------|---------|------------|
| **Binary (protobuf)** | Nhỏ gọn, nhanh | Khó debug, cần schema |
| **XML** | Human-readable | Verbose, chậm parse |
| **JSON** ✅ | Human-readable, mọi ngôn ngữ hỗ trợ | Hơi lớn hơn binary |

### 📄 Cấu trúc message:

```json
// Request: Client → Server
{
  "requestType": "ANSWER",
  "data": {
    "authToken": "abc123...",
    "gameId": 1,
    "questionNumber": 5,
    "answerIndex": 2
  }
}

// Response: Server → Client
{
  "responseCode": 200,
  "data": {
    "correct": true,
    "pointsEarned": 25,
    "totalScore": 150
  }
}

// Error Response
{
  "responseCode": 406,
  "message": "Not in a game"
}
```

### 📁 Request Router - Điều hướng request:

```cpp
// File: server/request_router.cpp

string RequestRouter::processRequest(const string& request, int client_fd) {
    // 1. Lấy loại request
    string request_type = StreamUtils::extractRequestType(request);
    
    // 2. Lấy session của client
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    
    // 3. Các request không cần auth
    if (request_type == "LOGIN") {
        return AuthHandlers::handleLogin(request, *session, client_fd);
    } else if (request_type == "REGISTER") {
        return AuthHandlers::handleRegister(request, *session, client_fd);
    }
    
    // 4. Các request còn lại cần auth
    string username = AuthManager::getInstance().requireAuth(request, *session, client_fd);
    if (username.empty()) {
        return StreamUtils::createErrorResponse(402, "Not authenticated");
    }
    
    // 5. Route đến handler phù hợp
    if (request_type == "START") {
        return GameHandlers::handleStart(request, *session, client_fd);
    } else if (request_type == "ANSWER") {
        return GameHandlers::handleAnswer(request, *session, client_fd);
    } else if (request_type == "GIVE_UP") {
        return GameHandlers::handleGiveUp(request, *session, client_fd);
    }
    // ... các handler khác
    
    return StreamUtils::createErrorResponse(415, "Unknown request type");
}
```

**Pattern:** Router Pattern - một điểm vào duy nhất, phân phối đến các handler chuyên biệt.

---

## 4️⃣ FORFEIT FUNCTIONALITY - CHỨC NĂNG BỎ CUỘC

### 🎯 Vấn đề cần giải quyết
- Player muốn dừng chơi và nhận tiền thưởng hiện tại
- Cần kết thúc game "sạch sẽ" (clear state, stop timer)

### 📁 File: `server/request_handlers/game_handlers.cpp`

```cpp
string handleGiveUp(const string& request, ClientSession& session, int client_fd) {
    // 1. Kiểm tra user có đang trong game không
    if (!session.in_game) {
        // Kiểm tra trong database (trường hợp reconnect)
        GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
        if (active_game.id == 0) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }
        // Restore session từ database
        session.in_game = true;
        session.game_id = active_game.id;
        session.current_question_number = active_game.current_question_number;
        // ...
    }

    // 2. Validate gameId (tránh gửi sai game)
    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId");
    }

    // 3. Lưu thông tin trước khi clear
    long long final_prize = session.current_prize;
    int final_question_number = session.current_question_number;
    int total_score = session.total_score;

    // 4. QUAN TRỌNG: Clear state NGAY LẬP TỨC
    int old_game_id = game_id;
    session.in_game = false;
    session.game_id = 0;
    session.current_question_number = 0;
    session.used_lifelines.clear();
    
    // 5. Stop timer
    GameTimer::getInstance().stopTimer(old_game_id);
    
    // 6. Update database (status = 'quit')
    Database::getInstance().endGame(old_game_id, "quit", total_score, final_prize);

    // 7. Gửi notification GAME_END
    string game_end_data = "{\"gameId\":" + to_string(old_game_id) +
                          ",\"status\":\"quit\"" +
                          ",\"finalPrize\":" + to_string(final_prize) +
                          ",\"totalScore\":" + to_string(total_score) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}
```

### 📊 Flow diagram:

```
Client                           Server
  │                                 │
  │──── GIVE_UP request ───────────>│
  │                                 │
  │                        ┌────────┴────────┐
  │                        │ 1. Validate game│
  │                        │ 2. Save final   │
  │                        │    prize/score  │
  │                        │ 3. Clear session│
  │                        │ 4. Stop timer   │
  │                        │ 5. Update DB    │
  │                        └────────┬────────┘
  │                                 │
  │<─── GAME_END notification ──────│
  │<─── Success response ───────────│
  │                                 │
  ▼                                 ▼
```

---

## 5️⃣ LOGIN SESSION & SESSION MANAGEMENT

### 🎯 Vấn đề cần giải quyết
- Theo dõi ai đang online
- Map giữa socket fd và user information
- Xử lý disconnect/reconnect

### 💡 Brainstorm: Tại sao dùng token-based auth?

| Phương pháp | Ưu điểm | Nhược điểm |
|-------------|---------|------------|
| **Session ID in cookie** | Phổ biến web | TCP không có cookie |
| **Password mỗi request** | Đơn giản | Không an toàn, chậm |
| **Token-based** ✅ | Stateless, an toàn | Cần quản lý token |

### 📁 File: `server/session_manager.h`

```cpp
// Cấu trúc lưu thông tin một client session
struct ClientSession {
    std::string client_ip;           // IP của client
    time_t connected_time;           // Thời điểm connect
    time_t last_ping_time;           // Lần ping cuối (detect disconnect)
    std::string auth_token;          // Token xác thực
    std::string username;            // Username sau login
    std::string role;                // "user" hoặc "admin"
    bool authenticated;              // Đã login chưa
    
    // Game state
    bool in_game;                    // Đang chơi không
    int game_id;                     // ID game hiện tại
    int current_question_number;     // Câu hỏi hiện tại (1-15)
    int current_prize;               // Tiền thưởng hiện tại
    int total_score;                 // Điểm tích lũy
    
    // Lifeline tracking
    std::set<std::string> used_lifelines;
    std::map<int, std::set<std::string>> used_lifelines_per_question;
    
    // Timer state (cho lifeline pause)
    bool timer_paused;
    int paused_time_remaining;
};
```

### 📁 File: `server/session_manager.cpp`

```cpp
// Singleton pattern - chỉ có 1 instance quản lý tất cả session
SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

// Tạo session khi client connect
void SessionManager::createSession(int client_fd, const string& client_ip) {
    lock_guard<mutex> lock(clients_mutex_);  // Thread-safe
    active_clients_[client_fd] = ClientSession(client_ip);
}

// Kiểm tra user online (cho friend list)
bool SessionManager::isUserOnline(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);
    return online_users_.find(username) != online_users_.end();
}

// Tìm client fd từ game_id (để gửi notification)
int SessionManager::getClientFdByGameId(int game_id) {
    lock_guard<mutex> lock(clients_mutex_);
    for (const auto& pair : active_clients_) {
        if (pair.second.in_game && pair.second.game_id == game_id) {
            return pair.first;
        }
    }
    return -1;
}
```

### 📁 Auth Manager - Quản lý token:

```cpp
// File: server/auth_manager.cpp

// Generate token 32 ký tự hex (128 bit entropy)
string AuthManager::generateToken() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, 15);
    
    stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << hex << dis(gen);  // 0-9, a-f
    }
    return ss.str();  // VD: "a3f7b2c9e1d4..."
}

// Validate token thuộc về đúng client
bool AuthManager::validateToken(const string& token, int client_fd) {
    lock_guard<mutex> lock(tokens_mutex_);
    auto it = token_to_fd_.find(token);
    if (it != token_to_fd_.end()) {
        return it->second == client_fd;  // Token phải match với fd
    }
    return false;
}
```

### 📁 Login handler:

```cpp
// File: server/request_handlers/auth_handlers.cpp

string handleLogin(const string& request, ClientSession& session, int client_fd) {
    // 1. Parse username/password từ request
    string username = JsonUtils::extractString(request, "username");
    string password = JsonUtils::extractString(request, "password");
    
    // 2. Kiểm tra banned
    if (Database::getInstance().isUserBanned(username)) {
        return StreamUtils::createErrorResponse(403, "Account is banned");
    }
    
    // 3. Xác thực với database
    bool login_success = Database::getInstance().authenticateUser(username, password);
    if (!login_success) {
        return StreamUtils::createErrorResponse(401, "Invalid credentials");
    }
    
    // 4. Generate và lưu token
    string token = AuthManager::getInstance().generateToken();
    session.auth_token = token;
    session.username = username;
    session.role = Database::getInstance().getUserRole(username);
    session.authenticated = true;
    
    // 5. Register token và mark user online
    AuthManager::getInstance().registerToken(token, client_fd, username);
    SessionManager::getInstance().addOnlineUser(username);
    
    // 6. Return token cho client
    string data = "{\"authToken\":\"" + token + "\",\"username\":\"" + username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}
```

### 📊 Login flow:

```
┌──────────┐                    ┌──────────┐                 ┌──────────┐
│  Client  │                    │  Server  │                 │ Database │
└────┬─────┘                    └────┬─────┘                 └────┬─────┘
     │                               │                            │
     │─── LOGIN(user, pass) ────────>│                            │
     │                               │── authenticateUser() ─────>│
     │                               │<─── true/false ────────────│
     │                               │                            │
     │                      ┌────────┴────────┐                   │
     │                      │ Generate token  │                   │
     │                      │ Store in memory │                   │
     │                      │ Mark online     │                   │
     │                      └────────┬────────┘                   │
     │                               │                            │
     │<─── {authToken: "abc..."} ────│                            │
     │                               │                            │
```

---

## 6️⃣ GAME START LOGIC - LOGIC BẮT ĐẦU GAME

### 🎯 Vấn đề cần giải quyết
- Tạo game session mới
- Pre-select 15 câu hỏi (tránh trùng lặp trong 1 game)
- Xử lý trường hợp đã có saved game

### 📁 File: `server/request_handlers/game_handlers.cpp`

```cpp
string handleStart(const string& request, ClientSession& session, int client_fd) {
    // 1. Kiểm tra nếu có game active trong DB → end nó trước
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
        // End game cũ
        GameTimer::getInstance().stopTimer(active_game.id);
        Database::getInstance().endGame(active_game.id, "quit", ...);
    }

    // 2. Kiểm tra saved game
    bool override_saved = JsonUtils::extractBool(request, "overrideSavedGame", false);
    GameProgress saved = GameStateManager::getInstance().loadGameProgress(session.username);
    if (saved.level > 0 && !override_saved) {
        return StreamUtils::createErrorResponse(412, 
            "You have a saved game. Use RESUME or set overrideSavedGame=true");
    }

    // 3. Tạo game session trong database
    int game_id = Database::getInstance().createGameSession(session.username);

    // 4. PRE-SELECT 15 câu hỏi (quan trọng!)
    // 5 easy (level 0), 5 medium (level 1), 5 hard (level 2)
    vector<Question> easy = Database::getInstance().getRandomQuestions(0, 5);
    vector<Question> medium = Database::getInstance().getRandomQuestions(1, 5);
    vector<Question> hard = Database::getInstance().getRandomQuestions(2, 5);
    
    // Lưu vào game_questions table
    int order = 1;
    for (const Question& q : easy) {
        Database::getInstance().addGameQuestion(game_id, order++, q.id);
    }
    for (const Question& q : medium) {
        Database::getInstance().addGameQuestion(game_id, order++, q.id);
    }
    for (const Question& q : hard) {
        Database::getInstance().addGameQuestion(game_id, order++, q.id);
    }

    // 5. Load câu hỏi đầu tiên
    Question first_question = Database::getInstance().getGameQuestion(game_id, 1);

    // 6. Update session state
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 0;  // Easy
    session.current_prize = ScoringSystem::getInstance().getPrizeForLevel(0, 1);
    session.total_score = 0;
    session.used_lifelines.clear();

    // 7. Start timer (30 giây)
    GameTimer::getInstance().startQuestionTimer(game_id);

    // 8. Gửi notifications
    NotificationUtils::sendNotification(client_fd, "GAME_START", game_start_data);
    NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);

    return StreamUtils::createSuccessResponse(200, data);
}
```

### 💡 Tại sao Pre-select 15 câu hỏi?

| Cách tiếp cận | Vấn đề |
|---------------|--------|
| Random mỗi câu | Có thể bị trùng câu hỏi trong 1 game |
| **Pre-select** ✅ | Đảm bảo 15 câu unique, có thể resume chính xác |

---

## 7️⃣ GAME DATA SAVING - LƯU TIẾN TRÌNH

### 🎯 Vấn đề cần giải quyết
- Cho phép user thoát giữa chừng và quay lại sau
- Lưu đủ thông tin để resume: question, score, time, lifelines

### 📁 Hàm SAVE_GAME:

```cpp
string handleSaveGame(const string& request, ClientSession& session, int client_fd) {
    // 1. STOP TIMER TRƯỚC để tránh timeout khi đang save
    int time_remaining = GameTimer::getInstance().getRemainingTime(session.game_id);
    GameTimer::getInstance().stopTimer(session.game_id);
    session.timer_paused = true;
    session.paused_time_remaining = time_remaining;

    // 2. Collect lifelines đã dùng cho câu hiện tại
    vector<string> used_lifelines;
    auto it = session.used_lifelines_per_question.find(session.current_question_number);
    if (it != session.used_lifelines_per_question.end()) {
        for (const string& lifeline : it->second) {
            used_lifelines.push_back(lifeline);
        }
    }

    // 3. Lưu vào saved_games table
    bool success = Database::getInstance().saveGameProgress(
        session.username,
        session.game_id,
        session.current_question_number,  // Câu đang chơi
        session.current_prize,
        session.total_score,
        time_remaining,                    // Thời gian còn lại
        used_lifelines                     // Lifelines đã dùng
    );

    // 4. Clear session (user không còn "in game")
    session.in_game = false;
    session.game_id = 0;
    // ...

    return StreamUtils::createSuccessResponse(200, "{\"message\":\"Game saved\"}");
}
```

### 📁 Hàm RESUME:

```cpp
string handleResume(const string& request, ClientSession& session, int client_fd) {
    // 1. Load saved game từ database
    GameSession saved_game = Database::getInstance().loadGameProgress(session.username);
    if (saved_game.id == 0) {
        return StreamUtils::createErrorResponse(404, "No saved game found");
    }

    // 2. Lấy time_remaining và lifelines đã lưu
    int time_remaining = Database::getInstance().getSavedGameTimeRemaining(session.username);
    vector<string> saved_lifelines = Database::getInstance().getSavedGameLifelines(session.username);

    // 3. Restore session state
    session.in_game = true;
    session.game_id = saved_game.id;
    session.current_question_number = saved_game.current_question_number;
    session.current_prize = saved_game.current_prize;
    session.total_score = saved_game.total_score;
    
    // Restore lifelines
    for (const string& lifeline : saved_lifelines) {
        session.used_lifelines.insert(lifeline);
        session.used_lifelines_per_question[saved_game.current_question_number].insert(lifeline);
    }

    // 4. Lấy câu hỏi từ game_questions (đã pre-select)
    Question current_question = Database::getInstance()
        .getGameQuestion(saved_game.id, saved_game.current_question_number);

    // 5. Gửi notifications
    NotificationUtils::sendNotification(client_fd, "GAME_START", ...);
    NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", ...);

    // 6. Resume timer với time_remaining đã lưu
    GameTimer::getInstance().resumeTimerWithTime(saved_game.id, time_remaining);

    return StreamUtils::createSuccessResponse(200, data);
}
```

### 📊 Save/Resume flow:

```
SAVE:                                RESUME:
┌───────────┐                       ┌───────────┐
│ Playing   │                       │ Not in    │
│ Question 5│                       │ game      │
│ 20s left  │                       │           │
│ Score: 100│                       │           │
└─────┬─────┘                       └─────┬─────┘
      │                                   │
      │ SAVE_GAME                         │ RESUME
      ▼                                   ▼
┌───────────────────────────────────────────────────┐
│                   DATABASE                         │
│  saved_games: {user, game_id, q=5, time=20,       │
│                score=100, lifelines=["5050"]}      │
└───────────────────────────────────────────────────┘
      │                                   │
      │ Clear session                     │ Restore session
      ▼                                   ▼
┌───────────┐                       ┌───────────┐
│ Not in    │                       │ Playing   │
│ game      │                       │ Question 5│
│           │                       │ 20s left  │
│           │                       │ Score: 100│
└───────────┘                       └───────────┘
```

---

## 8️⃣ PLAYER ONLINE STATUS - THEO DÕI TRẠNG THÁI ONLINE

### 🎯 Vấn đề cần giải quyết
- Friend list cần hiển thị ai đang online
- Detect disconnect để cleanup session

### 📁 Online tracking:

```cpp
// Khi login thành công
SessionManager::getInstance().addOnlineUser(username);

// Kiểm tra online (cho friend status)
bool SessionManager::isUserOnline(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);
    return online_users_.find(username) != online_users_.end();
}

// Khi disconnect/logout
void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        // Nếu đang trong game → end game
        if (!session->username.empty()) {
            GameSession active_game = Database::getInstance()
                .getActiveGameSession(session->username);
            if (active_game.id > 0) {
                // End game với status "quit"
                GameTimer::getInstance().stopTimer(active_game.id);
                Database::getInstance().endGame(active_game.id, "quit", ...);
                
                // Gửi GAME_END notification
                NotificationUtils::sendNotification(client_fd, "GAME_END", ...);
            }
        }
        
        // Cleanup auth
        if (!session->auth_token.empty()) {
            AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
        }
        
        // Mark offline
        if (!session->username.empty()) {
            SessionManager::getInstance().removeOnlineUser(session->username);
        }
    }
    
    // Remove session
    SessionManager::getInstance().removeSession(client_fd);
}
```

### 💡 Thread-safety với mutex:

```cpp
// Mọi access vào shared data đều cần lock
void SessionManager::addOnlineUser(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);  // Auto unlock khi ra khỏi scope
    online_users_.insert(username);
}
```

---

## 📝 TÓM TẮT CÁC ĐIỂM QUAN TRỌNG CHO PHẢN BIỆN

### 1. Stream Handler
- **Vấn đề:** TCP không có message boundary
- **Giải pháp:** Newline delimiter + buffer management
- **Code key:** `extractMessage()`, `readToBuffer()`

### 2. Socket I/O
- **Vấn đề:** Blocking I/O làm server đứng
- **Giải pháp:** `select()` với timeout
- **Code key:** `hasDataAvailable()`, `writeMessage()` với retry

### 3. Protocol
- **Format:** JSON over TCP với '\n' delimiter
- **Pattern:** Request-Response + Server notifications
- **Routing:** Router pattern trong `request_router.cpp`

### 4. Forfeit (GIVE_UP)
- **Flow:** Validate → Save state → Clear session → Stop timer → Update DB → Notify
- **Quan trọng:** Clear state NGAY LẬP TỨC để tránh race condition

### 5. Session Management
- **Token-based:** 32 char hex token
- **Tracking:** `active_clients_` map fd → session, `online_users_` set
- **Thread-safe:** mutex lock mọi access

### 6. Game Start
- **Pre-select:** 15 câu hỏi lưu vào `game_questions` table
- **Level:** 1-5 easy, 6-10 medium, 11-15 hard
- **Notifications:** GAME_START → QUESTION_INFO

### 7. Save/Resume
- **Save:** Stop timer → Save (question, time, score, lifelines) → Clear session
- **Resume:** Load → Restore session → Send notifications → Resume timer

### 8. Online Status
- **Track:** `online_users_` set
- **Cleanup:** On disconnect, end active game, unregister token, mark offline

---

## 🚀 CÂU HỎI PHẢN BIỆN CÓ THỂ GẶP

**Q: Tại sao dùng newline làm delimiter thay vì length prefix?**
> A: JSON không chứa newline khi serialize, dễ debug bằng telnet/netcat, human-readable.

**Q: Làm sao xử lý khi message bị "xé" giữa chừng?**
> A: Buffer accumulation - đọc vào buffer, tìm '\n', nếu chưa có thì tiếp tục đọc.

**Q: Tại sao cần pre-select 15 câu hỏi khi start game?**
> A: Tránh trùng câu trong 1 game, cho phép resume chính xác câu đang chơi.

**Q: Làm sao đảm bảo thread-safe khi nhiều client cùng lúc?**
> A: Mutex lock mọi access vào shared data (sessions, online_users, timers).

**Q: Khi user disconnect giữa game thì sao?**
> A: `cleanupClient()` detect và end game với status "quit", gửi GAME_END notification.

---

*Tài liệu này được viết để giúp hiểu code nhanh chóng và chuẩn bị cho phản biện.*
