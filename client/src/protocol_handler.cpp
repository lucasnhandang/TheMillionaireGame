#include "protocol_handler.h"
#include <iostream>
#include <sstream>

using namespace std;

namespace MillionaireGame {

ProtocolHandler::ProtocolHandler(ClientCore* client) : client_(client) {
}

string ProtocolHandler::buildRequest(const string& requestType,
                                     const vector<pair<string, string>>& stringFields,
                                     const vector<pair<string, int>>& intFields,
                                     const vector<pair<string, bool>>& boolFields) {
    stringstream ss;
    ss << "{\"requestType\":\"" << requestType << "\",\"data\":";
    
    vector<pair<string, string>> dataFields = stringFields;
    vector<pair<string, int>> dataIntFields = intFields;
    vector<pair<string, bool>> dataBoolFields = boolFields;
    
    ss << JsonParser::buildObject(dataFields, dataIntFields, dataBoolFields);
    ss << "}";
    
    return ss.str();
}

string ProtocolHandler::sendRequestAndReceive(const string& request) {
    if (!client_->sendMessage(request)) {
        return "";
    }
    
    // Receive response (may need to skip notifications)
    string message;
    int attempts = 0;
    const int maxAttempts = 10; // Max attempts to get a response (not notification)
    
    while (attempts < maxAttempts) {
        message = client_->receiveMessage(2); // 2 second timeout per attempt
        
        if (message.empty()) {
            return ""; // Connection error or timeout
        }
        
        // Check if this is a response (has responseCode) or notification (has type)
        if (isResponse(message)) {
            return message; // This is the response we want
        } else if (isNotification(message)) {
            // This is a notification, skip it and wait for response
            // In production, handle notification properly
            attempts++;
            continue;
        } else {
            // Unknown format, return it anyway
            return message;
        }
    }
    
    return ""; // Timeout waiting for response
}

// Authentication
ProtocolHandler::LoginResponse ProtocolHandler::login(const string& username, const string& password) {
    LoginResponse response;
    response.success = false;
    
    vector<pair<string, string>> fields;
    fields.push_back({"username", username});
    fields.push_back({"password", password});
    
    string request = buildRequest("LOGIN", fields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        response.message = "Connection error";
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    
    if (response.responseCode == 200) {
        response.success = true;
        response.authToken = JsonParser::extractString(reply, "authToken");
        response.username = JsonParser::extractString(reply, "username");
        response.role = JsonParser::extractString(reply, "role");
        response.message = JsonParser::extractString(reply, "message");
    } else {
        response.message = getResponseMessage(reply);
    }
    
    return response;
}

ProtocolHandler::RegisterResponse ProtocolHandler::registerUser(const string& username, const string& password) {
    RegisterResponse response;
    response.success = false;
    
    vector<pair<string, string>> fields;
    fields.push_back({"username", username});
    fields.push_back({"password", password});
    
    string request = buildRequest("REGISTER", fields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        response.message = "Connection error";
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    
    if (response.responseCode == 201) {
        response.success = true;
        response.username = JsonParser::extractString(reply, "username");
        response.message = JsonParser::extractString(reply, "message");
    } else {
        response.message = getResponseMessage(reply);
    }
    
    return response;
}

bool ProtocolHandler::logout(const string& authToken) {
    vector<pair<string, string>> fields;
    fields.push_back({"authToken", authToken});
    
    string request = buildRequest("LOGOUT", fields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

// Game Actions
ProtocolHandler::StartResponse ProtocolHandler::startGame(const string& authToken, bool overrideSavedGame) {
    StartResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    vector<pair<string, bool>> boolFields;
    boolFields.push_back({"overrideSavedGame", overrideSavedGame});
    
    string request = buildRequest("START", stringFields, {}, boolFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        response.message = "Connection error";
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    response.message = getResponseMessage(reply);
    
    return response;
}

ProtocolHandler::AnswerResponse ProtocolHandler::answerQuestion(const string& authToken, int gameId, int questionNumber, int answerIndex) {
    AnswerResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    vector<pair<string, int>> intFields;
    intFields.push_back({"gameId", gameId});
    intFields.push_back({"questionNumber", questionNumber});
    intFields.push_back({"answerIndex", answerIndex});
    
    string request = buildRequest("ANSWER", stringFields, intFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    
    if (response.success) {
        response.gameId = JsonParser::extractInt(reply, "gameId");
        response.correct = JsonParser::extractBool(reply, "correct");
        response.questionNumber = JsonParser::extractInt(reply, "questionNumber");
        response.timeRemaining = JsonParser::extractInt(reply, "timeRemaining");
        response.pointsEarned = JsonParser::extractInt(reply, "pointsEarned");
        response.totalScore = JsonParser::extractInt(reply, "totalScore");
        response.currentPrize = JsonParser::extractInt(reply, "currentPrize");
        response.gameOver = JsonParser::extractBool(reply, "gameOver");
        response.isWinner = JsonParser::extractBool(reply, "isWinner");
        response.correctAnswer = JsonParser::extractInt(reply, "correctAnswer", -1);
        response.safeCheckpointPrize = JsonParser::extractInt(reply, "safeCheckpointPrize");
        response.safeCheckpointScore = JsonParser::extractInt(reply, "safeCheckpointScore");
        response.finalPrize = JsonParser::extractInt(reply, "finalPrize");
    }
    
    return response;
}

ProtocolHandler::LifelineResponse ProtocolHandler::useLifeline(const string& authToken, int gameId, int questionNumber, const string& lifelineType) {
    LifelineResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"lifelineType", lifelineType});
    
    vector<pair<string, int>> intFields;
    intFields.push_back({"gameId", gameId});
    intFields.push_back({"questionNumber", questionNumber});
    
    string request = buildRequest("LIFELINE", stringFields, intFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        response.message = "Connection error";
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    response.message = getResponseMessage(reply);
    
    return response;
}

ProtocolHandler::GiveUpResponse ProtocolHandler::giveUp(const string& authToken, int gameId, int questionNumber) {
    GiveUpResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    vector<pair<string, int>> intFields;
    intFields.push_back({"gameId", gameId});
    intFields.push_back({"questionNumber", questionNumber});
    
    string request = buildRequest("GIVE_UP", stringFields, intFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    
    if (response.success) {
        response.finalPrize = JsonParser::extractInt(reply, "finalPrize");
        response.finalQuestionNumber = JsonParser::extractInt(reply, "finalQuestionNumber");
        response.totalScore = JsonParser::extractInt(reply, "totalScore");
        response.gameId = JsonParser::extractInt(reply, "gameId");
        response.message = JsonParser::extractString(reply, "message");
    }
    
    return response;
}

ProtocolHandler::ResumeResponse ProtocolHandler::resumeGame(const string& authToken) {
    ResumeResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("RESUME", stringFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    
    if (response.success) {
        response.questionNumber = JsonParser::extractInt(reply, "questionNumber");
        response.prize = JsonParser::extractInt(reply, "prize");
        response.gameId = JsonParser::extractInt(reply, "gameId");
        response.totalScore = JsonParser::extractInt(reply, "totalScore");
        response.message = JsonParser::extractString(reply, "message");
    }
    
    return response;
}

bool ProtocolHandler::leaveGame(const string& authToken) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("LEAVE_GAME", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

// Social Features
ProtocolHandler::LeaderboardResponse ProtocolHandler::getLeaderboard(const string& authToken, const string& type, int page, int limit) {
    LeaderboardResponse response;
    response.success = false;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"type", type});
    
    vector<pair<string, int>> intFields;
    intFields.push_back({"page", page});
    intFields.push_back({"limit", limit});
    
    string request = buildRequest("LEADERBOARD", stringFields, intFields);
    string reply = sendRequestAndReceive(request);
    
    if (reply.empty()) {
        response.responseCode = 500;
        return response;
    }
    
    response.responseCode = getResponseCode(reply);
    response.success = (response.responseCode == 200);
    
    if (response.success) {
        // Parse rankings array (simplified - would need proper JSON array parsing)
        response.total = JsonParser::extractInt(reply, "total");
        response.page = JsonParser::extractInt(reply, "page");
        response.limit = JsonParser::extractInt(reply, "limit");
        
        // Note: Full array parsing would require more complex JSON parsing
        // For now, this is a simplified version
    }
    
    return response;
}

vector<ProtocolHandler::FriendStatus> ProtocolHandler::getFriendStatus(const string& authToken) {
    vector<FriendStatus> result;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("FRIEND_STATUS", stringFields);
    string reply = sendRequestAndReceive(request);
    
    // Simplified parsing - full implementation would parse friends array
    return result;
}

bool ProtocolHandler::addFriend(const string& authToken, const string& friendUsername) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"friendUsername", friendUsername});
    
    string request = buildRequest("ADD_FRIEND", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

bool ProtocolHandler::acceptFriend(const string& authToken, const string& friendUsername) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"friendUsername", friendUsername});
    
    string request = buildRequest("ACCEPT_FRIEND", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

bool ProtocolHandler::declineFriend(const string& authToken, const string& friendUsername) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"friendUsername", friendUsername});
    
    string request = buildRequest("DECLINE_FRIEND", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

vector<pair<string, long>> ProtocolHandler::getFriendRequestList(const string& authToken) {
    vector<pair<string, long>> result;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("FRIEND_REQ_LIST", stringFields);
    string reply = sendRequestAndReceive(request);
    
    // Simplified parsing
    return result;
}

bool ProtocolHandler::deleteFriend(const string& authToken, const string& friendUsername) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"friendUsername", friendUsername});
    
    string request = buildRequest("DEL_FRIEND", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

bool ProtocolHandler::sendChat(const string& authToken, const string& recipient, const string& message) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"recipient", recipient});
    stringFields.push_back({"message", message});
    
