#include "admin_handlers.h"
#include "../json_utils.h"
#include "../../database/database.h"
#include <sstream>
#include <algorithm>

using namespace std;

namespace MillionaireGame {

namespace AdminHandlers {

// Helper function to extract options array from JSON
vector<string> extractOptions(const string& json) {
    vector<string> options(4, "");
    size_t pos = json.find("\"options\"");
    if (pos == string::npos) return options;
    
    pos = json.find('[', pos);
    if (pos == string::npos) return options;
    pos++;
    
    // Extract 4 options
    for (int i = 0; i < 4; i++) {
        size_t label_pos = json.find("\"label\"", pos);
        if (label_pos == string::npos) break;
        
        size_t label_start = json.find('"', json.find(':', label_pos) + 1) + 1;
        size_t label_end = json.find('"', label_start);
        string label = json.substr(label_start, label_end - label_start);
        
        size_t text_pos = json.find("\"text\"", label_end);
        if (text_pos == string::npos) break;
        
        size_t text_start = json.find('"', json.find(':', text_pos) + 1) + 1;
        size_t text_end = json.find('"', text_start);
        string text = json.substr(text_start, text_end - text_start);
        
        // Map label to index: A=0, B=1, C=2, D=3
        int idx = label[0] - 'A';
        if (idx >= 0 && idx < 4) {
            options[idx] = text;
        }
        
        pos = text_end + 1;
        if (i < 3) {
            pos = json.find(',', pos);
            if (pos == string::npos) break;
            pos++;
        }
    }
    
    return options;
}

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

    string data = "{\"questionId\":" + to_string(question_id) + ",\"message\":\"Question added successfully\"}";
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

string handleDelQues(const string& request, ClientSession& session) {
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
    
    // Delete question (soft delete)
    bool success = Database::getInstance().deleteQuestion(question_id);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to delete question");
    }

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

    // Check if user exists
    if (!Database::getInstance().userExists(target_username)) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }
    
    // Ban user
    bool success = Database::getInstance().banUser(target_username, reason);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to ban user");
    }

    string data = "{\"message\":\"User banned successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace AdminHandlers

} // namespace MillionaireGame

