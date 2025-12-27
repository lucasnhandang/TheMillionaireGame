#include "auth_handlers.h"
#include "../session_manager.h"
#include "../auth_manager.h"
#include "../logger.h"
#include <ctime>

using namespace std;

namespace MillionaireGame {

namespace AuthHandlers {

string handleLogin(const string& request, ClientSession& session, int client_fd) {
    if (session.authenticated) {
        return StreamUtils::createErrorResponse(400, "Already authenticated");
    }

    string username = JsonUtils::extractString(request, "username");
    string password = JsonUtils::extractString(request, "password");

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
    
    // Placeholder authentication
    bool login_success = true;  // Will be replaced with database call
    if (!login_success) {
        return StreamUtils::createErrorResponse(401, "Invalid credentials");
    }

    // Get user role (placeholder - will be replaced with database call)
    string user_role = "user";
    if (AuthManager::getInstance().isAdmin(username)) {
        user_role = "admin";
    }

    string token = AuthManager::getInstance().generateToken();
    session.auth_token = token;
    session.username = username;
    session.role = user_role;
    session.authenticated = true;

    AuthManager::getInstance().registerToken(token, client_fd, username);
    SessionManager::getInstance().addOnlineUser(username);

    string data = "{\"authToken\":\"" + token + "\",\"username\":\"" + username + 
                 "\",\"role\":\"" + user_role + "\",\"message\":\"Login successful\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleRegister(const string& request, ClientSession& session, int client_fd) {
    if (session.authenticated) {
        return StreamUtils::createErrorResponse(400, "Already authenticated");
    }

    string username = JsonUtils::extractString(request, "username");
    string password = JsonUtils::extractString(request, "password");

    if (username.empty() || password.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username or password");
    }

    // Validate password strength
    if (!AuthManager::getInstance().validatePasswordStrength(password)) {
        return StreamUtils::createErrorResponse(410, 
            "Password must be at least 8 characters and contain at least one uppercase letter, one lowercase letter, and one digit");
    }

    // TODO: Replace with database call when database is integrated
    // bool registered = Database::getInstance().registerUser(username, password);
    // if (!registered) {
    //     return StreamUtils::createErrorResponse(409, "Username already exists");
    // }
    
    // Placeholder registration check
    bool registered = true;  // Will be replaced with database call
    if (!registered) {
        return StreamUtils::createErrorResponse(409, "Username already exists");
    }

    // According to PROTOCOL.md, REGISTER returns 201 without authToken
    // User must LOGIN to get authToken
    string data = "{\"username\":\"" + username + 
                 "\",\"message\":\"Registration successful. Please login to continue.\"}";
    return StreamUtils::createSuccessResponse(201, data);
}

string handleLogout(const string& request, ClientSession& session, int client_fd) {
    string username = session.username;
    
    // Cleanup authentication
    if (!session.auth_token.empty()) {
        AuthManager::getInstance().unregisterToken(session.auth_token, username);
    }
    SessionManager::getInstance().removeOnlineUser(username);
    SessionManager::getInstance().removeSession(client_fd);
    
    string data = "{\"message\":\"Logout successful\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace AuthHandlers

} // namespace MillionaireGame