    string request = buildRequest("CHAT", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

// User Information
ProtocolHandler::UserInfo ProtocolHandler::getUserInfo(const string& authToken, const string& username) {
    UserInfo info;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"username", username});
    
    string request = buildRequest("USER_INFO", stringFields);
    string reply = sendRequestAndReceive(request);
    
    if (getResponseCode(reply) == 200) {
        info.username = JsonParser::extractString(reply, "username");
        info.totalGames = JsonParser::extractInt(reply, "totalGames");
        info.highestPrize = JsonParser::extractInt(reply, "highestPrize");
        info.finalQuestionNumber = JsonParser::extractInt(reply, "finalQuestionNumber");
        info.totalScore = JsonParser::extractInt(reply, "totalScore");
    }
    
    return info;
}

vector<ProtocolHandler::GameHistoryEntry> ProtocolHandler::viewHistory(const string& authToken) {
    vector<GameHistoryEntry> result;
    
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("VIEW_HISTORY", stringFields);
    string reply = sendRequestAndReceive(request);
    
    // Simplified parsing - full implementation would parse games array
    return result;
}

bool ProtocolHandler::changePassword(const string& authToken, const string& oldPassword, const string& newPassword) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    stringFields.push_back({"oldPassword", oldPassword});
    stringFields.push_back({"newPassword", newPassword});
    
    string request = buildRequest("CHANGE_PASS", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

// Connection
bool ProtocolHandler::ping(const string& authToken) {
    vector<pair<string, string>> stringFields;
    stringFields.push_back({"authToken", authToken});
    
    string request = buildRequest("PING", stringFields);
    string reply = sendRequestAndReceive(request);
    
    return getResponseCode(reply) == 200;
}

// Notification parsing
bool ProtocolHandler::isNotification(const string& message) {
    return message.find("\"type\":") != string::npos;
}

bool ProtocolHandler::isResponse(const string& message) {
    return message.find("\"responseCode\":") != string::npos;
}

string ProtocolHandler::getNotificationType(const string& message) {
    return JsonParser::extractString(message, "type");
}

ProtocolHandler::QuestionInfo ProtocolHandler::parseQuestionInfo(const string& message) {
    QuestionInfo info;
    
    // Extract from data field
    size_t dataPos = message.find("\"data\":");
    if (dataPos == string::npos) return info;
    
    string data = message.substr(dataPos);
    
    info.questionId = JsonParser::extractInt(data, "questionId");
    info.questionNumber = JsonParser::extractInt(data, "questionNumber");
    info.question = JsonParser::extractString(data, "question");
    info.prize = JsonParser::extractInt(data, "prize");
    info.totalQuestions = JsonParser::extractInt(data, "totalQuestions");
    info.timeLimit = JsonParser::extractInt(data, "timeLimit");
    info.timeRemaining = JsonParser::extractInt(data, "timeRemaining");
    info.gameId = JsonParser::extractInt(data, "gameId");
    info.totalScore = JsonParser::extractInt(data, "totalScore");
    
    // Parse options array (simplified)
    // Full implementation would parse the options array properly
    
    return info;
}

ProtocolHandler::LifelineInfo ProtocolHandler::parseLifelineInfo(const string& message) {
    LifelineInfo info;
    
    size_t dataPos = message.find("\"data\":");
    if (dataPos == string::npos) return info;
    
    string data = message.substr(dataPos);
    
    info.lifelineType = JsonParser::extractString(data, "lifelineType");
    info.questionNumber = JsonParser::extractInt(data, "questionNumber");
    info.suggestion = JsonParser::extractString(data, "suggestion");
    info.timeRemaining = JsonParser::extractInt(data, "timeRemaining");
    info.lifelinePenalty = JsonParser::extractInt(data, "lifelinePenalty");
    info.maxPointsAfterLifeline = JsonParser::extractInt(data, "maxPointsAfterLifeline");
    
    // Parse remainingOptions, poll, lifelinesLeft arrays (simplified)
    
    return info;
}

ProtocolHandler::GameStartInfo ProtocolHandler::parseGameStart(const string& message) {
    GameStartInfo info;
    
    size_t dataPos = message.find("\"data\":");
    if (dataPos == string::npos) return info;
    
    string data = message.substr(dataPos);
    
    info.gameId = JsonParser::extractInt(data, "gameId");
    info.timestamp = JsonParser::extractInt(data, "timestamp");
    
    return info;
}

ProtocolHandler::GameEndInfo ProtocolHandler::parseGameEnd(const string& message) {
    GameEndInfo info;
    
    size_t dataPos = message.find("\"data\":");
    if (dataPos == string::npos) return info;
    
    string data = message.substr(dataPos);
    
    info.gameId = JsonParser::extractInt(data, "gameId");
    info.status = JsonParser::extractString(data, "status");
    info.finalLevel = JsonParser::extractInt(data, "finalLevel");
    info.finalQuestionNumber = JsonParser::extractInt(data, "finalQuestionNumber");
    info.safeCheckpointPrize = JsonParser::extractInt(data, "safeCheckpointPrize");
    info.safeCheckpointScore = JsonParser::extractInt(data, "safeCheckpointScore");
    info.finalPrize = JsonParser::extractInt(data, "finalPrize");
    info.totalScore = JsonParser::extractInt(data, "totalScore");
    info.isWinner = JsonParser::extractBool(data, "isWinner");
    
    return info;
}

// Generic response parsing
int ProtocolHandler::getResponseCode(const string& message) {
    return JsonParser::extractInt(message, "responseCode", 500);
}

string ProtocolHandler::getResponseMessage(const string& message) {
    string msg = JsonParser::extractString(message, "message");
    if (msg.empty()) {
        // Try to extract from data.message
        size_t dataPos = message.find("\"data\":");
        if (dataPos != string::npos) {
            string data = message.substr(dataPos);
            msg = JsonParser::extractString(data, "message");
        }
    }
    return msg;
}

} // namespace MillionaireGame

