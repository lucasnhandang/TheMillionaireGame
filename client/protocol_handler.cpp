#include "protocol_handler.h"
#include <sstream>
#include <iostream>

ProtocolHandler::ProtocolHandler(SocketClient* client)
    : client_(client), currentGameId(0), currentQuestionNumber(0) {
}

std::string ProtocolHandler::buildDataJson(const std::map<std::string, std::string>& strings,
                                          const std::map<std::string, int>& ints,
                                          const std::map<std::string, bool>& bools) {
    return MillionaireGame::JsonUtils::buildJson(strings, ints, bools);
}

ProtocolHandler::LoginResponse ProtocolHandler::login(const std::string& username, const std::string& password) {
    LoginResponse response;
    response.responseCode = 500;
    
    std::map<std::string, std::string> data;
    data["username"] = username;
    data["password"] = password;
    
    std::string dataJson = buildDataJson(data);
    if (!client_->sendRequest("LOGIN", dataJson)) {
        return response;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    if (response.responseCode == 200) {
        // Extract data from nested "data" object
        size_t dataStart = msg.data.find("\"data\":{");
        if (dataStart != std::string::npos) {
            std::string dataStr = msg.data.substr(dataStart + 7);
            response.authToken = MillionaireGame::JsonUtils::extractString(dataStr, "authToken");
            response.username = MillionaireGame::JsonUtils::extractString(dataStr, "username");
            response.role = MillionaireGame::JsonUtils::extractString(dataStr, "role");
            response.message = MillionaireGame::JsonUtils::extractString(dataStr, "message");
            
            this->authToken = response.authToken;
            this->username = response.username;
            this->role = response.role;
        }
    } else {
        response.message = MillionaireGame::JsonUtils::extractString(msg.data, "message");
    }
    
    return response;
}

int ProtocolHandler::registerUser(const std::string& username, const std::string& password) {
    std::map<std::string, std::string> data;
    data["username"] = username;
    data["password"] = password;
    
    std::string dataJson = buildDataJson(data);
    if (!client_->sendRequest("REGISTER", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

bool ProtocolHandler::logout() {
    if (authToken.empty()) {
        return false;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::string dataJson = buildDataJson(data);
    if (!client_->sendRequest("LOGOUT", dataJson)) {
        return false;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type == "RESPONSE" && 
        MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500) == 200) {
        authToken.clear();
        username.clear();
        role.clear();
        return true;
    }
    
    return false;
}

int ProtocolHandler::startGame(bool overrideSavedGame) {
    if (authToken.empty()) {
        return 402;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::map<std::string, bool> bools;
    bools["overrideSavedGame"] = overrideSavedGame;
    
    std::string dataJson = buildDataJson(data, {}, bools);
    if (!client_->sendRequest("START", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::resumeGame() {
    if (authToken.empty()) {
        return 402;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::string dataJson = buildDataJson(data);
    if (!client_->sendRequest("RESUME", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (code == 200) {
        size_t dataStart = msg.data.find("\"data\":{");
        if (dataStart != std::string::npos) {
            std::string dataStr = msg.data.substr(dataStart + 7);
            currentGameId = MillionaireGame::JsonUtils::extractInt(dataStr, "gameId", 0);
            currentQuestionNumber = MillionaireGame::JsonUtils::extractInt(dataStr, "questionNumber", 0);
        }
    }
    
    return code;
}

ProtocolHandler::AnswerResponse ProtocolHandler::answerQuestion(int answerIndex) {
    AnswerResponse response;
    response.responseCode = 406;
    
    if (authToken.empty() || currentGameId == 0 || currentQuestionNumber == 0) {
        return response;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;
    ints["answerIndex"] = answerIndex;
    
    std::string dataJson = buildDataJson(data, ints);
    if (!client_->sendRequest("ANSWER", dataJson)) {
        response.responseCode = 500;
        return response;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        response.responseCode = 500;
        return response;
    }
    
    response.responseCode = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    
    if (response.responseCode == 200) {
        size_t dataStart = msg.data.find("\"data\":{");
        if (dataStart != std::string::npos) {
            std::string dataStr = msg.data.substr(dataStart + 7);
            response.correct = MillionaireGame::JsonUtils::extractBool(dataStr, "correct", false);
            response.questionNumber = MillionaireGame::JsonUtils::extractInt(dataStr, "questionNumber", 0);
            response.timeRemaining = MillionaireGame::JsonUtils::extractInt(dataStr, "timeRemaining", 0);
            response.pointsEarned = MillionaireGame::JsonUtils::extractInt(dataStr, "pointsEarned", 0);
            response.totalScore = MillionaireGame::JsonUtils::extractInt(dataStr, "totalScore", 0);
            response.currentPrize = MillionaireGame::JsonUtils::extractInt(dataStr, "currentPrize", 0);
            response.gameOver = MillionaireGame::JsonUtils::extractBool(dataStr, "gameOver", false);
            response.isWinner = MillionaireGame::JsonUtils::extractBool(dataStr, "isWinner", false);
            response.correctAnswer = MillionaireGame::JsonUtils::extractInt(dataStr, "correctAnswer", -1);
            response.finalPrize = MillionaireGame::JsonUtils::extractInt(dataStr, "finalPrize", 0);
            
            if (!response.gameOver) {
                currentQuestionNumber = response.questionNumber;
            } else {
                currentGameId = 0;
                currentQuestionNumber = 0;
            }
        }
    }
    
    return response;
}

int ProtocolHandler::useLifeline(const std::string& lifelineType) {
    if (authToken.empty() || currentGameId == 0 || currentQuestionNumber == 0) {
        return 406;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    data["lifelineType"] = lifelineType;
    
    std::map<std::string, int> ints;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;
    
    std::string dataJson = buildDataJson(data, ints);
    if (!client_->sendRequest("LIFELINE", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    return MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
}

int ProtocolHandler::giveUp() {
    if (authToken.empty() || currentGameId == 0 || currentQuestionNumber == 0) {
        return 406;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::map<std::string, int> ints;
    ints["gameId"] = currentGameId;
    ints["questionNumber"] = currentQuestionNumber;
    
    std::string dataJson = buildDataJson(data, ints);
    if (!client_->sendRequest("GIVE_UP", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (code == 200) {
        currentGameId = 0;
        currentQuestionNumber = 0;
    }
    
    return code;
}

int ProtocolHandler::leaveGame() {
    if (authToken.empty()) {
        return 402;
    }
    
    std::map<std::string, std::string> data;
    data["authToken"] = authToken;
    
    std::string dataJson = buildDataJson(data);
    if (!client_->sendRequest("LEAVE_GAME", dataJson)) {
        return 500;
    }
    
    SocketClient::Message msg = waitForResponse();
    if (msg.type != "RESPONSE") {
        return 500;
    }
    
    int code = MillionaireGame::JsonUtils::extractInt(msg.data, "responseCode", 500);
    if (code == 200) {
        currentGameId = 0;
        currentQuestionNumber = 0;
    }
    
    return code;
}

SocketClient::Message ProtocolHandler::waitForResponse(int timeoutMs) {
    SocketClient::Message msg;
    int waited = 0;
    int step = 100; // Check every 100ms
    
    while (waited < timeoutMs) {
        if (client_->getMessage(msg, step)) {
            return msg;
        }
        waited += step;
    }
    
    msg.type = "TIMEOUT";
    msg.data = "";
    return msg;
}

