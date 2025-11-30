#include "stream_handler.h"
#include "logger.h"
#include "config.h"
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <map>
#include <unordered_map>
#include <set>
#include <random>
#include <sstream>
#include <cerrno>
#include <iomanip>
#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
using namespace MillionaireGame;

class GameServer {
public:
    GameServer(const ServerConfig& config)
        : config_(config), running_(false), accepting_(true), server_fd_(-1) {
        LogLevel log_level = LogLevel::INFO;
        if (config.log_level == "DEBUG") log_level = LogLevel::DEBUG;
        else if (config.log_level == "WARNING") log_level = LogLevel::WARNING;
        else if (config.log_level == "ERROR") log_level = LogLevel::ERROR;
        
        Logger::getInstance().initialize(config.log_file, log_level);
    }

    ~GameServer() {
        stop();
    }

    bool start() {
        if (running_) {
            LOG_WARNING("Server is already running");
            return false;
        }

        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            LOG_ERROR("Failed to create socket: " + string(strerror(errno)));
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            LOG_ERROR("Failed to set SO_REUSEADDR: " + string(strerror(errno)));
            close(server_fd_);
            return false;
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config_.port);

        if (::bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            LOG_ERROR("Failed to bind socket on port " + to_string(config_.port) + ": " + string(strerror(errno)));
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, config_.max_clients) < 0) {
            LOG_ERROR("Failed to listen on socket: " + string(strerror(errno)));
            close(server_fd_);
            return false;
        }

        running_ = true;
        accepting_ = true;
        LOG_INFO("Server started on port " + to_string(config_.port));

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        GameServer::instance_ = this;

        return true;
    }

    void run() {
        if (!running_) {
            LOG_ERROR("Server not started. Call start() first.");
            return;
        }

        while (running_) {
            if (!accepting_) {
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            }

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

            if (client_fd < 0) {
                if (running_ && accepting_) {
                    LOG_ERROR("Failed to accept connection: " + string(strerror(errno)));
                }
                continue;
            }

            {
                lock_guard<mutex> lock(clients_mutex_);
                if (active_clients_.size() >= static_cast<size_t>(config_.max_clients)) {
                    LOG_WARNING("Max clients reached, rejecting connection");
                    close(client_fd);
                    continue;
                }
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            LOG_INFO("New client connected from " + string(client_ip) + ":" + to_string(ntohs(client_addr.sin_port)));

            thread client_thread(&GameServer::handleClient, this, client_fd, string(client_ip));
            client_thread.detach();
        }

        waitForClientsToFinish();
    }

    void stopAccepting() {
        accepting_ = false;
        LOG_INFO("Stopped accepting new connections. Waiting for existing clients to finish...");
    }

    void stop() {
        if (!running_) {
            return;
        }

        stopAccepting();
        
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }

        running_ = false;
        
        {
            lock_guard<mutex> lock(clients_mutex_);
            for (auto& pair : active_clients_) {
                if (pair.second.handler) {
                    pair.second.handler->close();
                }
            }
        }

        Logger::getInstance().close();
        LOG_INFO("Server stopped");
    }

