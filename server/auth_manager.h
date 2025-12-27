#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <random>
#include <sstream>
#include <cctype>

namespace MillionaireGame {

// Forward declaration
struct ClientSession;

/**
 * Authentication and Authorization Manager
 * Handles token generation, validation, and password checking
 */
class AuthManager {
public:
    static AuthManager& getInstance();
    
    /**
     * Generate unique 32-character hex authentication token
     */
    std::string generateToken();
    
    /**
     * Register token for a client connection
     */
    void registerToken(const std::string& token, int client_fd, const std::string& username);
    
    /**
     * Unregister token when client disconnects
     */
    void unregisterToken(const std::string& token, const std::string& username);
    
    /**
     * Validate authentication token from request
     * Returns true if token is valid and belongs to the client
     */
    bool validateToken(const std::string& token, int client_fd);
    
    /**
     * Check if request requires authentication and validate it
     * Returns username if valid, empty string otherwise
     */
    std::string requireAuth(const std::string& request, ClientSession& session);
    
    /**
     * Validate password strength
     * Requirements: 8+ chars, uppercase, lowercase, digit
     */
    bool validatePasswordStrength(const std::string& password);
    
    /**
     * Check if user has admin role
     * TODO: Replace with database call
     */
    bool isAdmin(const std::string& username);
    
    /**
     * Get username from token
     */
    std::string getUsernameFromToken(const std::string& token);

private:
    AuthManager() = default;
    ~AuthManager() = default;
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;
    
    std::mutex tokens_mutex_;
    std::unordered_map<std::string, int> token_to_fd_;
    std::unordered_map<std::string, std::string> username_to_token_;
};

} // namespace MillionaireGame

#endif // AUTH_MANAGER_H

