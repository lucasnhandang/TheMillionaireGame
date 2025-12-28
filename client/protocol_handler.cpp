#include "protocol_handler.h"
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>

// Helper function to extract int from JSON (inline implementation)
static int extractIntFromJson(const std::string& json, const std::string& key, int default_value = 0) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return default_value;
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return default_value;
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) return default_value;
    
    size_t end = pos;
    while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
        end++;
    }
    
    if (end == pos) return default_value;
    
    try {
        return std::stoi(json.substr(pos, end - pos));
    } catch (...) {
        return default_value;
    }
}

ProtocolHandler::ProtocolHandler(SocketClient* socket) 
    : socket_(socket), auth_token_(""), username_(""), role_("user") {
    if (socket_) {
        socket_->setMessageCallback([this](const std::string& msg) {
            this->handleServerMessage(msg);
        });
    }
}

std::string ProtocolHandler::buildRequest(const std::string& requestType, const std::string& data) {
    std::ostringstream json;
    json << "{\"requestType\":\"" << requestType << "\",\"data\":{" << data << "}}";
    return json.str();
}

std::string ProtocolHandler::buildDataWithAuth(const std::string& additionalData) {
    std::ostringstream data;
    if (!auth_token_.empty()) {
        data << "\"authToken\":\"" << auth_token_ << "\"";
        if (!additionalData.empty()) {
            data << "," << additionalData;
        }
    } else if (!additionalData.empty()) {
        data << additionalData;
    }
    return data.str();
}

bool ProtocolHandler::sendRequest(const std::string& requestType, const std::string& data) {
    if (!socket_ || !socket_->isConnected()) {
        return false;
    }
    
    std::string request = buildRequest(requestType, data);
    return socket_->sendMessage(request);
}

void ProtocolHandler::parseResponse(const std::string& jsonResponse) {
    // Extract responseCode
    int responseCode = extractIntFromJson(jsonResponse, "responseCode");
    
    // Extract data field if exists
    std::string data = "";
    size_t dataPos = jsonResponse.find("\"data\":");
    if (dataPos != std::string::npos) {
        size_t start = jsonResponse.find("{", dataPos);
        if (start != std::string::npos) {
            int depth = 0;
            size_t end = start;
            for (size_t i = start; i < jsonResponse.length(); i++) {
                if (jsonResponse[i] == '{') depth++;
                if (jsonResponse[i] == '}') depth--;
                if (depth == 0) {
                    end = i;
                    break;
                }
            }
            if (end > start) {
                data = jsonResponse.substr(start, end - start + 1);
            }
        }
    }
    
    if (response_callback_) {
        response_callback_(responseCode, data);
    }
}

void ProtocolHandler::handleServerMessage(const std::string& jsonMessage) {
    parseResponse(jsonMessage);
}

void ProtocolHandler::setResponseCallback(std::function<void(int responseCode, const std::string& jsonData)> callback) {
    response_callback_ = callback;
}

bool ProtocolHandler::login(const std::string& username, const std::string& password) {
    std::ostringstream data;
    data << "\"username\":\"" << username << "\",\"password\":\"" << password << "\"";
    return sendRequest("LOGIN", data.str());
}

bool ProtocolHandler::registerUser(const std::string& username, const std::string& password) {
    std::ostringstream data;
    data << "\"username\":\"" << username << "\",\"password\":\"" << password << "\"";
    return sendRequest("REGISTER", data.str());
}

bool ProtocolHandler::logout() {
    std::string data = buildDataWithAuth("");
    return sendRequest("LOGOUT", data);
}

bool ProtocolHandler::startGame(bool overrideSavedGame) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (overrideSavedGame) {
        if (!auth_token_.empty()) data << ",";
        data << "\"overrideSavedGame\":true";
    }
    return sendRequest("START", data.str());
}

bool ProtocolHandler::answerQuestion(int gameId, int questionNumber, int answerIndex) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"gameId\":" << gameId 
         << ",\"questionNumber\":" << questionNumber 
         << ",\"answerIndex\":" << answerIndex;
    return sendRequest("ANSWER", data.str());
}

bool ProtocolHandler::useLifeline(int gameId, int questionNumber, const std::string& lifelineType) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"gameId\":" << gameId 
         << ",\"questionNumber\":" << questionNumber 
         << ",\"lifelineType\":\"" << lifelineType << "\"";
    return sendRequest("LIFELINE", data.str());
}

bool ProtocolHandler::giveUp(int gameId, int questionNumber) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"gameId\":" << gameId 
         << ",\"questionNumber\":" << questionNumber;
    return sendRequest("GIVE_UP", data.str());
}

bool ProtocolHandler::resumeGame() {
    std::string data = buildDataWithAuth("");
    return sendRequest("RESUME", data);
}

bool ProtocolHandler::leaveGame() {
    std::string data = buildDataWithAuth("");
    return sendRequest("LEAVE_GAME", data);
}

