#include "admin_handlers.h"
#include "../json_utils.h"
#include "../stream_handler.h"
#include "../notification_utils.h"
#include <ctime>
#include "../../database/database.h"
#include <sstream>
#include <algorithm>

using namespace std;

namespace MillionaireGame {

namespace AdminHandlers {

string handleAddQues(const string& request, ClientSession& session, int client_fd) {
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

    if (level < 0 || level > 2) {
        return StreamUtils::createErrorResponse(422, "Invalid level: must be 0-2 (0=easy, 1=medium, 2=hard)");
    }

    // Extract and validate options array
    vector<string> options = extractOptions(request);
    if (options[0].empty() || options[1].empty() || options[2].empty() || options[3].empty()) {
        return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 options with label and text");
    }

    // Create Question object
    Question q;
    q.question_text = question;
    q.option_a = options[0];
    q.option_b = options[1];
    q.option_c = options[2];
    q.option_d = options[3];
    q.correct_answer = correct_answer;
    q.level = level;
    q.is_active = true;
    q.updated_by = 0;  // Could get from session if needed

    // Add question to database
    int question_id = Database::getInstance().addQuestion(q);
    if (question_id == 0) {
        return StreamUtils::createErrorResponse(500, "Failed to add question");
    }

    // Placeholder question ID
    int question_id = 0;

    string data = "{\"message\":\"Question added successfully\",\"questionId\":" + to_string(question_id) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleChangeQues(const string& request, ClientSession& session, int client_fd) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int question_id = JsonUtils::extractInt(request, "questionId", -1);

    if (question_id < 0) {
        return StreamUtils::createErrorResponse(400, "Missing questionId");
    }

    // Check if question exists
    if (!Database::getInstance().questionExists(question_id)) {
        return StreamUtils::createErrorResponse(404, "Question not found");
    }

    // Get existing question
    Question q = Database::getInstance().getQuestion(question_id);
    if (q.id == 0) {
        return StreamUtils::createErrorResponse(404, "Question not found");
    }

    // Update fields if provided
    string new_question = JsonUtils::extractString(request, "question");
    if (!new_question.empty()) {
        q.question_text = new_question;
    }

    vector<string> options = extractOptions(request);
    if (!options[0].empty()) {
        q.option_a = options[0];
        q.option_b = options[1];
        q.option_c = options[2];
        q.option_d = options[3];
    }

    int correct_answer = JsonUtils::extractInt(request, "correctAnswer", -1);
    if (correct_answer != -1) {
        if (correct_answer < 0 || correct_answer > 3) {
            return StreamUtils::createErrorResponse(422, "Invalid correctAnswer: must be 0-3");
        }
        q.correct_answer = correct_answer;
    }

    // Update question in database
    bool success = Database::getInstance().updateQuestion(question_id, q);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to update question");
    }

    string data = "{\"message\":\"Question updated successfully\",\"questionId\":" + to_string(question_id) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleViewQues(const string& request, ClientSession& session, int client_fd) {
    if (session.role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int page = JsonUtils::extractInt(request, "page", 1);
    int limit = JsonUtils::extractInt(request, "limit", 20);
    int level = JsonUtils::extractInt(request, "level", -1);

    if (page < 1 || limit < 1) {
        return StreamUtils::createErrorResponse(422, "Page and limit must be positive");
    }

    if (level != -1 && (level < 0 || level > 2)) {
        return StreamUtils::createErrorResponse(422, "Invalid level: must be 0-2");
    }

    // Get questions from database
    vector<Question> questions = Database::getInstance().getQuestions(level == -1 ? 0 : level, page, limit);
    
    stringstream ss;
    ss << "{\"questions\":[";
    for (size_t i = 0; i < questions.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"questionId\":" << questions[i].id
           << ",\"question\":\"" << questions[i].question_text << "\""
           << ",\"level\":" << questions[i].level << "}";
    }
    ss << "],\"total\":" << questions.size()
       << ",\"page\":" << page << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleDelQues(const string& request, ClientSession& session, int client_fd) {
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

    string data = "{\"message\":\"Question deleted successfully\",\"questionId\":" + to_string(question_id) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleBanUser(const string& request, ClientSession& session, int client_fd) {
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

    string data = "{\"message\":\"User banned successfully\",\"username\":\"" + target_username + "\"}";
    
    // TODO: Send USER_BANNED notification to the banned user (force disconnect)
    // This requires finding the user's client_fd
    // int banned_user_fd = findClientFdByUsername(target_username);
    // if (banned_user_fd != -1) {
    //     string user_notification_data = "{\"reason\":\"" + reason +
    //                                    "\",\"timestamp\":" + to_string(time(nullptr)) + "}";
    //     NotificationUtils::sendNotification(banned_user_fd, "USER_BANNED", user_notification_data);
    // }
    
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace AdminHandlers

} // namespace MillionaireGame

