#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "socket_client.h"
#include "json_utils.h"
#include <string>
#include <map>
#include <vector>
#include <functional>

class GameEventQueue;

class ProtocolHandler {
public:
    ProtocolHandler(SocketClient* client);
    ~ProtocolHandler();
    
    // Authentication
    struct LoginResponse {
        int responseCode;
        std::string authToken;
        std::string username;
        std::string role;
        std::string message;
    };
    
    LoginResponse login(const std::string& username, const std::string& password);
    int registerUser(const std::string& username, const std::string& password);
    bool logout();
    
    // Game actions
    int startGame(bool overrideSavedGame = false);
    int resumeGame();
    struct AnswerResponse {
        int responseCode;
        bool correct;
        int questionNumber;
        int timeRemaining;
        int pointsEarned;
        int totalScore;
        int currentPrize;
        bool gameOver;
        bool isWinner;
        int correctAnswer;
        int finalPrize;
    };
    
    AnswerResponse answerQuestion(int answerIndex);
    int useLifeline(const std::string& lifelineType);
    int giveUp();
    int leaveGame();
    
    // Connection
    int sendConnection();
    int sendPing();
    
    // Social features
    struct LeaderboardEntry {
        std::string username;
        int finalQuestionNumber;
        int totalScore;
        int rank;
        bool isWinner;
    };
    struct LeaderboardResponse {
        int responseCode;
        std::vector<LeaderboardEntry> rankings;
        int total;
        int page;
        int limit;
    };
    LeaderboardResponse getLeaderboard(const std::string& type = "global", int page = 1, int limit = 20);
    
    struct FriendStatus {
        std::string username;
        std::string status;
    };
    struct FriendStatusResponse {
        int responseCode;
        std::vector<FriendStatus> friends;
    };
    FriendStatusResponse getFriendStatus();
    
    struct FindFriendResponse {
        int responseCode;
        std::string username;
        std::string status;
    };
    FindFriendResponse findFriend(const std::string& username);
    
    int addFriend(const std::string& friendUsername);
    int acceptFriend(const std::string& friendUsername);
    int declineFriend(const std::string& friendUsername);
    
    struct FriendRequest {
        std::string username;
        long long sentAt;
    };
    struct FriendReqListResponse {
        int responseCode;
        std::vector<FriendRequest> friendRequests;
    };
    FriendReqListResponse getFriendReqList();
    
    int deleteFriend(const std::string& friendUsername);
    int sendChat(const std::string& recipient, const std::string& message);
    
    struct ChatMessage {
        std::string from;
        std::string to;
        std::string content;
        long long timestamp;
    };
    struct GetMessagesResponse {
        int responseCode;
        std::vector<ChatMessage> messages;
        int page;
        int limit;
    };
    GetMessagesResponse getMessages(const std::string& friendUsername, int page = 1, int limit = 50);
    
    // User information
    struct UserInfo {
        int responseCode;
        std::string username;
        int totalGames;
        long long highestPrize;
        int finalQuestionNumber;
        int totalScore;
    };
    UserInfo getUserInfo(const std::string& username);
    
    struct GameHistory {
        int gameId;
        std::string date;
        int finalQuestionNumber;
        int totalScore;
        long long finalPrize;
        std::string status;
    };
    struct ViewHistoryResponse {
        int responseCode;
        std::vector<GameHistory> games;
    };
    ViewHistoryResponse viewHistory();
    
    int changePassword(const std::string& oldPassword, const std::string& newPassword);
    
    // Admin functions
    struct AddQuestionRequest {
        std::string question;
        std::vector<std::string> options;
        int correctAnswer;
        int level;
        std::string lifeline_5050_info;
        std::string lifeline_ask_info;
        std::string lifeline_call_info;
    };
    struct AddQuestionResponse {
        int responseCode;
        int questionId;
        std::string message;
    };
    AddQuestionResponse addQuestion(const AddQuestionRequest& req);
    
    int changeQuestion(int questionId, const std::string& question, 
                      const std::vector<std::string>& options, int correctAnswer);
    
    struct QuestionInfo {
        int questionId;
        std::string question;
        int level;
    };
    struct ViewQuestionsResponse {
        int responseCode;
        std::vector<QuestionInfo> questions;
        int total;
        int page;
    };
    ViewQuestionsResponse viewQuestions(int page = 1, int limit = 20, int level = -1);
    
    int deleteQuestion(int questionId);
    int banUser(const std::string& username, const std::string& reason);
    
    struct QuestionDetail {
        int responseCode;
        int questionId;
        std::string question;
        std::vector<std::string> options;
        int correctAnswer;
        int level;
        std::string lifeline_5050_info;
        std::string lifeline_ask_info;
        std::string lifeline_call_info;
    };
    QuestionDetail getQuestionDetail(int questionId);
    
    // User management
    struct UserListEntry {
        std::string username;
        std::string role;
        bool isBanned;
        int totalGames;
        long long highestPrize;
    };
    struct ViewUsersResponse {
        int responseCode;
        std::vector<UserListEntry> users;
        int total;
        int page;
        int limit;
    };
    ViewUsersResponse viewUsers(int page = 1, int limit = 10);
    
    int promoteUser(const std::string& username);
    int revokeAdmin(const std::string& username);
    
    // Game state
    int currentGameId;
    int currentQuestionNumber;
    
    // Auth state
    std::string authToken;
    std::string username;
    std::string role;
    
    bool isAuthenticated() const { return !authToken.empty(); }
    bool isAdmin() const { return role == "admin"; }
    
    // Wait for response
    SocketClient::Message waitForResponse(int timeoutMs = 5000);
    
    // Accessors for Qt integration
    SocketClient* getClient() const { return client_; }
    class GameEventQueue* getEventQueue() const;
    
private:
    SocketClient* client_;
    class GameEventQueue* eventQueue_;
    std::string buildDataJson(const std::map<std::string, std::string>& strings,
                              const std::map<std::string, int>& ints = {},
                              const std::map<std::string, bool>& bools = {});
};

#endif // PROTOCOL_HANDLER_H

