#include "protocol_handler.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

ProtocolHandler::ProtocolHandler(SocketClient* client)
    : currentGameId(0), currentQuestionNumber(0), client_(client) {
}

ProtocolHandler::LoginResponse ProtocolHandler::login(const std::string& username, const std::string& password) {
    LoginResponse response;
    response.responseCode = 500;
    response.message = "Internal error";
    
    std::map<std::string, std::string> strings;
    strings["username"] = username;
    strings["password"] = password;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("LOGIN", data)) {
        response.responseCode = 503;
        response.message = "Failed to send request";
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        response.message = "Request timeout";
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    response.message = MillionaireGame::JsonUtils::extractString(msg.data, "message");
    
    if (response.responseCode == 200) {
        response.authToken = MillionaireGame::JsonUtils::extractString(msg.data, "authToken");
        response.username = MillionaireGame::JsonUtils::extractString(msg.data, "username");
        response.role = MillionaireGame::JsonUtils::extractString(msg.data, "role");
        
        this->authToken = response.authToken;
        this->username = response.username;
        this->role = response.role;
    }
    
    return response;
}

int ProtocolHandler::registerUser(const std::string& username, const std::string& password) {
    std::map<std::string, std::string> strings;
    strings["username"] = username;
    strings["password"] = password;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("REGISTER", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

bool ProtocolHandler::logout() {
    if (!client_->isConnected() || authToken.empty()) {
        return false;
    }
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("LOGOUT", data)) {
        return false;
    }
    
    SocketClient::Message msg = waitForResponse(3000);
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    if (code == 200) {
        authToken.clear();
        username.clear();
        role.clear();
    }
    
    return code == 200;
}

int ProtocolHandler::startGame(bool overrideSavedGame) {
    std::cerr << "[DEBUG] startGame called, overrideSavedGame=" << overrideSavedGame << std::endl;
    std::cerr << "[DEBUG] authToken=" << (authToken.empty() ? "EMPTY" : authToken.substr(0, 10) + "...") << std::endl;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, bool> bools;
    bools["overrideSavedGame"] = overrideSavedGame;
    
    std::string data = buildDataJson(strings, {}, bools);
    std::cerr << "[DEBUG] Sending START request, data=" << data << std::endl;
    
    if (!client_->sendRequest("START", data)) {
        std::cerr << "[DEBUG] Failed to send START request" << std::endl;
        return 503;
    }
    
    std::cerr << "[DEBUG] Waiting for response..." << std::endl;
    SocketClient::Message msg = waitForResponse(5000);
    
    std::cerr << "[DEBUG] Received message, type=" << msg.type << ", data=" << msg.data.substr(0, 200) << std::endl;
    
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        std::cerr << "[DEBUG] Timeout or empty response" << std::endl;
        return 504;
    }
    
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    std::cerr << "[DEBUG] Extracted responseCode=" << code << std::endl;
    
    if (code == 200) {
        currentGameId = MillionaireGame::JsonUtils::extractInt(msg.data, "gameId", 0);
        currentQuestionNumber = MillionaireGame::JsonUtils::extractInt(msg.data, "questionNumber", 1);
        std::cerr << "[DEBUG] Game started successfully, gameId=" << currentGameId << ", questionNumber=" << currentQuestionNumber << std::endl;
    } else {
        std::string errorMsg = MillionaireGame::JsonUtils::extractString(msg.data, "message");
        std::cerr << "[DEBUG] Start game failed with code=" << code << ", message=" << errorMsg << std::endl;
    }
    
    return code;
}