bool ProtocolHandler::getLeaderboard(const std::string& type, int page, int limit) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"type\":\"" << type << "\""
         << ",\"page\":" << page 
         << ",\"limit\":" << limit;
    return sendRequest("LEADERBOARD", data.str());
}

bool ProtocolHandler::getFriendStatus() {
    std::string data = buildDataWithAuth("");
    return sendRequest("FRIEND_STATUS", data);
}

bool ProtocolHandler::addFriend(const std::string& username) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"friendUsername\":\"" << username << "\"";
    return sendRequest("ADD_FRIEND", data.str());
}

bool ProtocolHandler::acceptFriend(const std::string& username) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"friendUsername\":\"" << username << "\"";
    return sendRequest("ACCEPT_FRIEND", data.str());
}

bool ProtocolHandler::declineFriend(const std::string& username) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"friendUsername\":\"" << username << "\"";
    return sendRequest("DECLINE_FRIEND", data.str());
}

bool ProtocolHandler::getFriendRequestList() {
    std::string data = buildDataWithAuth("");
    return sendRequest("FRIEND_REQ_LIST", data);
}

bool ProtocolHandler::deleteFriend(const std::string& username) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"friendUsername\":\"" << username << "\"";
    return sendRequest("DEL_FRIEND", data.str());
}

bool ProtocolHandler::sendChat(const std::string& recipient, const std::string& message) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"recipient\":\"" << recipient << "\""
         << ",\"message\":\"" << message << "\"";
    return sendRequest("CHAT", data.str());
}

bool ProtocolHandler::getUserInfo(const std::string& username) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"username\":\"" << username << "\"";
    return sendRequest("USER_INFO", data.str());
}

bool ProtocolHandler::getGameHistory() {
    std::string data = buildDataWithAuth("");
    return sendRequest("VIEW_HISTORY", data);
}

bool ProtocolHandler::changePassword(const std::string& oldPassword, const std::string& newPassword) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"oldPassword\":\"" << oldPassword << "\""
         << ",\"newPassword\":\"" << newPassword << "\"";
    return sendRequest("CHANGE_PASS", data.str());
}

bool ProtocolHandler::ping() {
    std::string data = buildDataWithAuth("");
    return sendRequest("PING", data);
}

// Admin functions
bool ProtocolHandler::addQuestion(const std::string& question, 
                                  const std::vector<std::pair<std::string, std::string>>& options, 
                                  int correctAnswer, int level) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"question\":\"" << question << "\",\"options\":[";
    for (size_t i = 0; i < options.size(); i++) {
        if (i > 0) data << ",";
        data << "{\"label\":\"" << options[i].first << "\",\"text\":\"" << options[i].second << "\"}";
    }
    data << "],\"correctAnswer\":" << correctAnswer << ",\"level\":" << level;
    return sendRequest("ADD_QUES", data.str());
}

bool ProtocolHandler::changeQuestion(int questionId, const std::string& question, 
                                     const std::vector<std::pair<std::string, std::string>>& options, 
                                     int correctAnswer) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"questionId\":" << questionId 
         << ",\"question\":\"" << question << "\",\"options\":[";
    for (size_t i = 0; i < options.size(); i++) {
        if (i > 0) data << ",";
        data << "{\"label\":\"" << options[i].first << "\",\"text\":\"" << options[i].second << "\"}";
    }
    data << "],\"correctAnswer\":" << correctAnswer;
    return sendRequest("CHANGE_QUES", data.str());
}

bool ProtocolHandler::viewQuestions(int page, int limit, int level) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"page\":" << page << ",\"limit\":" << limit;
    if (level > 0) {
        data << ",\"level\":" << level;
    }
    return sendRequest("VIEW_QUES", data.str());
}

bool ProtocolHandler::deleteQuestion(int questionId) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"questionId\":" << questionId;
    return sendRequest("DEL_QUES", data.str());
}

bool ProtocolHandler::banUser(const std::string& username, const std::string& reason) {
    std::ostringstream data;
    data << buildDataWithAuth("");
    if (!auth_token_.empty()) data << ",";
    data << "\"username\":\"" << username << "\""
         << ",\"reason\":\"" << reason << "\"";
    return sendRequest("BAN_USER", data.str());
}

void ProtocolHandler::setAuthToken(const std::string& token) {
    auth_token_ = token;
}

std::string ProtocolHandler::getAuthToken() const {
    return auth_token_;
}

void ProtocolHandler::setUsername(const std::string& username) {
    username_ = username;
}

std::string ProtocolHandler::getUsername() const {
    return username_;
}

void ProtocolHandler::setRole(const std::string& role) {
    role_ = role;
}

std::string ProtocolHandler::getRole() const {
    return role_;
}

bool ProtocolHandler::isAdmin() const {
    return role_ == "admin";
}

