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

// Helper function to extract options array from JSON
// Format: "options":["3","4","5","6"]
static vector<string> extractOptions(const string& json) {
    vector<string> options(4);
    
    // Look for "options" array in JSON
    size_t options_start = json.find("\"options\"");
    if (options_start == string::npos) {
        return options;  // Return empty options if not found
    }
    
    // Find the array start
    size_t array_start = json.find("[", options_start);
    if (array_start == string::npos) {
        return options;
    }
    
    // Extract each option string from the array
    size_t pos = array_start + 1;
    for (int i = 0; i < 4 && pos < json.length(); i++) {
        // Skip whitespace and commas
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ',')) {
            pos++;
        }
        if (pos >= json.length()) break;
        
        // Find next quoted string
        if (json[pos] != '"') break;
        pos++; // Skip opening quote
        
        size_t quote_end = json.find("\"", pos);
        if (quote_end == string::npos) break;
        
        options[i] = json.substr(pos, quote_end - pos);
        pos = quote_end + 1;
    }
    
    return options;
}

// Helper function to extract JSON value (string, array, or object)
// Returns the raw JSON value as string
static string extractJsonValue(const string& json, const string& key) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    if (pos >= json.length()) return "";
    
    size_t start = pos;
    
    // Determine the type and extract accordingly
    if (json[pos] == '"') {
        // String value - extract until closing quote
        pos++;
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        return json.substr(start, end - start + 1);
    } else if (json[pos] == '[') {
        // Array value - extract until matching closing bracket
        int bracket_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '[') bracket_count++;
            if (json[end] == ']') {
                bracket_count--;
                if (bracket_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else if (json[pos] == '{') {
        // Object value - extract until matching closing brace
        int brace_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '{') brace_count++;
            if (json[end] == '}') {
                brace_count--;
                if (brace_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else {
        // Number or boolean - extract until comma, }, or ]
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
            end++;
        }
        return json.substr(start, end - start);
    }
}

string handleAddQues(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
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

    // Extract and validate options array (simplified format: ["3","4","5","6"])
    vector<string> options = extractOptions(request);
    if (options[0].empty() || options[1].empty() || options[2].empty() || options[3].empty()) {
        return StreamUtils::createErrorResponse(422, "Invalid options array: must have 4 option strings");
    }

    // Extract and validate lifeline info (required)
    // lifeline_5050_info and lifeline_ask_info are JSONB (arrays/objects)
    // lifeline_call_info is TEXT (plain string)
    string lifeline_5050_info = extractJsonValue(request, "lifeline_5050_info");
    string lifeline_ask_info = extractJsonValue(request, "lifeline_ask_info");
    string lifeline_call_info = JsonUtils::extractString(request, "lifeline_call_info");  // Plain string, no quotes
    
    if (lifeline_5050_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_5050_info");
    }
    if (lifeline_ask_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_ask_info");
    }
    if (lifeline_call_info.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing lifeline_call_info");
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
    q.lifeline_5050_info = lifeline_5050_info;
    q.lifeline_ask_info = lifeline_ask_info;
    q.lifeline_call_info = lifeline_call_info;
    q.updated_by = 0;  // Could get from session if needed

    // Add question to database
    int question_id = Database::getInstance().addQuestion(q);
    if (question_id == 0) {
        return StreamUtils::createErrorResponse(500, "Failed to add question");
    }

    string data = "{\"message\":\"Question added successfully\",\"questionId\":" + to_string(question_id) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleChangeQues(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
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
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
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

    // Normalize level (-1 means 'all levels')
    int effective_level = (level == -1 ? 0 : level);
    
    // Get questions from database (current page) and total count for pagination
    vector<Question> questions = Database::getInstance().getQuestions(effective_level, page, limit);
    int total_questions = Database::getInstance().getQuestionCount(effective_level);
    
    stringstream ss;
    ss << "{\"questions\":[";
    for (size_t i = 0; i < questions.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"questionId\":" << questions[i].id
           << ",\"question\":\"" << questions[i].question_text << "\""
           << ",\"level\":" << questions[i].level << "}";
    }
    ss << "],\"total\":" << total_questions
       << ",\"page\":" << page << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleDelQues(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int question_id = JsonUtils::extractInt(request, "questionId", -1);

    if (question_id < 0) {
        return StreamUtils::createErrorResponse(400, "Missing questionId");
    }

    // Check if question exists
    bool exists = Database::getInstance().questionExists(question_id);
    if (!exists) {
        return StreamUtils::createErrorResponse(404, "Question not found");
    }
    
    // Delete the question from database
    bool deleted = Database::getInstance().deleteQuestion(question_id);
    if (!deleted) {
        return StreamUtils::createErrorResponse(500, "Failed to delete question");
    }

    string data = "{\"message\":\"Question deleted successfully\",\"questionId\":" + to_string(question_id) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleGetQuestion(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }
    
    int question_id = JsonUtils::extractInt(request, "questionId", -1);
    if (question_id < 0) {
        return StreamUtils::createErrorResponse(400, "Missing questionId");
    }
    
    Question q = Database::getInstance().getQuestion(question_id);
    if (q.id == 0 || !q.is_active) {
        return StreamUtils::createErrorResponse(404, "Question not found");
    }
    
    // Build JSON with full question data (options and lifelines)
    stringstream ss;
    ss << "{\"questionId\":" << q.id
       << ",\"question\":\"" << q.question_text << "\""
       << ",\"optionA\":\"" << q.option_a << "\""
       << ",\"optionB\":\"" << q.option_b << "\""
       << ",\"optionC\":\"" << q.option_c << "\""
       << ",\"optionD\":\"" << q.option_d << "\""
       << ",\"correctAnswer\":" << q.correct_answer
       << ",\"level\":" << q.level;
    
    if (!q.lifeline_5050_info.empty()) {
        ss << ",\"lifeline_5050_info\":" << q.lifeline_5050_info;
    }
    if (!q.lifeline_ask_info.empty()) {
        ss << ",\"lifeline_ask_info\":" << q.lifeline_ask_info;
    }
    if (!q.lifeline_call_info.empty()) {
        ss << ",\"lifeline_call_info\":\"" << q.lifeline_call_info << "\"";
    }
    
    ss << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleBanUser(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly (not cached session role)
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
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

    // Check if user exists
    bool user_exists = Database::getInstance().userExists(target_username);
    if (!user_exists) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }
    
    // Ban the user in database
    bool success = Database::getInstance().banUser(target_username, reason);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to ban user");
    }

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

string handleViewUsers(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    int page = JsonUtils::extractInt(request, "page", 1);
    int limit = JsonUtils::extractInt(request, "limit", 10);

    if (page < 1) {
        return StreamUtils::createErrorResponse(422, "Invalid page: must be positive");
    }
    if (limit < 1 || limit > 100) {
        return StreamUtils::createErrorResponse(422, "Invalid limit: must be between 1 and 100");
    }

    // Get users from database
    vector<User> users = Database::getInstance().getAllUsers(page, limit);
    int total_users = Database::getInstance().getTotalUserCount();

    // Build JSON array of users
    ostringstream users_json;
    users_json << "[";
    for (size_t i = 0; i < users.size(); i++) {
        if (i > 0) users_json << ",";
        users_json << "{"
                   << "\"username\":\"" << users[i].username << "\","
                   << "\"role\":\"" << users[i].role << "\","
                   << "\"isBanned\":" << (users[i].is_banned ? "true" : "false") << ","
                   << "\"totalGames\":" << users[i].total_games << ","
                   << "\"highestPrize\":" << users[i].highest_prize
                   << "}";
    }
    users_json << "]";

    string data = "{\"users\":" + users_json.str() +
                  ",\"total\":" + to_string(total_users) +
                  ",\"page\":" + to_string(page) +
                  ",\"limit\":" + to_string(limit) + "}";

    return StreamUtils::createSuccessResponse(200, data);
}

string handlePromoteUser(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    string target_username = JsonUtils::extractString(request, "username");
    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }

    // Cannot promote yourself (already admin)
    if (target_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot promote yourself");
    }

    // Check if user exists
    if (!Database::getInstance().userExists(target_username)) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }

    // Check if user is already admin
    string current_role = Database::getInstance().getUserRole(target_username);
    if (current_role == "admin") {
        return StreamUtils::createErrorResponse(409, "User is already an admin");
    }

    // Promote user to admin
    bool success = Database::getInstance().updateUserRole(target_username, "admin");
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to promote user");
    }

    string data = "{\"message\":\"User promoted to admin successfully\",\"username\":\"" + target_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleRevokeAdmin(const string& request, ClientSession& session, int client_fd) {
    // Check database role directly
    string user_role = Database::getInstance().getUserRole(session.username);
    if (user_role != "admin") {
        return StreamUtils::createErrorResponse(403, "Access forbidden - not an admin account");
    }

    string target_username = JsonUtils::extractString(request, "username");
    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }

    // Cannot revoke your own admin rights
    if (target_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot revoke your own admin rights");
    }

    // Check if user exists
    if (!Database::getInstance().userExists(target_username)) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }

    // Check if user is actually an admin
    string current_role = Database::getInstance().getUserRole(target_username);
    if (current_role != "admin") {
        return StreamUtils::createErrorResponse(409, "User is not an admin");
    }

    // Revoke admin rights (set role to "user")
    bool success = Database::getInstance().updateUserRole(target_username, "user");
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to revoke admin rights");
    }

    string data = "{\"message\":\"Admin rights revoked successfully\",\"username\":\"" + target_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace AdminHandlers

} // namespace MillionaireGame