int ProtocolHandler::resumeGame() {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("RESUME", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (code == 200) {
        currentGameId = MillionaireGame::JsonUtils::extractInt(msg.data, "gameId", 0);
        currentQuestionNumber = MillionaireGame::JsonUtils::extractInt(msg.data, "questionNumber", 1);
    }
    
    return code;
}

ProtocolHandler::AnswerResponse ProtocolHandler::answerQuestion(int answerIndex) {
    AnswerResponse response;
    response.responseCode = 500;
    response.correct = false;
    response.gameOver = false;
    response.isWinner = false;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["answerIndex"] = answerIndex;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;  // FIXED: Added questionNumber as required by server
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("ANSWER", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(10000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    response.correct = MillionaireGame::JsonUtils::extractBool(msg.data, "correct", false);
    response.questionNumber = MillionaireGame::JsonUtils::extractInt(msg.data, "questionNumber", 0);
    response.timeRemaining = MillionaireGame::JsonUtils::extractInt(msg.data, "timeRemaining", 0);
    response.pointsEarned = MillionaireGame::JsonUtils::extractInt(msg.data, "pointsEarned", 0);
    response.totalScore = MillionaireGame::JsonUtils::extractInt(msg.data, "totalScore", 0);
    response.currentPrize = MillionaireGame::JsonUtils::extractInt(msg.data, "currentPrize", 0);
    response.gameOver = MillionaireGame::JsonUtils::extractBool(msg.data, "gameOver", false);
    response.isWinner = MillionaireGame::JsonUtils::extractBool(msg.data, "isWinner", false);
    response.correctAnswer = MillionaireGame::JsonUtils::extractInt(msg.data, "correctAnswer", -1);
    response.finalPrize = MillionaireGame::JsonUtils::extractInt(msg.data, "finalPrize", 0);
    
    if (response.responseCode == 200 && !response.gameOver) {
        currentQuestionNumber = response.questionNumber;
    }
    
    return response;
}

int ProtocolHandler::useLifeline(const std::string& lifelineType) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["lifelineType"] = lifelineType;
    
    std::map<std::string, int> ints;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;  // FIXED: Added questionNumber as required by server
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("LIFELINE", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::giveUp() {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;  // FIXED: Added questionNumber as required by server
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("GIVE_UP", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::leaveGame() {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    // FIXED: According to server handler, LEAVE_GAME only needs authToken (no gameId or questionNumber)
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("LEAVE_GAME", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

SocketClient::Message ProtocolHandler::waitForResponse(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    SocketClient::Message msg;
    int messageCount = 0;
    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        
        if (elapsedMs >= timeoutMs) {
            std::cerr << "[DEBUG] waitForResponse timeout after " << elapsedMs << "ms, checked " << messageCount << " messages" << std::endl;
            msg.type = "TIMEOUT";
            msg.data = "";
            return msg;
        }
        
        int remainingMs = timeoutMs - elapsedMs;
        if (client_->getMessage(msg, remainingMs > 100 ? 100 : remainingMs)) {
            messageCount++;
            std::cerr << "[DEBUG] waitForResponse got message #" << messageCount << ", type=" << msg.type 
                      << ", data preview=" << msg.data.substr(0, 100) << std::endl;
            
            if (msg.type == "RESPONSE") {
                if (msg.data.find("\"type\":\"CONNECTION\"") != std::string::npos) {
                    std::cerr << "[DEBUG] Skipping CONNECTION notification" << std::endl;
                    continue;
                }
                
                if (msg.data.find("\"type\":\"GAME_START\"") != std::string::npos) {
                    std::cerr << "[DEBUG] Skipping GAME_START notification" << std::endl;
                    continue;
                }
                
                if (msg.data.find("\"responseCode\"") != std::string::npos) {
                    std::cerr << "[DEBUG] Found response with responseCode, returning" << std::endl;
                    return msg;
                } else {
                    std::cerr << "[DEBUG] RESPONSE message without responseCode, putting back for notification handler" << std::endl;
                    client_->putMessageBack(msg);
                }
            } else {
                // Non-RESPONSE message (QUESTION_INFO, GAME_END, etc.) - put it back for notification handler
                std::cerr << "[DEBUG] Non-RESPONSE message type (" << msg.type << "), putting back for notification handler" << std::endl;
                client_->putMessageBack(msg);
            }
        }
    }
}

std::string ProtocolHandler::buildDataJson(const std::map<std::string, std::string>& strings,
                                           const std::map<std::string, int>& ints,
                                           const std::map<std::string, bool>& bools) {
    return MillionaireGame::JsonUtils::buildJson(strings, ints, bools);
}

// Connection methods
int ProtocolHandler::sendConnection() {
    std::map<std::string, std::string> strings;
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("CONNECTION", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(3000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::sendPing() {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("PING", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(3000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

// Social features
ProtocolHandler::LeaderboardResponse ProtocolHandler::getLeaderboard(const std::string& type, int page, int limit) {
    LeaderboardResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["type"] = type;
    
    std::map<std::string, int> ints;
    ints["page"] = page;
    ints["limit"] = limit;
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("LEADERBOARD", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    response.total = MillionaireGame::JsonUtils::extractInt(msg.data, "total", 0);
    response.page = MillionaireGame::JsonUtils::extractInt(msg.data, "page", 1);
    response.limit = MillionaireGame::JsonUtils::extractInt(msg.data, "limit", 20);
    
    // TODO: Parse rankings array from JSON
    // This requires more sophisticated JSON parsing for arrays
    
    return response;
}

ProtocolHandler::FriendStatusResponse ProtocolHandler::getFriendStatus() {
    FriendStatusResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("FRIEND_STATUS", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    // TODO: Parse friends array from JSON
    
    return response;
}

int ProtocolHandler::addFriend(const std::string& friendUsername) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["friendUsername"] = friendUsername;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("ADD_FRIEND", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::acceptFriend(const std::string& friendUsername) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["friendUsername"] = friendUsername;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("ACCEPT_FRIEND", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::declineFriend(const std::string& friendUsername) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["friendUsername"] = friendUsername;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("DECLINE_FRIEND", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

ProtocolHandler::FriendReqListResponse ProtocolHandler::getFriendReqList() {
    FriendReqListResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("FRIEND_REQ_LIST", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    // TODO: Parse friendRequests array from JSON
    
    return response;
}

int ProtocolHandler::deleteFriend(const std::string& friendUsername) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["friendUsername"] = friendUsername;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("DEL_FRIEND", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::sendChat(const std::string& recipient, const std::string& message) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["recipient"] = recipient;
    strings["message"] = message;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("CHAT", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

// User information
ProtocolHandler::UserInfo ProtocolHandler::getUserInfo(const std::string& username) {
    UserInfo response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["username"] = username;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("USER_INFO", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (response.responseCode == 200) {
        response.username = MillionaireGame::JsonUtils::extractString(msg.data, "username");
        response.totalGames = MillionaireGame::JsonUtils::extractInt(msg.data, "totalGames", 0);
        response.highestPrize = MillionaireGame::JsonUtils::extractInt(msg.data, "highestPrize", 0);
        response.finalQuestionNumber = MillionaireGame::JsonUtils::extractInt(msg.data, "finalQuestionNumber", 0);
        response.totalScore = MillionaireGame::JsonUtils::extractInt(msg.data, "totalScore", 0);
    }
    
    return response;
}

ProtocolHandler::ViewHistoryResponse ProtocolHandler::viewHistory() {
    ViewHistoryResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("VIEW_HISTORY", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    // TODO: Parse games array from JSON
    
    return response;
}

int ProtocolHandler::changePassword(const std::string& oldPassword, const std::string& newPassword) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["oldPassword"] = oldPassword;
    strings["newPassword"] = newPassword;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("CHANGE_PASS", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

// Admin functions
ProtocolHandler::AddQuestionResponse ProtocolHandler::addQuestion(const AddQuestionRequest& req) {
    AddQuestionResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["question"] = req.question;
    strings["lifeline_call_info"] = req.lifeline_call_info;
    
    std::map<std::string, int> ints;
    ints["correctAnswer"] = req.correctAnswer;
    ints["level"] = req.level;
    
    // Build options array manually in JSON
    std::string options_json = "[";
    for (size_t i = 0; i < req.options.size(); i++) {
        if (i > 0) options_json += ",";
        options_json += "\"" + req.options[i] + "\"";
    }
    options_json += "]";
    
    // Build complete data JSON with options array and lifeline info
    std::string data = "{";
    data += "\"authToken\":\"" + authToken + "\",";
    data += "\"question\":\"" + req.question + "\",";
    data += "\"options\":" + options_json + ",";
    data += "\"correctAnswer\":" + std::to_string(req.correctAnswer) + ",";
    data += "\"level\":" + std::to_string(req.level) + ",";
    data += "\"lifeline_5050_info\":" + req.lifeline_5050_info + ",";
    data += "\"lifeline_ask_info\":" + req.lifeline_ask_info + ",";
    data += "\"lifeline_call_info\":\"" + req.lifeline_call_info + "\"";
    data += "}";
    
    if (!client_->sendRequest("ADD_QUES", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (response.responseCode == 200) {
        response.questionId = MillionaireGame::JsonUtils::extractInt(msg.data, "questionId", 0);
        response.message = MillionaireGame::JsonUtils::extractString(msg.data, "message");
    }
    
    return response;
}

int ProtocolHandler::changeQuestion(int questionId, const std::string& question, 
                                   const std::vector<std::string>& options, int correctAnswer) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["question"] = question;
    
    std::map<std::string, int> ints;
    ints["questionId"] = questionId;
    ints["correctAnswer"] = correctAnswer;
    
    // Build options array manually in JSON
    std::string options_json = "[";
    for (size_t i = 0; i < options.size(); i++) {
        if (i > 0) options_json += ",";
        options_json += "\"" + options[i] + "\"";
    }
    options_json += "]";
    
    // Build complete data JSON with options array
    std::string data = "{";
    data += "\"authToken\":\"" + authToken + "\",";
    data += "\"questionId\":" + std::to_string(questionId) + ",";
    data += "\"question\":\"" + question + "\",";
    data += "\"options\":" + options_json + ",";
    data += "\"correctAnswer\":" + std::to_string(correctAnswer);
    data += "}";
    
    if (!client_->sendRequest("CHANGE_QUES", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

ProtocolHandler::ViewQuestionsResponse ProtocolHandler::viewQuestions(int page, int limit, int level) {
    ViewQuestionsResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["page"] = page;
    ints["limit"] = limit;
    if (level >= 0) {
        ints["level"] = level;
    }
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("VIEW_QUES", data)) {
        response.responseCode = 503;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        response.responseCode = 504;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    response.total = MillionaireGame::JsonUtils::extractInt(msg.data, "total", 0);
    response.page = MillionaireGame::JsonUtils::extractInt(msg.data, "page", 1);
    
    // TODO: Parse questions array from JSON
    
    return response;
}

int ProtocolHandler::deleteQuestion(int questionId) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["questionId"] = questionId;
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("DEL_QUES", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::banUser(const std::string& username, const std::string& reason) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["username"] = username;
    strings["reason"] = reason;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("BAN_USER", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}


