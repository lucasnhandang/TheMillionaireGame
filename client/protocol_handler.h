#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include "socket_client.h"

// Forward declarations for nlohmann/json (or use a simple JSON library)
// For simplicity, we'll use string-based JSON manipulation
// In production, use nlohmann/json or similar

/**
 * ProtocolHandler - Handles JSON protocol communication with game server
 * Builds requests and parses responses according to PROTOCOL.md
 */
class ProtocolHandler {
public:
    ProtocolHandler(SocketClient* socket);
    
    // Authentication
    bool login(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    bool logout();
    
    // Game actions
    bool startGame(bool overrideSavedGame = false);
    bool answerQuestion(int gameId, int questionNumber, int answerIndex);
    bool useLifeline(int gameId, int questionNumber, const std::string& lifelineType);
    bool giveUp(int gameId, int questionNumber);
    bool resumeGame();
    bool leaveGame();
    
    // Social features
    bool getLeaderboard(const std::string& type, int page = 1, int limit = 20);
    bool getFriendStatus();
    bool addFriend(const std::string& username);
    bool acceptFriend(const std::string& username);
    bool declineFriend(const std::string& username);
    bool getFriendRequestList();
    bool deleteFriend(const std::string& username);
    bool sendChat(const std::string& recipient, const std::string& message);
    
    // User features
    bool getUserInfo(const std::string& username);
    bool getGameHistory();
    bool changePassword(const std::string& oldPassword, const std::string& newPassword);
    
    // Connection
    bool ping();
    
    // Admin features (if role is admin)
    bool addQuestion(const std::string& question, const std::vector<std::pair<std::string, std::string>>& options, 
                     int correctAnswer, int level);
    bool changeQuestion(int questionId, const std::string& question, 
                       const std::vector<std::pair<std::string, std::string>>& options, 
                       int correctAnswer);
    bool viewQuestions(int page = 1, int limit = 20, int level = 0);
    bool deleteQuestion(int questionId);
    bool banUser(const std::string& username, const std::string& reason);
    
    // Response handling
    void setResponseCallback(std::function<void(int responseCode, const std::string& jsonData)> callback);
    void handleServerMessage(const std::string& jsonMessage);
    
    // State
    void setAuthToken(const std::string& token);
    std::string getAuthToken() const;
    void setUsername(const std::string& username);
    std::string getUsername() const;
    void setRole(const std::string& role);
    std::string getRole() const;
    bool isAdmin() const;

private:
    SocketClient* socket_;
    std::string auth_token_;
    std::string username_;
    std::string role_;
    std::function<void(int responseCode, const std::string& jsonData)> response_callback_;
    
    // Helper methods
    std::string buildRequest(const std::string& requestType, const std::string& data);
    std::string buildDataWithAuth(const std::string& additionalData = "");
    bool sendRequest(const std::string& requestType, const std::string& data);
    void parseResponse(const std::string& jsonResponse);
};

#endif // PROTOCOL_HANDLER_H

