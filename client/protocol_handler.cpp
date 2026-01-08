#include "protocol_handler.h"
#include "game_event.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

ProtocolHandler::ProtocolHandler(SocketClient* client)
    : currentGameId(0), currentQuestionNumber(0), client_(client), eventQueue_(new GameEventQueue()) {
}

ProtocolHandler::~ProtocolHandler() {
    delete eventQueue_;
}

GameEventQueue* ProtocolHandler::getEventQueue() const {
    return eventQueue_;
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
        std::cerr << "[DEBUG] ProtocolHandler - Full login response: " << msg.data << std::endl;
        response.authToken = MillionaireGame::JsonUtils::extractString(msg.data, "authToken");
        response.username = MillionaireGame::JsonUtils::extractString(msg.data, "username");
        response.role = MillionaireGame::JsonUtils::extractString(msg.data, "role");
        std::cerr << "[DEBUG] ProtocolHandler - Extracted role: '" << response.role << "'" << std::endl;
        
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
            
            // Simple logic: If msg.type == "RESPONSE", it's a request response (has responseCode)
            // All other types are notifications (have notificationType) - put them back for notification handler
            if (msg.type == "RESPONSE") {
                std::cerr << "[DEBUG] Found RESPONSE with responseCode, returning" << std::endl;
                return msg;
            } else {
                // This is a notification (GAME_START, QUESTION_INFO, etc.) - put it back for notification handler
                std::cerr << "[DEBUG] Notification type (" << msg.type << "), putting back for notification handler" << std::endl;
                client_->putMessageBack(msg);
                // Yield to give notification handler a chance to grab the message
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    
    if (response.responseCode == 200) {
        // Basic metadata
        response.total = MillionaireGame::JsonUtils::extractInt(msg.data, "total", 0);
        response.page = MillionaireGame::JsonUtils::extractInt(msg.data, "page", 1);
        
        // Parse questions array from JSON (similar approach as viewUsers)
        size_t questions_start = msg.data.find("\"questions\"");
        if (questions_start != std::string::npos) {
            size_t array_start = msg.data.find("[", questions_start);
            if (array_start != std::string::npos) {
                size_t pos = array_start + 1;
                while (pos < msg.data.length()) {
                    // Skip whitespace
                    while (pos < msg.data.length() &&
                           (msg.data[pos] == ' ' || msg.data[pos] == '\t' || msg.data[pos] == '\n')) {
                        pos++;
                    }
                    if (pos >= msg.data.length() || msg.data[pos] == ']') {
                        break;
                    }
                    
                    // Find object
                    if (msg.data[pos] == '{') {
                        size_t obj_end = msg.data.find("}", pos);
                        if (obj_end != std::string::npos) {
                            std::string question_obj = msg.data.substr(pos, obj_end - pos + 1);
                            
                            QuestionInfo q;
                            q.questionId = MillionaireGame::JsonUtils::extractInt(question_obj, "questionId", -1);
                            q.question = MillionaireGame::JsonUtils::extractString(question_obj, "question");
                            q.level = MillionaireGame::JsonUtils::extractInt(question_obj, "level", 0);
                            
                            if (q.questionId != -1) {
                                response.questions.push_back(q);
                            }
                            
                            pos = obj_end + 1;
                        } else {
                            break;
                        }
                    }
                    
                    // Skip comma / whitespace between objects
                    while (pos < msg.data.length() &&
                           (msg.data[pos] == ',' || msg.data[pos] == ' ' ||
                            msg.data[pos] == '\t' || msg.data[pos] == '\n')) {
                        pos++;
                    }
                }
            }
        }
    }
    
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

ProtocolHandler::QuestionDetail ProtocolHandler::getQuestionDetail(int questionId) {
    QuestionDetail detail;
    detail.responseCode = 500;
    detail.questionId = questionId;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["questionId"] = questionId;
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("GET_QUESTION", data)) {
        detail.responseCode = 503;
        return detail;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        detail.responseCode = 504;
        return detail;
    }
    
    detail.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (detail.responseCode != 200) {
        return detail;
    }
    
    // Parse fields
    detail.questionId = MillionaireGame::JsonUtils::extractInt(msg.data, "questionId", questionId);
    detail.question = MillionaireGame::JsonUtils::extractString(msg.data, "question");
    
    detail.options.clear();
    detail.options.push_back(MillionaireGame::JsonUtils::extractString(msg.data, "optionA"));
    detail.options.push_back(MillionaireGame::JsonUtils::extractString(msg.data, "optionB"));
    detail.options.push_back(MillionaireGame::JsonUtils::extractString(msg.data, "optionC"));
    detail.options.push_back(MillionaireGame::JsonUtils::extractString(msg.data, "optionD"));
    
    detail.correctAnswer = MillionaireGame::JsonUtils::extractInt(msg.data, "correctAnswer", 0);
    detail.level = MillionaireGame::JsonUtils::extractInt(msg.data, "level", 0);
    
    detail.lifeline_5050_info = MillionaireGame::JsonUtils::extractString(msg.data, "lifeline_5050_info");
    detail.lifeline_ask_info = MillionaireGame::JsonUtils::extractString(msg.data, "lifeline_ask_info");
    detail.lifeline_call_info = MillionaireGame::JsonUtils::extractString(msg.data, "lifeline_call_info");
    
    return detail;
}

ProtocolHandler::ViewUsersResponse ProtocolHandler::viewUsers(int page, int limit) {
    ViewUsersResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["page"] = page;
    ints["limit"] = limit;
    
    std::string data = buildDataJson(strings, ints);
    
    if (!client_->sendRequest("VIEW_USERS", data)) {
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
        response.total = MillionaireGame::JsonUtils::extractInt(msg.data, "total", 0);
        response.page = MillionaireGame::JsonUtils::extractInt(msg.data, "page", 1);
        response.limit = MillionaireGame::JsonUtils::extractInt(msg.data, "limit", 10);
        
        // Parse users array
        size_t users_start = msg.data.find("\"users\"");
        if (users_start != std::string::npos) {
            size_t array_start = msg.data.find("[", users_start);
            if (array_start != std::string::npos) {
                size_t pos = array_start + 1;
                while (pos < msg.data.length()) {
                    // Skip whitespace
                    while (pos < msg.data.length() && (msg.data[pos] == ' ' || msg.data[pos] == '\t' || msg.data[pos] == '\n')) pos++;
                    if (pos >= msg.data.length() || msg.data[pos] == ']') break;
                    
                    // Find object
                    if (msg.data[pos] == '{') {
                        size_t obj_end = msg.data.find("}", pos);
                        if (obj_end != std::string::npos) {
                            std::string user_obj = msg.data.substr(pos, obj_end - pos + 1);
                            
                            UserListEntry user;
                            user.username = MillionaireGame::JsonUtils::extractString(user_obj, "username");
                            user.role = MillionaireGame::JsonUtils::extractString(user_obj, "role");
                            user.isBanned = MillionaireGame::JsonUtils::extractBool(user_obj, "isBanned", false);
                            user.totalGames = MillionaireGame::JsonUtils::extractInt(user_obj, "totalGames", 0);
                            user.highestPrize = MillionaireGame::JsonUtils::extractLongLong(user_obj, "highestPrize", 0);
                            
                            response.users.push_back(user);
                            pos = obj_end + 1;
                        } else {
                            break;
                        }
                    }
                    
                    // Skip comma
                    while (pos < msg.data.length() && (msg.data[pos] == ',' || msg.data[pos] == ' ' || msg.data[pos] == '\t' || msg.data[pos] == '\n')) pos++;
                }
            }
        }
    }
    
    return response;
}

int ProtocolHandler::promoteUser(const std::string& username) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["username"] = username;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("PROMOTE_USER", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::revokeAdmin(const std::string& username) {
    std::map<std::string, std::string> strings;
    strings["authToken"] = authToken;
    strings["username"] = username;
    
    std::string data = buildDataJson(strings);
    
    if (!client_->sendRequest("REVOKE_ADMIN", data)) {
        return 503;
    }
    
    SocketClient::Message msg = waitForResponse(5000);
    if (msg.type == "TIMEOUT" || msg.data.empty()) {
        return 504;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}


