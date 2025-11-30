#include "user_handlers.h"
#include "../auth_manager.h"
#include "../json_utils.h"

using namespace std;

namespace MillionaireGame {

namespace UserHandlers {

string handleUserInfo(const string& request, ClientSession& session) {
    string target_username = JsonUtils::extractString(request, "username");

    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }

    // TODO: Replace with database call
    // UserInfo info = Database::getInstance().getUserInfo(target_username);
    // if (info.username.empty()) {
    //     return StreamUtils::createErrorResponse(404, "User not found");
    // }

    // Placeholder response
    string data = "{\"username\":\"" + target_username + 
                 "\",\"totalGames\":0,\"highestPrize\":0,\"finalQuestionNumber\":0,\"totalScore\":0}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleViewHistory(const string& request, ClientSession& session) {
    // TODO: Replace with database call
    string data = "{\"games\":[]}";
    return StreamUtils::createSuccessResponse(200, data);
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

    // TODO: Replace with database call
    // bool success = Database::getInstance().changePassword(session.username, old_password, new_password);
    // if (!success) {
    //     return StreamUtils::createErrorResponse(401, "Wrong old password");
    // }

    string data = "{\"message\":\"Password changed successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace UserHandlers

} // namespace MillionaireGame

