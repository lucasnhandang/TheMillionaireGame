#include "admin_handlers.h"
#include "../json_utils.h"

using namespace std;

namespace MillionaireGame {

namespace AdminHandlers {

string handleAddQues(const string& request, ClientSession& session) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    string question = JsonUtils::extractString(request, "question");
    int correct_answer = JsonUtils::extractInt(request, "correctAnswer", -1);
    int level = JsonUtils::extractInt(request, "level", -1);

    if (question.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing question");
    }

    if (correct_answer < 0 || correct_answer > 3) {
        return StreamUtils::createErrorResponse(422, "Invalid correctAnswer: must be 0-3");
    }

    if (level < 1 || level > 15) {
        return StreamUtils::createErrorResponse(422, "Invalid level: must be 1-15");
    }

    // TODO: Validate options array (must have exactly 4 options with label and text)
    // Extract and validate options array from request
    // if (options.size() != 4) {
    //     return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 options");
    // }
    // for (const auto& opt : options) {
    //     if (opt.label.empty() || opt.text.empty()) {
    //         return StreamUtils::createErrorResponse(422, "Invalid option: label and text required");
    //     }
    // }

    // TODO: Replace with database call
    // int question_id = Database::getInstance().addQuestion(question_data);
    // if (question_id == -1) {
    //     return StreamUtils::createErrorResponse(409, "Question ID conflict");
    // }

    string data = "{\"questionId\":0,\"message\":\"Question added successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleChangeQues(const string& request, ClientSession& session) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int question_id = JsonUtils::extractInt(request, "questionId", -1);

    if (question_id < 0) {
        return StreamUtils::createErrorResponse(400, "Missing questionId");
    }

    // TODO: Validate options array if provided (must have exactly 4 options)
    // string options_str = JsonUtils::extractString(request, "options");
    // if (!options_str.empty()) {
    //     // Parse and validate options array
    //     if (options.size() != 4) {
    //         return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 options");
    //     }
    // }

    // TODO: Replace with database call
    // bool exists = Database::getInstance().questionExists(question_id);
    // if (!exists) {
    //     return StreamUtils::createErrorResponse(404, "Question not found");
    // }
    // 
    // int correct_answer = JsonUtils::extractInt(request, "correctAnswer", -1);
    // if (correct_answer != -1 && (correct_answer < 0 || correct_answer > 3)) {
    //     return StreamUtils::createErrorResponse(422, "Invalid correctAnswer: must be 0-3");
    // }
    // 
    // Database::getInstance().updateQuestion(question_id, question_data);

    string data = "{\"message\":\"Question updated successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleViewQues(const string& request, ClientSession& session) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int page = JsonUtils::extractInt(request, "page", 1);
    int limit = JsonUtils::extractInt(request, "limit", 20);
    int level = JsonUtils::extractInt(request, "level", -1);

    if (page < 1 || limit < 1) {
        return StreamUtils::createErrorResponse(422, "Page and limit must be positive");
    }

    if (level != -1 && (level < 1 || level > 15)) {
        return StreamUtils::createErrorResponse(422, "Invalid level: must be 1-15");
    }

    // TODO: Replace with database call
    string data = "{\"questions\":[],\"total\":0,\"page\":" + to_string(page) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleDelQues(const string& request, ClientSession& session) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int question_id = JsonUtils::extractInt(request, "questionId", -1);

    if (question_id < 0) {
        return StreamUtils::createErrorResponse(400, "Missing questionId");
    }

    // TODO: Replace with database call
    // bool exists = Database::getInstance().questionExists(question_id);
    // if (!exists) {
    //     return StreamUtils::createErrorResponse(404, "Question not found");
    // }
    // 
    // Database::getInstance().deleteQuestion(question_id);

    string data = "{\"message\":\"Question deleted successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleBanUser(const string& request, ClientSession& session) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    string target_username = JsonUtils::extractString(request, "username");
    string reason = JsonUtils::extractString(request, "reason");

    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }

    if (reason.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing reason");
    }

    if (target_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot ban yourself");
    }

    // TODO: Replace with database call
    // bool user_exists = Database::getInstance().userExists(target_username);
    // if (!user_exists) {
    //     return StreamUtils::createErrorResponse(404, "User not found");
    // }
    // 
    // Database::getInstance().banUser(target_username, reason);

    string data = "{\"message\":\"User banned successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace AdminHandlers

} // namespace MillionaireGame

