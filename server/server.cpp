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
#include <iomanip>
#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
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

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
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
        bool authenticated;
        bool in_game;
        int current_level;
        int current_prize;

        ClientSession(unique_ptr<StreamHandler> h, const string& ip)
            : handler(move(h)), client_ip(ip), connected_time(time(nullptr)),
              last_ping_time(time(nullptr)), authenticated(false), 
              in_game(false), current_level(0), current_prize(0) {}
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

        if (request_type == "LOGIN") {
            return handleLogin(request, session, client_fd);
        } else if (request_type == "REGISTER") {
            return handleRegister(request, session, client_fd);
        } else if (request_type == "START_GAME") {
            return handleStartGame(request, session);
        } else if (request_type == "ANSWER") {
            return handleAnswer(request, session);
        } else if (request_type == "GIVE_UP") {
            return handleGiveUp(request, session);
        } else if (request_type == "RESUME") {
            return handleResume(request, session);
        } else if (request_type == "PING") {
            return handlePing(request, session);
        } else if (request_type == "CONNECTION") {
            return handleConnection(request, session);
        } else if (request_type == "FRIEND_STATUS") {
            return handleFriendStatus(request, session);
        } else if (request_type == "FRIEND_STATUS_INFO") {
            return handleFriendStatusInfo(request, session);
        } else {
            return StreamUtils::createErrorResponse(404, "Unknown request type: " + request_type);
        }
    }

    /**
     * Validate auth token from client request
     * Client MUST include "authToken" field in every request (except LOGIN/REGISTER)
     * Token is validated by checking:
     * 1. Token exists in token_to_fd_ map
     * 2. Token belongs to this client's socket fd
     * 3. Token matches session's stored token
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

        bool login_success = authenticateUser(username, password);
        if (!login_success) {
            return StreamUtils::createErrorResponse(401, "Invalid credentials");
        }

        string token = generateAuthToken();
        session.auth_token = token;
        session.username = username;
        session.authenticated = true;

        {
            lock_guard<mutex> token_lock(tokens_mutex_);
            token_to_fd_[token] = client_fd;
            username_to_token_[username] = token;
            online_users_.insert(username);
        }

        string data = "{\"authToken\":\"" + token + "\",\"username\":\"" + username + 
                     "\",\"message\":\"Login successful\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleRegister(const string& request, ClientSession& session, int client_fd) {
        if (session.authenticated) {
            return StreamUtils::createErrorResponse(400, "Already authenticated");
        }

        string username = extractStringFromJson(request, "username");
        string password = extractStringFromJson(request, "password");

        if (username.empty() || password.empty()) {
            return StreamUtils::createErrorResponse(400, "Missing username or password");
        }

        bool registered = registerUser(username, password);
        if (!registered) {
            return StreamUtils::createErrorResponse(409, "Username already exists");
        }

        string token = generateAuthToken();
        session.auth_token = token;
        session.username = username;
        session.authenticated = true;

        {
            lock_guard<mutex> token_lock(tokens_mutex_);
            token_to_fd_[token] = client_fd;
            username_to_token_[username] = token;
            online_users_.insert(username);
        }

        string data = "{\"authToken\":\"" + token + "\",\"username\":\"" + username + 
                     "\",\"message\":\"Registration successful\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleStartGame(const string& request, ClientSession& session) {
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(401, "Not authenticated or invalid authToken");
        }

        if (session.in_game) {
            return StreamUtils::createErrorResponse(400, "Already in a game");
        }

        session.in_game = true;
        session.current_level = 1;
        session.current_prize = 1000000;

        string question = "{\"id\":1,\"text\":\"What is the capital of Vietnam?\","
                         "\"options\":["
                         "{\"label\":\"A\",\"text\":\"Hanoi\"},"
                         "{\"label\":\"B\",\"text\":\"Ho Chi Minh City\"},"
                         "{\"label\":\"C\",\"text\":\"Da Nang\"},"
                         "{\"label\":\"D\",\"text\":\"Hue\"}"
                         "]}";

        string data = "{\"question\":" + question + ",\"level\":" + to_string(session.current_level) + 
                     ",\"prize\":" + to_string(session.current_prize) + 
                     ",\"lifelines\":[\"5050\",\"PHONE\",\"AUDIENCE\"]}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleAnswer(const string& request, ClientSession& session) {
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(401, "Not authenticated or invalid authToken");
        }

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(400, "Not in a game");
        }

        string answer = extractStringFromJson(request, "answer");
        bool correct = checkAnswer(session.current_level, answer);

        if (correct) {
            session.current_level++;
            session.current_prize *= 2;
        } else {
            session.in_game = false;
        }

        string data = "{\"correct\":" + string(correct ? "true" : "false") + 
                     ",\"currentPrize\":" + to_string(session.current_prize) + 
                     ",\"level\":" + to_string(session.current_level) + 
                     ",\"gameOver\":" + string(correct ? "false" : "true") + "}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleGiveUp(const string& request, ClientSession& session) {
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(401, "Not authenticated or invalid authToken");
        }

        if (!session.in_game) {
            return StreamUtils::createErrorResponse(400, "Not in a game");
        }

        int final_prize = session.current_prize;
        int final_level = session.current_level;

        saveGameProgress(session.username, final_level, final_prize);

        session.in_game = false;

        string data = "{\"finalPrize\":" + to_string(final_prize) + 
                     ",\"finalLevel\":" + to_string(final_level) + 
                     ",\"message\":\"Game saved successfully\"}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleResume(const string& request, ClientSession& session) {
        string username;
        if (!requireAuth(request, session, username)) {
            return StreamUtils::createErrorResponse(401, "Not authenticated or invalid authToken");
        }

        if (session.in_game) {
            return StreamUtils::createErrorResponse(400, "Already in a game");
        }

        GameProgress progress = loadGameProgress(session.username);
        if (progress.level == 0) {
            return StreamUtils::createErrorResponse(404, "No saved game found");
        }

        session.in_game = true;
        session.current_level = progress.level;
        session.current_prize = progress.prize;

        string data = "{\"level\":" + to_string(progress.level) + 
                     ",\"prize\":" + to_string(progress.prize) + 
                     ",\"message\":\"Game resumed successfully\"}";

        return StreamUtils::createSuccessResponse(200, data);
    }

    string handlePing(const string& request, ClientSession& session) {
        string data = "{\"message\":\"PONG\"}";
        return StreamUtils::createSuccessResponse(200, data);
    }

    string handleConnection(const string& request, ClientSession& session) {
        string data = "{\"message\":\"Connection alive\",\"timestamp\":" + to_string(time(nullptr)) + "}";
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