private:
    struct ClientSession {
        unique_ptr<StreamHandler> handler;
        string client_ip;
        time_t connected_time;
        time_t last_ping_time;
        string auth_token;
        string username;
        string role;  // "user" or "admin"
        bool authenticated;
        bool in_game;
        int game_id;  // Current game session ID
        int current_question_number;  // Current question number (1-15)
        int current_level;
        int current_prize;
        int total_score;
        set<string> used_lifelines;  // Track which lifelines have been used

        ClientSession(unique_ptr<StreamHandler> h, const string& ip)
            : handler(move(h)), client_ip(ip), connected_time(time(nullptr)),
              last_ping_time(time(nullptr)), authenticated(false), 
              in_game(false), game_id(0), current_question_number(0),
              current_level(0), current_prize(0), total_score(0), role("user") {}
    };

    ServerConfig config_;
    atomic<bool> running_;
    atomic<bool> accepting_;
    int server_fd_;
    mutex clients_mutex_;
    mutex tokens_mutex_;
    unordered_map<int, ClientSession> active_clients_;
    unordered_map<string, int> token_to_fd_;
    unordered_map<string, string> username_to_token_;
    set<string> online_users_;
    static GameServer* instance_;

    static void signalHandler(int sig) {
        if (instance_) {
            instance_->stopAccepting();
        }
    }

    void waitForClientsToFinish() {
        LOG_INFO("Waiting for all clients to disconnect...");
        while (true) {
            {
                lock_guard<mutex> lock(clients_mutex_);
                if (active_clients_.empty()) {
                    break;
                }
            }
            this_thread::sleep_for(chrono::seconds(1));
        }
        LOG_INFO("All clients disconnected");
    }

    string generateAuthToken() {
        static random_device rd;
        static mt19937 gen(rd());
        static uniform_int_distribution<> dis(0, 15);
        
        stringstream ss;
        for (int i = 0; i < 32; i++) {
            ss << hex << dis(gen);
        }
        return ss.str();
    }

    bool validateAuthToken(const string& token, int& client_fd) {
        lock_guard<mutex> lock(tokens_mutex_);
        auto it = token_to_fd_.find(token);
        if (it != token_to_fd_.end()) {
            client_fd = it->second;
            return true;
        }
        return false;
    }

    void handleClient(int client_fd, const string& client_ip) {
        auto handler = make_unique<StreamHandler>(client_fd);
        handler->setReadTimeout(config_.connection_timeout_seconds, 0);
        handler->setWriteTimeout(10, 0);

        StreamHandler* handler_ptr = handler.get();

        {
            lock_guard<mutex> lock(clients_mutex_);
            active_clients_[client_fd] = ClientSession(move(handler), client_ip);
        }

        LOG_INFO("Client handler started for " + client_ip);

        sendConnectionMessage(handler_ptr);

        try {
            while (running_ && handler_ptr->isConnected()) {
                string request = handler_ptr->readMessage(config_.ping_timeout_seconds + 5);

                if (request.empty()) {
                    if (!handler_ptr->isConnected()) {
                        LOG_INFO("Client " + client_ip + " disconnected");
                        break;
                    }
                    continue;
                }

                if (!StreamUtils::validateJsonFormat(request)) {
                    string error = StreamUtils::createErrorResponse(400, "Invalid JSON format");
                    handler_ptr->writeMessage(error);
                    continue;
                }

                string response = processRequest(request, client_fd);
                if (!response.empty()) {
                    handler_ptr->writeMessage(response);
                }

                updateLastPingTime(client_fd);
            }
        } catch (const exception& e) {
            LOG_ERROR("Exception in client handler: " + string(e.what()));
        }

        cleanupClient(client_fd);
        LOG_INFO("Client handler finished for " + client_ip);
        close(client_fd);
    }

    void sendConnectionMessage(StreamHandler* handler) {
        string connection_msg = StreamUtils::createSuccessResponse(200, 
            "{\"message\":\"Connected to Millionaire Game Server\"}");
        handler->writeMessage(connection_msg);
    }

    void updateLastPingTime(int client_fd) {
        lock_guard<mutex> lock(clients_mutex_);
        auto it = active_clients_.find(client_fd);
        if (it != active_clients_.end()) {
            it->second.last_ping_time = time(nullptr);
        }
    }

    void cleanupClient(int client_fd) {
        lock_guard<mutex> lock(clients_mutex_);
        auto it = active_clients_.find(client_fd);
        if (it != active_clients_.end()) {
            string token = it->second.auth_token;
            string username = it->second.username;
            
            {
                lock_guard<mutex> token_lock(tokens_mutex_);
                if (!token.empty()) {
                    token_to_fd_.erase(token);
                }
                if (!username.empty()) {
                    username_to_token_.erase(username);
                    online_users_.erase(username);
                }
            }
            
            active_clients_.erase(it);
        }
    }

    string processRequest(const string& request, int client_fd) {
        string request_type = StreamUtils::extractRequestType(request);

        if (request_type.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing requestType");
        }

        lock_guard<mutex> lock(clients_mutex_);
        auto it = active_clients_.find(client_fd);
        if (it == active_clients_.end()) {
            return StreamUtils::createErrorResponse(500, "Client session not found");
        }

        ClientSession& session = it->second;

        // Authentication requests (no auth required)
        if (request_type == "LOGIN") {
            return handleLogin(request, session, client_fd);
        } else if (request_type == "REGISTER") {
            return handleRegister(request, session, client_fd);
        } else if (request_type == "CONNECTION") {
            return handleConnection(request, session);
        }
        
        // All other requests require authentication
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(402, "Not authenticated or invalid authToken");
        }

        // Game actions
        if (request_type == "START") {
            return handleStart(request, session);
        } else if (request_type == "ANSWER") {
            return handleAnswer(request, session);
        } else if (request_type == "LIFELINE") {
            return handleLifeline(request, session);
        } else if (request_type == "GIVE_UP") {
            return handleGiveUp(request, session);
        } else if (request_type == "RESUME") {
            return handleResume(request, session);
        } else if (request_type == "LEAVE_GAME") {
            return handleLeaveGame(request, session);
        }
        
        // Social features
        else if (request_type == "LEADERBOARD") {
            return handleLeaderboard(request, session);
        } else if (request_type == "FRIEND_STATUS") {
            return handleFriendStatus(request, session);
        } else if (request_type == "FRIEND_STATUS_INFO") {
            return handleFriendStatusInfo(request, session);
        } else if (request_type == "ADD_FRIEND") {
            return handleAddFriend(request, session);
        } else if (request_type == "ACCEPT_FRIEND") {
            return handleAcceptFriend(request, session);
        } else if (request_type == "DECLINE_FRIEND") {
            return handleDeclineFriend(request, session);
        } else if (request_type == "FRIEND_REQ_LIST") {
            return handleFriendReqList(request, session);
        } else if (request_type == "DEL_FRIEND") {
            return handleDelFriend(request, session);
        } else if (request_type == "CHAT") {
            return handleChat(request, session);
        }
        
        // User information
        else if (request_type == "USER_INFO") {
            return handleUserInfo(request, session);
        } else if (request_type == "VIEW_HISTORY") {
            return handleViewHistory(request, session);
        } else if (request_type == "CHANGE_PASS") {
            return handleChangePass(request, session);
        }
        
        // Connection management
        else if (request_type == "PING") {
            return handlePing(request, session);
        } else if (request_type == "LOGOUT") {
            return handleLogout(request, session, client_fd);
        }
        
        // Admin requests
        else if (request_type == "ADD_QUES") {
            return handleAddQues(request, session);
        } else if (request_type == "CHANGE_QUES") {
            return handleChangeQues(request, session);
        } else if (request_type == "VIEW_QUES") {
            return handleViewQues(request, session);
        } else if (request_type == "DEL_QUES") {
            return handleDelQues(request, session);
        } else if (request_type == "BAN_USER") {
            return handleBanUser(request, session);
        }
        
        // Unknown request type
        else {
            return StreamUtils::createErrorResponse(415, "Unknown request type: " + request_type);
        }
    }

    /**
     * Validate auth token from client request
     * Client MUST include "authToken" field in every request (except LOGIN/REGISTER/CONNECTION)
     * Token is validated by checking:
     * 1. Token exists in request JSON
     * 2. Token exists in token_to_fd_ map
     * 3. Token belongs to this client's socket fd
     * 4. Token matches session's stored token
     * Returns false if validation fails (caller should return 402 AUTH_ERROR)
     */
    bool requireAuth(const string& request, ClientSession& session, string& username) {
        string token_from_request = extractStringFromJson(request, "authToken");
        
        if (token_from_request.empty()) {
            return false;
        }
        
        lock_guard<mutex> token_lock(tokens_mutex_);
        auto token_it = token_to_fd_.find(token_from_request);
        if (token_it == token_to_fd_.end()) {
            return false;
        }
        
        int token_fd = token_it->second;
        if (token_fd != session.handler->getSocketFd()) {
            return false;
        }
        
        if (session.auth_token != token_from_request) {
            return false;
        }
        
        username = session.username;
        return true;
    }

    /**
     * Handle LOGIN request
     * Returns authToken that client MUST include in all subsequent requests
     * Also returns user role (user/admin)
     */
    string handleLogin(const string& request, ClientSession& session, int client_fd) {
        if (session.authenticated) {
            return StreamUtils::createErrorResponse(400, "Already authenticated");
        }

        string username = extractStringFromJson(request, "username");
        string password = extractStringFromJson(request, "password");

        if (username.empty() || password.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing username or password");
        }

        // TODO: Replace with database call when database is integrated
        // bool login_success = Database::getInstance().authenticateUser(username, password);
        // if (!login_success) {
        //     return StreamUtils::createErrorResponse(401, "Invalid credentials");
        // }
        // 
        // // Check if user is banned
        // User user = Database::getInstance().getUser(username);
        // if (user.is_banned) {
        //     return StreamUtils::createErrorResponse(403, "Account is banned");
        // }
        // 
        // string user_role = Database::getInstance().getUserRole(username);
        
        bool login_success = authenticateUser(username, password);
        if (!login_success) {
            return StreamUtils::createErrorResponse(401, "Invalid credentials");
        }

        // Get user role (placeholder - will be replaced with database call)
        string user_role = "user";
        if (isAdmin(username)) {
            user_role = "admin";
        }

        string token = generateAuthToken();
        session.auth_token = token;
        session.username = username;
        session.role = user_role;
        session.authenticated = true;

        {
            lock_guard<mutex> token_lock(tokens_mutex_);
            token_to_fd_[token] = client_fd;
            username_to_token_[username] = token;
            online_users_.insert(username);
        }

        string data = "{\"authToken\":\"" + token + "\",\"username\":\"" + username + 
                     "\",\"role\":\"" + user_role + "\",\"message\":\"Login successful\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle REGISTER request
     * According to PROTOCOL.md, returns 201 CREATED without authToken
     * User must LOGIN after registration to get authToken
     */
    string handleRegister(const string& request, ClientSession& session, int client_fd) {
        if (session.authenticated) {
            return StreamUtils::createErrorResponse(400, "Already authenticated");
        }

        string username = extractStringFromJson(request, "username");
        string password = extractStringFromJson(request, "password");

        if (username.empty() || password.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing username or password");
        }

        // Validate password strength
        if (!validatePasswordStrength(password)) {
            return StreamUtils::createErrorResponse(410, 
                "Password must be at least 8 characters and contain at least one uppercase letter, one lowercase letter, and one digit");
        }

        // TODO: Replace with database call when database is integrated
        // bool registered = Database::getInstance().registerUser(username, password);
        // if (!registered) {
        //     return StreamUtils::createErrorResponse(409, "Username already exists");
        // }
        
        bool registered = registerUser(username, password);
        if (!registered) {
            return StreamUtils::createErrorResponse(409, "Username already exists");
        }

        // According to PROTOCOL.md, REGISTER returns 201 without authToken
        // User must LOGIN to get authToken
        string data = "{\"username\":\"" + username + 
                     "\",\"message\":\"Registration successful. Please login to continue.\"}";
        return StreamUtils::createSuccessResponse(201, data);
    }

    /**
     * Handle START request
     * Starts a new game session
     * According to PROTOCOL.md:
     * - Returns error 405 if user already in game
     * - Returns error 412 if user has saved game and overrideSavedGame is false
     * - Sends GAME_START notification followed by QUESTION_INFO
     */
    string handleStart(const string& request, ClientSession& session) {
        string username = session.username;

        // Check if user already in active game
        if (session.in_game) {
            return StreamUtils::createErrorResponse(405, "Already in a game");
        }

        // Check for saved game
        bool override_saved = extractBoolFromJson(request, "overrideSavedGame", false);
        
        // TODO: Replace with database call when database is integrated
        // GameSession saved_game = Database::getInstance().getActiveGameSession(username);
        // if (saved_game.game_id > 0 && !override_saved) {
        //     return StreamUtils::createErrorResponse(412, 
        //         "You have a saved game. Use RESUME to continue or set overrideSavedGame=true to start new game");
        // }
        
        GameProgress saved_progress = loadGameProgress(username);
        if (saved_progress.level > 0 && !override_saved) {
            return StreamUtils::createErrorResponse(412, 
                "You have a saved game. Use RESUME to continue or set overrideSavedGame=true to start new game");
        }

        // Generate new game ID
        int game_id = generateGameId();
        
        // Initialize game state
        session.in_game = true;
        session.game_id = game_id;
        session.current_question_number = 1;
        session.current_level = 1;
        session.current_prize = 1000000;  // Starting prize
        session.total_score = 0;
        session.used_lifelines.clear();

        // TODO: Replace with database call when database is integrated
        // int game_id = Database::getInstance().createGameSession(username);
        // Question question = QuestionManager::getInstance().getRandomQuestion(1);

        // Send GAME_START notification
        // TODO: Send notification after game logic is integrated
        // string game_start_notification = createGameStartNotification(game_id);
        // session.handler->writeMessage(game_start_notification);

        // Send QUESTION_INFO notification
        // TODO: Send notification after game logic is integrated
        // string question_info = createQuestionInfoNotification(game_id, question);
        // session.handler->writeMessage(question_info);

        // Return success response (notifications will be sent separately)
        string data = "{\"message\":\"Game started\",\"gameId\":" + to_string(game_id) + 
                     ",\"timestamp\":" + to_string(time(nullptr)) + "}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle ANSWER request
     * Validates gameId, questionNumber, and answerIndex
     * Returns answer result with scoring information
     */
    string handleAnswer(const string& request, ClientSession& session) {
        string username = session.username;

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }

        // Extract required fields
        int game_id = extractIntFromJson(request, "gameId", -1);
        int question_number = extractIntFromJson(request, "questionNumber", -1);
        int answer_index = extractIntFromJson(request, "answerIndex", -1);

        // Validate gameId matches current game
        if (game_id != session.game_id) {
            return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
        }

        // Validate questionNumber matches current question
        if (question_number != session.current_question_number) {
            return StreamUtils::createErrorResponse(422, 
                "Question number mismatch: expected " + to_string(session.current_question_number) + 
                ", got " + to_string(question_number));
        }

        // Validate answerIndex (0-3)
        if (answer_index < 0 || answer_index > 3) {
            return StreamUtils::createErrorResponse(422, "Invalid answerIndex: must be 0-3");
        }

        // TODO: Check timeout - will be implemented with game timer
        // if (isQuestionTimeout(game_id, question_number)) {
        //     return handleQuestionTimeout(session);
        // }

        // TODO: Replace with game logic when integrated
        // bool correct = GameStateManager::getInstance().checkAnswer(game_id, question_number, answer_index);
        // int time_remaining = GameTimer::getInstance().getRemainingTime(game_id);
        // int lifelines_used = session.used_lifelines.size();
        // int points_earned = ScoringSystem::getInstance().calculateQuestionScore(time_remaining, lifelines_used);
        
        bool correct = checkAnswer(session.current_level, to_string(answer_index));
        int time_remaining = 15;  // Placeholder
        int lifelines_used = session.used_lifelines.size();
        int points_earned = max(0, time_remaining - (lifelines_used * 5));

        if (correct) {
            // Update score
            session.total_score += points_earned;
            session.current_question_number++;
            
            // Check if game won (answered all 15 questions)
            if (session.current_question_number > 15) {
                // Game won
                session.in_game = false;
                // TODO: Save to database and leaderboard
                // Database::getInstance().endGame(game_id, "won", session.total_score, 1000000000);
                
                string data = "{\"gameId\":" + to_string(game_id) + 
                             ",\"correct\":true" +
                             ",\"questionNumber\":15" +
                             ",\"timeRemaining\":" + to_string(time_remaining) +
                             ",\"pointsEarned\":" + to_string(points_earned) +
                             ",\"totalScore\":" + to_string(session.total_score) +
                             ",\"currentPrize\":1000000000" +
                             ",\"gameOver\":true" +
                             ",\"isWinner\":true}";
                
                // TODO: Send GAME_END notification
                return StreamUtils::createSuccessResponse(200, data);
            } else {
                // Continue to next question
                session.current_prize *= 2;
                // TODO: Send next QUESTION_INFO notification
                
                string data = "{\"gameId\":" + to_string(game_id) + 
                             ",\"correct\":true" +
                             ",\"questionNumber\":" + to_string(session.current_question_number - 1) +
                             ",\"timeRemaining\":" + to_string(time_remaining) +
                             ",\"pointsEarned\":" + to_string(points_earned) +
                             ",\"totalScore\":" + to_string(session.total_score) +
                             ",\"currentPrize\":" + to_string(session.current_prize) +
                             ",\"gameOver\":false" +
                             ",\"isWinner\":false}";
                return StreamUtils::createSuccessResponse(200, data);
            }
        } else {
            // Wrong answer - game ends
            session.in_game = false;
            
            // Calculate safe checkpoint
            int safe_checkpoint_prize = 0;
            int safe_checkpoint_score = 0;
            if (session.current_question_number > 15) {
                safe_checkpoint_prize = 1000000000;
                safe_checkpoint_score = session.total_score;
            } else if (session.current_question_number > 10) {
                safe_checkpoint_prize = 100000000;
                safe_checkpoint_score = session.total_score - points_earned;  // Approximate
            } else if (session.current_question_number > 5) {
                safe_checkpoint_prize = 10000000;
                safe_checkpoint_score = session.total_score - points_earned;  // Approximate
            }
            
            // TODO: Save to database
            // Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
            
            // TODO: Send GAME_END notification
            
            string data = "{\"gameId\":" + to_string(game_id) + 
                         ",\"correct\":false" +
                         ",\"questionNumber\":" + to_string(session.current_question_number) +
                         ",\"correctAnswer\":" + to_string(answer_index) +  // Placeholder
                         ",\"pointsEarned\":0" +
                         ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                         ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                         ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                         ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                         ",\"gameOver\":true" +
                         ",\"isWinner\":false}";
            return StreamUtils::createSuccessResponse(200, data);
        }
    }

    /**
     * Handle LIFELINE request
     * Validates gameId, questionNumber, and lifelineType
     * Returns LIFELINE_INFO notification
     */
    string handleLifeline(const string& request, ClientSession& session) {
        string username = session.username;

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }

        int game_id = extractIntFromJson(request, "gameId", -1);
        int question_number = extractIntFromJson(request, "questionNumber", -1);
        string lifeline_type = extractStringFromJson(request, "lifelineType");

        // Validate gameId
        if (game_id != session.game_id) {
            return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
        }

        // Validate questionNumber
        if (question_number != session.current_question_number) {
            return StreamUtils::createErrorResponse(422, 
                "Question number mismatch: expected " + to_string(session.current_question_number) + 
                ", got " + to_string(question_number));
        }

        // Validate lifelineType
        if (lifeline_type != "5050" && lifeline_type != "PHONE" && lifeline_type != "AUDIENCE") {
            return StreamUtils::createErrorResponse(422, "Invalid lifelineType");
        }

        // Check if lifeline already used
        if (session.used_lifelines.find(lifeline_type) != session.used_lifelines.end()) {
            return StreamUtils::createErrorResponse(407, "Lifeline already used");
        }

        // Mark lifeline as used
        session.used_lifelines.insert(lifeline_type);

        // TODO: Process lifeline with game logic
        // LifelineResult result = LifelineManager::getInstance().useLifeline(game_id, lifeline_type);
        // TODO: Send LIFELINE_INFO notification

        string data = "{\"message\":\"Lifeline processed\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle GIVE_UP request
     * Player quits and takes current winnings
     */
    string handleGiveUp(const string& request, ClientSession& session) {
        string username = session.username;

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }

        int game_id = extractIntFromJson(request, "gameId", -1);
        int question_number = extractIntFromJson(request, "questionNumber", -1);

        // Validate gameId
        if (game_id != session.game_id) {
            return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
        }

        // Validate questionNumber
        if (question_number != session.current_question_number) {
            return StreamUtils::createErrorResponse(422, 
                "Question number mismatch: expected " + to_string(session.current_question_number) + 
                ", got " + to_string(question_number));
        }

        int final_prize = session.current_prize;
        int final_question_number = session.current_question_number;
        int total_score = session.total_score;

        // End game
        session.in_game = false;

        // TODO: Save to database
        // Database::getInstance().endGame(game_id, "quit", total_score, final_prize);

        string data = "{\"finalPrize\":" + to_string(final_prize) + 
                     ",\"finalQuestionNumber\":" + to_string(final_question_number) + 
                     ",\"totalScore\":" + to_string(total_score) +
                     ",\"gameId\":" + to_string(game_id) +
                     ",\"message\":\"You gave up and took the prize.\"}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle RESUME request
     * Resumes a previously auto-saved game
     */
    string handleResume(const string& request, ClientSession& session) {
        string username = session.username;

        if (session.in_game) {
            return StreamUtils::createErrorResponse(405, "User already in a game");
        }

        // TODO: Replace with database call when database is integrated
        // GameSession saved_game = Database::getInstance().loadGameProgress(username);
        // if (saved_game.game_id == 0) {
        //     return StreamUtils::createErrorResponse(404, "No saved game found");
        // }
        
        GameProgress progress = loadGameProgress(username);
        if (progress.level == 0) {
            return StreamUtils::createErrorResponse(404, "No saved game found");
        }

        // Restore game state
        session.in_game = true;
        session.game_id = progress.level;  // Placeholder - will be replaced with actual game_id from database
        session.current_question_number = progress.level;
        session.current_prize = progress.prize;
        // TODO: Restore total_score, used_lifelines from database

        // TODO: Send QUESTION_INFO notification for resumed question

        string data = "{\"questionNumber\":" + to_string(progress.level) + 
                     ",\"prize\":" + to_string(progress.prize) + 
                     ",\"gameId\":" + to_string(session.game_id) +
                     ",\"totalScore\":" + to_string(session.total_score) +
                     ",\"message\":\"Game resumed successfully\"}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle LEAVE_GAME request
     * Leaves current game and auto-saves state
     */
    string handleLeaveGame(const string& request, ClientSession& session) {
        string username = session.username;

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }

        // Auto-save game state
        // TODO: Replace with database call when database is integrated
        // Database::getInstance().saveGameProgress(username, session.game_id, 
        //     session.current_question_number, session.current_prize, session.total_score);
        
        saveGameProgress(username, session.current_question_number, session.current_prize);

        // Mark as not in game (but game is saved)
        session.in_game = false;

        string data = "{\"message\":\"Left game successfully. Game state saved. Use RESUME to continue later.\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle LOGOUT request
     */
    string handleLogout(const string& request, ClientSession& session, int client_fd) {
        string username = session.username;
        
        // Cleanup session
        cleanupClient(client_fd);
        
        string data = "{\"message\":\"Logout successful\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle PING request
     */
    string handlePing(const string& request, ClientSession& session) {
        string data = "{\"message\":\"PONG\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle CONNECTION request
     */
    string handleConnection(const string& request, ClientSession& session) {
        string data = "{\"message\":\"Connection alive\",\"timestamp\":" + to_string(time(nullptr)) + "}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle LEADERBOARD request
     */
    string handleLeaderboard(const string& request, ClientSession& session) {
        string type = extractStringFromJson(request, "type");
        int page = extractIntFromJson(request, "page", 1);
        int limit = extractIntFromJson(request, "limit", 20);

        if (type != "global" && type != "friend") {
            return StreamUtils::createErrorResponse(422, "Invalid type: must be 'global' or 'friend'");
        }

        if (page < 1) {
            return StreamUtils::createErrorResponse(422, "Page number must be positive");
        }

        if (limit < 1) {
            return StreamUtils::createErrorResponse(422, "Limit must be positive");
        }

        // TODO: Replace with database call when database is integrated
        // vector<LeaderboardEntry> rankings = Database::getInstance().getLeaderboard(type, page, limit);
        
        // Placeholder response
        string data = "{\"rankings\":[],\"total\":0,\"page\":" + to_string(page) + 
                     ",\"limit\":" + to_string(limit) + "}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle ADD_FRIEND request
     */
    string handleAddFriend(const string& request, ClientSession& session) {
        string username = session.username;
        string friend_username = extractStringFromJson(request, "friendUsername");

        if (friend_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing friendUsername");
        }

        if (friend_username == username) {
            return StreamUtils::createErrorResponse(422, "Cannot add yourself as friend");
        }

        // TODO: Replace with database call when database is integrated
        // bool success = Database::getInstance().addFriendRequest(username, friend_username);
        // if (!success) {
        //     // Check specific error
        //     return StreamUtils::createErrorResponse(404, "Friend not found");
        //     // or return StreamUtils::createErrorResponse(409, "Friend request already sent or friend already exists");
        // }

        string data = "{\"message\":\"Friend request sent successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle ACCEPT_FRIEND request
     */
    string handleAcceptFriend(const string& request, ClientSession& session) {
        string username = session.username;
        string friend_username = extractStringFromJson(request, "friendUsername");

        if (friend_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing friendUsername");
        }

        // TODO: Replace with database call when database is integrated
        // bool success = Database::getInstance().acceptFriendRequest(friend_username, username);
        // if (!success) {
        //     return StreamUtils::createErrorResponse(404, "Friend request not found");
        // }

        string data = "{\"message\":\"Friend request accepted successfully\",\"friendUsername\":\"" + 
                     friend_username + "\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle DECLINE_FRIEND request
     */
    string handleDeclineFriend(const string& request, ClientSession& session) {
        string username = session.username;
        string friend_username = extractStringFromJson(request, "friendUsername");

        if (friend_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing friendUsername");
        }

        // TODO: Replace with database call when database is integrated
        // bool success = Database::getInstance().declineFriendRequest(friend_username, username);
        // if (!success) {
        //     return StreamUtils::createErrorResponse(404, "Friend request not found");
        // }

        string data = "{\"message\":\"Friend request declined successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle FRIEND_REQ_LIST request
     */
    string handleFriendReqList(const string& request, ClientSession& session) {
        string username = session.username;

        // TODO: Replace with database call when database is integrated
        // vector<FriendRequest> requests = Database::getInstance().getFriendRequests(username);

        // Placeholder response
        string data = "{\"friendRequests\":[]}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle DEL_FRIEND request
     */
    string handleDelFriend(const string& request, ClientSession& session) {
        string username = session.username;
        string friend_username = extractStringFromJson(request, "friendUsername");

        if (friend_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing friendUsername");
        }

        // TODO: Replace with database call when database is integrated
        // bool success = Database::getInstance().deleteFriend(username, friend_username);
        // if (!success) {
        //     return StreamUtils::createErrorResponse(404, "Friend not found");
        // }

        string data = "{\"message\":\"Friend removed successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle CHAT request
     */
    string handleChat(const string& request, ClientSession& session) {
        string username = session.username;
        string recipient = extractStringFromJson(request, "recipient");
        string message = extractStringFromJson(request, "message");

        if (recipient.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing recipient");
        }

        if (message.empty()) {
            return StreamUtils::createErrorResponse(422, "Invalid message format or empty message");
        }

        // TODO: Replace with database/user lookup when database is integrated
        // bool user_exists = Database::getInstance().userExists(recipient);
        // if (!user_exists) {
        //     return StreamUtils::createErrorResponse(404, "Recipient user not found");
        // }
        //
        // // Send message to recipient if online, or store for later
        // sendChatMessage(recipient, username, message);

        string data = "{\"message\":\"Message sent successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle USER_INFO request
     */
    string handleUserInfo(const string& request, ClientSession& session) {
        string target_username = extractStringFromJson(request, "username");

        if (target_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing username");
        }

        // TODO: Replace with database call when database is integrated
        // UserInfo info = Database::getInstance().getUserInfo(target_username);
        // if (info.username.empty()) {
        //     return StreamUtils::createErrorResponse(404, "User not found");
        // }

        // Placeholder response
        string data = "{\"username\":\"" + target_username + 
                     "\",\"totalGames\":0,\"highestPrize\":0,\"finalQuestionNumber\":0,\"totalScore\":0}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle VIEW_HISTORY request
     */
    string handleViewHistory(const string& request, ClientSession& session) {
        string username = session.username;

        // TODO: Replace with database call when database is integrated
        // vector<GameSession> games = Database::getInstance().getGameHistory(username, 20);

        // Placeholder response
        string data = "{\"games\":[]}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle CHANGE_PASS request
     */
    string handleChangePass(const string& request, ClientSession& session) {
        string username = session.username;
        string old_password = extractStringFromJson(request, "oldPassword");
        string new_password = extractStringFromJson(request, "newPassword");

        if (old_password.empty() || new_password.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing oldPassword or newPassword");
        }

        // Validate new password strength
        if (!validatePasswordStrength(new_password)) {
            return StreamUtils::createErrorResponse(410, 
                "Password must be at least 8 characters and contain at least one uppercase letter, one lowercase letter, and one digit");
        }

        // TODO: Replace with database call when database is integrated
        // bool success = Database::getInstance().changePassword(username, old_password, new_password);
        // if (!success) {
        //     return StreamUtils::createErrorResponse(401, "Wrong old password");
        // }

        string data = "{\"message\":\"Password changed successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle ADD_QUES request (Admin only)
     */
    string handleAddQues(const string& request, ClientSession& session) {
        string username = session.username;

        // Check admin role
        if (session.role != "admin") {
            return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
        }

        string question = extractStringFromJson(request, "question");
        int correct_answer = extractIntFromJson(request, "correctAnswer", -1);
        int level = extractIntFromJson(request, "level", -1);

        if (question.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing question");
        }

        if (correct_answer < 0 || correct_answer > 3) {
            return StreamUtils::createErrorResponse(422, "Invalid correctAnswer: must be 0-3");
        }

        if (level < 1 || level > 15) {
            return StreamUtils::createErrorResponse(422, "Invalid level: must be 1-15");
        }

        // TODO: Extract options array and validate
        // TODO: Replace with database call when database is integrated
        // int question_id = Database::getInstance().addQuestion(question_data);

        // Placeholder response
        string data = "{\"questionId\":0,\"message\":\"Question added successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle CHANGE_QUES request (Admin only)
     */
    string handleChangeQues(const string& request, ClientSession& session) {
        string username = session.username;

        if (session.role != "admin") {
            return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
        }

        int question_id = extractIntFromJson(request, "questionId", -1);

        if (question_id < 0) {
            return StreamUtils::createErrorResponse(400, "Missing questionId");
        }

        // TODO: Replace with database call when database is integrated
        // bool exists = Database::getInstance().questionExists(question_id);
        // if (!exists) {
        //     return StreamUtils::createErrorResponse(404, "Question not found");
        // }
        // Database::getInstance().updateQuestion(question_id, question_data);

        string data = "{\"message\":\"Question updated successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle VIEW_QUES request (Admin only)
     */
    string handleViewQues(const string& request, ClientSession& session) {
        string username = session.username;

        if (session.role != "admin") {
            return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
        }

        int page = extractIntFromJson(request, "page", 1);
        int limit = extractIntFromJson(request, "limit", 20);
        int level = extractIntFromJson(request, "level", -1);

        if (page < 1) {
            return StreamUtils::createErrorResponse(422, "Page number must be positive");
        }

        if (limit < 1) {
            return StreamUtils::createErrorResponse(422, "Limit must be positive");
        }

        if (level != -1 && (level < 1 || level > 15)) {
            return StreamUtils::createErrorResponse(422, "Invalid level: must be 1-15");
        }

        // TODO: Replace with database call when database is integrated
        // vector<Question> questions = Database::getInstance().getQuestions(level, page, limit);

        // Placeholder response
        string data = "{\"questions\":[],\"total\":0,\"page\":" + to_string(page) + "}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle DEL_QUES request (Admin only)
     */
    string handleDelQues(const string& request, ClientSession& session) {
        string username = session.username;

        if (session.role != "admin") {
            return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
        }

        int question_id = extractIntFromJson(request, "questionId", -1);

        if (question_id < 0) {
            return StreamUtils::createErrorResponse(400, "Missing questionId");
        }

        // TODO: Replace with database call when database is integrated
        // bool exists = Database::getInstance().questionExists(question_id);
        // if (!exists) {
        //     return StreamUtils::createErrorResponse(404, "Question not found");
        // }
        // Database::getInstance().deleteQuestion(question_id);

        string data = "{\"message\":\"Question deleted successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle BAN_USER request (Admin only)
     */
    string handleBanUser(const string& request, ClientSession& session) {
        string username = session.username;
        string target_username = extractStringFromJson(request, "username");
        string reason = extractStringFromJson(request, "reason");

        if (session.role != "admin") {
            return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
        }

        if (target_username.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing username");
        }

        if (reason.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing reason");
        }

        if (target_username == username) {
            return StreamUtils::createErrorResponse(422, "Cannot ban yourself");
        }

        // TODO: Replace with database call when database is integrated
        // bool exists = Database::getInstance().userExists(target_username);
        // if (!exists) {
        //     return StreamUtils::createErrorResponse(404, "User not found");
        // }
        // Database::getInstance().banUser(target_username, reason);

        string data = "{\"message\":\"User banned successfully\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    /**
     * Handle FRIEND_STATUS request
     * Server automatically retrieves friends list from database for authenticated user
     * Returns status (online/offline/ingame) for all friends
     */
    string handleFriendStatus(const string& request, ClientSession& session) {
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(401, "Not authenticated or invalid authToken");
        }

        vector<string> friend_list = getFriendsList(username);
        vector<FriendStatus> statuses;

        {
            lock_guard<mutex> token_lock(tokens_mutex_);
            for (const string& friend_name : friend_list) {
                FriendStatus status;
                status.username = friend_name;
                
                bool is_online = online_users_.find(friend_name) != online_users_.end();
                if (is_online) {
                    auto token_it = username_to_token_.find(friend_name);
                    if (token_it != username_to_token_.end()) {
                        auto fd_it = token_to_fd_.find(token_it->second);
                        if (fd_it != token_to_fd_.end()) {
                            lock_guard<mutex> client_lock(clients_mutex_);
                            auto client_it = active_clients_.find(fd_it->second);
                            if (client_it != active_clients_.end()) {
                                status.status = client_it->second.in_game ? "ingame" : "online";
                            } else {
                                status.status = "offline";
                            }
                        } else {
                            status.status = "offline";
                        }
                    } else {
                        status.status = "offline";
                    }
                } else {
                    status.status = "offline";
                }
                
                statuses.push_back(status);
            }
        }

        string status_json = buildFriendStatusJson(statuses);
        string data = "{\"friends\":" + status_json + "}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleFriendStatusInfo(const string& request, ClientSession& session) {
        return handleFriendStatus(request, session);
    }

    struct FriendStatus {
        string username;
        string status;
    };

    struct GameProgress {
        int level;
        int prize;
        
        GameProgress() : level(0), prize(0) {}
    };

    /**
     * Extract string value from JSON
     * Looks for "key":"value" pattern in JSON string
     */
    string extractStringFromJson(const string& json, const string& key) {
        string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == string::npos) return "";
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length() || json[pos] != '"') return "";
        pos++;
        
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        
        return json.substr(pos, end - pos);
    }

    /**
     * Extract integer value from JSON
     * Looks for "key":value pattern in JSON string
     */
    int extractIntFromJson(const string& json, const string& key, int default_value = 0) {
        string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == string::npos) return default_value;
        
        pos = json.find(':', pos);
        if (pos == string::npos) return default_value;
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length()) return default_value;
        
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
            end++;
        }
        
        if (end == pos) return default_value;
        
        try {
            return stoi(json.substr(pos, end - pos));
        } catch (...) {
            return default_value;
        }
    }

    /**
     * Extract boolean value from JSON
     * Looks for "key":true/false pattern in JSON string
     */
    bool extractBoolFromJson(const string& json, const string& key, bool default_value = false) {
        string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == string::npos) return default_value;
        
        pos = json.find(':', pos);
        if (pos == string::npos) return default_value;
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length()) return default_value;
        
        if (json.substr(pos, 4) == "true") {
            return true;
        } else if (json.substr(pos, 5) == "false") {
            return false;
        }
        
        return default_value;
    }

    /**
     * Validate password strength according to ERROR_CODES.md
     * Requirements:
     * - Minimum 8 characters
     * - At least one uppercase letter
     * - At least one lowercase letter
     * - At least one digit
     */
    bool validatePasswordStrength(const string& password) {
        if (password.length() < 8) {
            return false;
        }
        
        bool has_upper = false;
        bool has_lower = false;
        bool has_digit = false;
        
        for (char c : password) {
            if (isupper(c)) has_upper = true;
            if (islower(c)) has_lower = true;
            if (isdigit(c)) has_digit = true;
        }
        
        return has_upper && has_lower && has_digit;
    }

    /**
     * Generate unique game ID
     */
    int generateGameId() {
        static atomic<int> game_id_counter(1);
        return game_id_counter.fetch_add(1);
    }

    /**
     * Check if user has admin role
     * TODO: Replace with database call when database is integrated
     */
    bool isAdmin(const string& username) {
        // Placeholder - will be replaced with database check
        // return Database::getInstance().isAdmin(username);
        return false;
    }

    vector<string> getFriendsList(const string& username) {
        vector<string> friends;
        return getFriendsListFromDatabase(username);
    }

    string buildFriendStatusJson(const vector<FriendStatus>& statuses) {
        stringstream ss;
        ss << "[";
        for (size_t i = 0; i < statuses.size(); i++) {
            if (i > 0) ss << ",";
            ss << "{\"username\":\"" << statuses[i].username << "\","
               << "\"status\":\"" << statuses[i].status << "\"}";
        }
        ss << "]";
        return ss.str();
    }

    bool authenticateUser(const string& username, const string& password) {
        return true;
    }

    bool registerUser(const string& username, const string& password) {
        lock_guard<mutex> token_lock(tokens_mutex_);
        if (username_to_token_.find(username) != username_to_token_.end()) {
            return false;
        }
        return true;
    }

    bool checkAnswer(int level, const string& answer) {
        return true;
    }

    void saveGameProgress(const string& username, int level, int prize) {
    }

    GameProgress loadGameProgress(const string& username) {
        GameProgress progress;
        return progress;
    }

    vector<string> getFriendsListFromDatabase(const string& username) {
        vector<string> friends;
        return friends;
    }
};

GameServer* GameServer::instance_ = nullptr;

void printUsage(const char* program_name) {
    cout << "Usage: " << program_name << " [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -c <config_file>  Configuration file path (default: config.json)" << endl;
    cout << "  -p <port>         Server port (overrides config file)" << endl;
    cout << "  -l <log_file>     Log file path (overrides config file)" << endl;
    cout << "  -h                Show this help message" << endl;
}

int main(int argc, char* argv[]) {
    string config_file = "config.json";
    ServerConfig config;
    
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-c" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (string(argv[i]) == "-p" && i + 1 < argc) {
            config.port = stoi(argv[++i]);
        } else if (string(argv[i]) == "-l" && i + 1 < argc) {
            config.log_file = argv[++i];
        } else if (string(argv[i]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (config.port == 0) {
        if (!ConfigLoader::loadFromFile(config_file, config)) {
            cerr << "Warning: Failed to load config file: " << config_file << endl;
            cerr << "Using default configuration" << endl;
        }
    }

    GameServer server(config);

    if (!server.start()) {
        cerr << "Failed to start server" << endl;
        return 1;
    }

    server.run();

    return 0;
}
