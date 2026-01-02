#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "socket_client.h"
#include "json_utils.h"
#include <string>
#include <map>
#include <functional>

class ProtocolHandler {
public:
    ProtocolHandler(SocketClient* client);
    
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
    
private:
    SocketClient* client_;
    std::string buildDataJson(const std::map<std::string, std::string>& strings,
                              const std::map<std::string, int>& ints = {},
                              const std::map<std::string, bool>& bools = {});
};

#endif // PROTOCOL_HANDLER_H

