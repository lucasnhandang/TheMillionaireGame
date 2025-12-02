#include "auth_manager.h"
#include "session_manager.h"
#include "json_utils.h"
#include "stream_handler.h"
#include "../database/database.h"
#include <iomanip>

using namespace std;

namespace MillionaireGame {

AuthManager& AuthManager::getInstance() {
    static AuthManager instance;
    return instance;
}

string AuthManager::generateToken() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, 15);
    
    stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << hex << dis(gen);
    }
    return ss.str();
}

void AuthManager::registerToken(const string& token, int client_fd, const string& username) {
    lock_guard<mutex> lock(tokens_mutex_);
    token_to_fd_[token] = client_fd;
    username_to_token_[username] = token;
}

void AuthManager::unregisterToken(const string& token, const string& username) {
    lock_guard<mutex> lock(tokens_mutex_);
    if (!token.empty()) {
        token_to_fd_.erase(token);
    }
    if (!username.empty()) {
        username_to_token_.erase(username);
    }
}

bool AuthManager::validateToken(const string& token, int client_fd) {
    lock_guard<mutex> lock(tokens_mutex_);
    auto it = token_to_fd_.find(token);
    if (it != token_to_fd_.end()) {
        return it->second == client_fd;
    }
    return false;
}

string AuthManager::requireAuth(const string& request, ClientSession& session) {
    string token_from_request = JsonUtils::extractString(request, "authToken");
    
    if (token_from_request.empty()) {
        return "";
    }
    
    if (!validateToken(token_from_request, session.handler->getSocketFd())) {
        return "";
    }
    
    if (session.auth_token != token_from_request) {
        return "";
    }
    
    return session.username;
}

bool AuthManager::validatePasswordStrength(const string& password) {
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

bool AuthManager::isAdmin(const string& username) {
    return Database::getInstance().getUserRole(username) == "admin";
}

string AuthManager::getUsernameFromToken(const string& token) {
    lock_guard<mutex> lock(tokens_mutex_);
    // Reverse lookup: token -> fd -> would need session manager
    // For now, return empty (will be handled by session manager)
    return "";
}

} // namespace MillionaireGame

