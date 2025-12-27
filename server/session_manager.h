#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "stream_handler.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <set>
#include <ctime>

namespace MillionaireGame {

/**
 * Client session structure
 * Stores all information about a connected client
 */
struct ClientSession {
    std::unique_ptr<StreamHandler> handler;
    std::string client_ip;
    time_t connected_time;
    time_t last_ping_time;
    std::string auth_token;
    std::string username;
    std::string role;  // "user" or "admin"
    bool authenticated;
    bool in_game;
    int game_id;  // Current game session ID
    int current_question_number;  // Current question number (1-15)
    int current_level;
    int current_prize;
    int total_score;
    std::set<std::string> used_lifelines;  // Track which lifelines have been used

    ClientSession(std::unique_ptr<StreamHandler> h, const std::string& ip);
};

/**
 * Session Manager
 * Manages all client sessions
 */
class SessionManager {
public:
    static SessionManager& getInstance();
    
    /**
     * Create new session for a client
     */
    void createSession(int client_fd, std::unique_ptr<StreamHandler> handler, const std::string& client_ip);
    
    /**
     * Get session by client file descriptor
     * Returns nullptr if not found
     */
    ClientSession* getSession(int client_fd);
    
    /**
     * Remove session when client disconnects
     */
    void removeSession(int client_fd);
    
    /**
     * Update last ping time for a session
     */
    void updatePingTime(int client_fd);
    
    /**
     * Check if user is online
     */
    bool isUserOnline(const std::string& username);
    
    /**
     * Add user to online list
     */
    void addOnlineUser(const std::string& username);
    
    /**
     * Remove user from online list
     */
    void removeOnlineUser(const std::string& username);
    
    /**
     * Get all active client file descriptors
     */
    std::vector<int> getAllClientFds();
    
    /**
     * Get number of active clients
     */
    size_t getClientCount();
    
    /**
     * Wait for all clients to disconnect
     */
    void waitForClientsToFinish();

private:
    SessionManager() = default;
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    std::mutex clients_mutex_;
    std::unordered_map<int, ClientSession> active_clients_;
    std::set<std::string> online_users_;
};

} // namespace MillionaireGame

#endif // SESSION_MANAGER_H

