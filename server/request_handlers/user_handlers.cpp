#include "user_handlers.h"
#include "../auth_manager.h"
#include "../json_utils.h"
#include "../../database/database.h"
#include <sstream>

using namespace std;

namespace MillionaireGame {

namespace UserHandlers {

string handleUserInfo(const string& request, ClientSession& session) {
    string target_username = JsonUtils::extractString(request, "username");

    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }

    // Check if user exists
    if (!Database::getInstance().userExists(target_username)) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }
    
    // Get user info from leaderboard
    vector<LeaderboardEntry> entries = Database::getInstance().getLeaderboard("global", 1, 1, target_username);
    int total_games = 0;
    long long highest_prize = 0;
    int final_question_number = 0;
    long long total_score = 0;
    
    if (!entries.empty() && entries[0].username == target_username) {
        final_question_number = entries[0].final_question_number;
        total_score = entries[0].total_score;
        // Note: highest_prize and total_games would need additional query or leaderboard update
    }
    
    stringstream ss;
    ss << "{\"username\":\"" << target_username
       << "\",\"totalGames\":" << total_games
       << ",\"highestPrize\":" << highest_prize
       << ",\"finalQuestionNumber\":" << final_question_number
       << ",\"totalScore\":" << total_score << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleViewHistory(const string& request, ClientSession& session) {
    vector<GameSession> games = Database::getInstance().getGameHistory(session.username, 20);
    
    stringstream ss;
    ss << "{\"games\":[";
    for (size_t i = 0; i < games.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"gameId\":" << games[i].id
           << ",\"date\":\"" << (games[i].ended_at > 0 ? to_string(games[i].ended_at) : to_string(games[i].started_at)) << "\""
           << ",\"finalQuestionNumber\":" << games[i].current_question_number
           << ",\"totalScore\":" << games[i].total_score
           << ",\"finalPrize\":" << games[i].final_prize
           << ",\"status\":\"" << games[i].status << "\"}";
    }
    ss << "]}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleChangePass(const string& request, ClientSession& session) {
    string old_password = JsonUtils::extractString(request, "oldPassword");
    string new_password = JsonUtils::extractString(request, "newPassword");

    if (old_password.empty() || new_password.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing oldPassword or newPassword");
    }

    if (!AuthManager::getInstance().validatePasswordStrength(new_password)) {
        return StreamUtils::createErrorResponse(410, 
            "Password must be at least 8 characters and contain at least one uppercase letter, one lowercase letter, and one digit");
    }

    // Change password in database
    bool success = Database::getInstance().changePassword(session.username, old_password, new_password);
    if (!success) {
        return StreamUtils::createErrorResponse(401, "Wrong old password");
    }

    string data = "{\"message\":\"Password changed successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace UserHandlers

} // namespace MillionaireGame

