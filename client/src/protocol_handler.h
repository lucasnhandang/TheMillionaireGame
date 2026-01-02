#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "client_core.h"
#include "utils/json_parser.h"
#include <string>
#include <vector>
#include <map>

namespace MillionaireGame {

/**
 * Protocol handler for building requests and parsing responses
 * Handles all request types defined in PROTOCOL.md
 */
class ProtocolHandler {
public:
    ProtocolHandler(ClientCore* client);
    
    // Authentication
    struct LoginResponse {
        bool success;
        int responseCode;
        std::string authToken;
        std::string username;
        std::string role;
        std::string message;
    };
    
    struct RegisterResponse {
        bool success;
        int responseCode;
        std::string username;
        std::string message;
    };
    
    LoginResponse login(const std::string& username, const std::string& password);
    RegisterResponse registerUser(const std::string& username, const std::string& password);
    bool logout(const std::string& authToken);
    
    // Game Actions
    struct StartResponse {
        bool success;
        int responseCode;
        std::string message;
    };
    
    struct AnswerResponse {
        bool success;
        int responseCode;
        int gameId;
        bool correct;
        int questionNumber;
        int timeRemaining;
        int pointsEarned;
        int totalScore;
        long long currentPrize;
        bool gameOver;
        bool isWinner;
        int correctAnswer; // For wrong answers
        long long safeCheckpointPrize;
        int safeCheckpointScore;
        long long finalPrize;
    };
    
    struct LifelineResponse {
        bool success;
        int responseCode;
        std::string message;
    };
    
    struct GiveUpResponse {
        bool success;
        int responseCode;
        long long finalPrize;
        int finalQuestionNumber;
        int totalScore;
        int gameId;
        std::string message;
    };
    
    struct ResumeResponse {
        bool success;
        int responseCode;
        int questionNumber;
        long long prize;
        int gameId;
        int totalScore;
        std::string message;
    };
    
    StartResponse startGame(const std::string& authToken, bool overrideSavedGame = false);
    AnswerResponse answerQuestion(const std::string& authToken, int gameId, int questionNumber, int answerIndex);
    LifelineResponse useLifeline(const std::string& authToken, int gameId, int questionNumber, const std::string& lifelineType);
    GiveUpResponse giveUp(const std::string& authToken, int gameId, int questionNumber);
    ResumeResponse resumeGame(const std::string& authToken);
    bool leaveGame(const std::string& authToken);
    
    // Social Features
    struct LeaderboardEntry {
        std::string username;
        int finalQuestionNumber;
        int totalScore;
        int rank;
        bool isWinner;
    };
    
    struct LeaderboardResponse {
        bool success;
        int responseCode;
        std::vector<LeaderboardEntry> rankings;
        int total;
        int page;
        int limit;
    };
    
    struct FriendStatus {
        std::string username;
        std::string status; // online, ingame, offline
    };
    
    LeaderboardResponse getLeaderboard(const std::string& authToken, const std::string& type, int page = 1, int limit = 20);
    std::vector<FriendStatus> getFriendStatus(const std::string& authToken);
    bool addFriend(const std::string& authToken, const std::string& friendUsername);
    bool acceptFriend(const std::string& authToken, const std::string& friendUsername);
    bool declineFriend(const std::string& authToken, const std::string& friendUsername);
    std::vector<std::pair<std::string, long>> getFriendRequestList(const std::string& authToken);
    bool deleteFriend(const std::string& authToken, const std::string& friendUsername);
    bool sendChat(const std::string& authToken, const std::string& recipient, const std::string& message);
    
    // User Information
    struct UserInfo {
        std::string username;
        int totalGames;
        long long highestPrize;
        int finalQuestionNumber;
        int totalScore;
    };
    
    struct GameHistoryEntry {
        int gameId;
        std::string date;
        int finalQuestionNumber;
        int totalScore;
        long long finalPrize;
        std::string status; // won, lost, quit
    };
    
    UserInfo getUserInfo(const std::string& authToken, const std::string& username);
    std::vector<GameHistoryEntry> viewHistory(const std::string& authToken);
    bool changePassword(const std::string& authToken, const std::string& oldPassword, const std::string& newPassword);
    
    // Connection
    bool ping(const std::string& authToken);
    
    // Notification parsing
    struct QuestionInfo {
        int questionId;
        int questionNumber;
        std::string question;
        std::vector<std::pair<int, std::string>> options; // index, label, text
        std::vector<std::string> optionLabels;
        std::vector<std::string> optionTexts;
        long long prize;
        int totalQuestions;
        std::vector<std::string> lifelines;
        int timeLimit;
        int timeRemaining;
        int gameId;
        int totalScore;
    };
    
    struct LifelineInfo {
        std::string lifelineType;
        int questionNumber;
        std::vector<int> remainingOptions; // For 5050
        std::string suggestion; // For PHONE
        std::map<std::string, int> poll; // For AUDIENCE
        std::vector<std::string> lifelinesLeft;
        int timeRemaining;
        int lifelinePenalty;
        int maxPointsAfterLifeline;
    };
    
    struct GameStartInfo {
        int gameId;
        long timestamp;
    };
    
    struct GameEndInfo {
        int gameId;
        std::string status; // won, lost, quit
        int finalLevel;
        int finalQuestionNumber;
        long long safeCheckpointPrize;
        int safeCheckpointScore;
        long long finalPrize;
        int totalScore;
        bool isWinner;
    };
    
    bool isNotification(const std::string& message);
    bool isResponse(const std::string& message);
    std::string getNotificationType(const std::string& message);
    QuestionInfo parseQuestionInfo(const std::string& message);
    LifelineInfo parseLifelineInfo(const std::string& message);
    GameStartInfo parseGameStart(const std::string& message);
    GameEndInfo parseGameEnd(const std::string& message);
    
    // Generic response parsing
    int getResponseCode(const std::string& message);
    std::string getResponseMessage(const std::string& message);

private:
    ClientCore* client_;
    
    std::string buildRequest(const std::string& requestType, const std::vector<std::pair<std::string, std::string>>& stringFields,
                            const std::vector<std::pair<std::string, int>>& intFields = {},
                            const std::vector<std::pair<std::string, bool>>& boolFields = {});
    std::string sendRequestAndReceive(const std::string& request);
};

} // namespace MillionaireGame

#endif // PROTOCOL_HANDLER_H

