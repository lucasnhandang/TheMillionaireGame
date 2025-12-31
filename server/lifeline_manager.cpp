#include "lifeline_manager.h"
#include "../database/database.h"
#include "session_manager.h"
#include <random>
#include <vector>
#include <algorithm>
#include <sstream>

namespace MillionaireGame {

LifelineManager& LifelineManager::getInstance() {
    static LifelineManager instance;
    return instance;
}

LifelineResult LifelineManager::use5050(int /* game_id */, int question_id) {
    LifelineResult result;
    result.lifeline_type = "5050";
    result.delay_seconds = 2;
    
    Question question = Database::getInstance().getQuestion(question_id);
    if (question.id == 0) {
        result.success = false;
        return result;
    }
    
    // Use database hint if available, otherwise fallback to random generation
    if (!question.lifeline_5050_info.empty()) {
        // Database contains JSON array of indices to keep, e.g., "[0,2]"
        std::stringstream ss;
        ss << "{\"remainingOptions\":" << question.lifeline_5050_info << "}";
        result.success = true;
        result.result_data = ss.str();
        return result;
    }
    
    // Fallback: Random generation (for backward compatibility)
    int correct = question.correct_answer;
    std::vector<int> incorrect_options;
    for (int i = 0; i < 4; i++) {
        if (i != correct) {
            incorrect_options.push_back(i);
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(incorrect_options.begin(), incorrect_options.end(), gen);
    
    std::vector<int> remaining;
    remaining.push_back(correct);
    remaining.push_back(incorrect_options[0]);
    std::sort(remaining.begin(), remaining.end());
    
    std::stringstream ss;
    ss << "{\"remainingOptions\":[";
    for (size_t i = 0; i < remaining.size(); i++) {
        if (i > 0) ss << ",";
        ss << remaining[i];
    }
    ss << "]}";
    
    result.success = true;
    result.result_data = ss.str();
    return result;
}

LifelineResult LifelineManager::usePhone(int /* game_id */, int question_id) {
    LifelineResult result;
    result.lifeline_type = "PHONE";
    result.delay_seconds = 5;
    
    Question question = Database::getInstance().getQuestion(question_id);
    if (question.id == 0) {
        result.success = false;
        return result;
    }
    
    // Use database hint if available
    if (!question.lifeline_call_info.empty()) {
        // Database contains message string, e.g., "I'm 85% sure it's A"
        // Extract suggestion index from message (look for letter A-D)
        std::string message = question.lifeline_call_info;
        int suggestion = -1;
        char label = '\0';
        
        // Find letter in message (A, B, C, or D)
        for (size_t i = 0; i < message.length(); i++) {
            if (message[i] >= 'A' && message[i] <= 'D') {
                label = message[i];
                suggestion = message[i] - 'A';
                break;
            }
        }
        
        // If no letter found, try to extract from "it's X" pattern
        if (suggestion == -1) {
            size_t pos = message.find("it's ");
            if (pos != std::string::npos && pos + 5 < message.length()) {
                char c = message[pos + 5];
                if (c >= 'A' && c <= 'D') {
                    label = c;
                    suggestion = c - 'A';
                }
            }
        }
        
        // Fallback: use correct answer if extraction failed
        if (suggestion == -1) {
            suggestion = question.correct_answer;
            label = 'A' + suggestion;
        }
        
        // Return message as-is (already formatted as "I'm X% sure it's Y")
        std::stringstream ss;
        ss << "{\"suggestion\":\"" << message << "\"}";
        result.success = true;
        result.result_data = ss.str();
        return result;
    }
    
    // Fallback: Random generation (for backward compatibility)
    int correct = question.correct_answer;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    int suggestion = correct;
    if (dis(gen) >= 70) {
        std::uniform_int_distribution<> wrong_dis(0, 3);
        do {
            suggestion = wrong_dis(gen);
        } while (suggestion == correct);
    }
    
    char label = 'A' + suggestion;
    
    std::stringstream ss;
    ss << "{\"suggestion\":" << suggestion << ",\"label\":\"" << label << "\",\"confidence\":\"";
    if (suggestion == correct) {
        ss << "I'm " << (dis(gen) % 30 + 70) << "% sure it's " << label;
    } else {
        ss << "I think it might be " << label << ", but I'm not certain";
    }
    ss << "\"}";
    
    result.success = true;
    result.result_data = ss.str();
    return result;
}

LifelineResult LifelineManager::useAudience(int /* game_id */, int question_id) {
    LifelineResult result;
    result.lifeline_type = "AUDIENCE";
    result.delay_seconds = 3;
    
    Question question = Database::getInstance().getQuestion(question_id);
    if (question.id == 0) {
        result.success = false;
        return result;
    }
    
    // Use database hint if available
    if (!question.lifeline_ask_info.empty()) {
        // Database contains JSON object with percentages, e.g., "{\"A\":65,\"B\":15,\"C\":10,\"D\":10}"
        std::stringstream ss;
        ss << "{\"poll\":" << question.lifeline_ask_info << "}";
        result.success = true;
        result.result_data = ss.str();
        return result;
    }
    
    // Fallback: Random generation (for backward compatibility)
    int correct = question.correct_answer;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> correct_dis(40, 60);
    std::uniform_int_distribution<> wrong_dis(5, 25);
    
    int correct_percent = correct_dis(gen);
    int remaining = 100 - correct_percent;
    
    std::vector<int> wrong_percents;
    for (int i = 0; i < 3; i++) {
        int percent = wrong_dis(gen);
        wrong_percents.push_back(percent);
        remaining -= percent;
    }
    
    if (remaining > 0) {
        wrong_percents[0] += remaining / 3;
        wrong_percents[1] += remaining / 3;
        wrong_percents[2] += remaining % 3;
    }
    
    std::stringstream ss;
    ss << "{\"percentages\":{";
    int wrong_idx = 0;
    for (int i = 0; i < 4; i++) {
        if (i > 0) ss << ",";
        char label = 'A' + i;
        int percent = (i == correct) ? correct_percent : wrong_percents[wrong_idx++];
        ss << "\"" << label << "\":" << percent;
    }
    ss << "}}";
    
    result.success = true;
    result.result_data = ss.str();
    return result;
}

bool LifelineManager::isLifelineUsed(int /* game_id */, const std::string& /* lifeline_type */) {
    // This method is called from handlers which have access to session
    // For now, return false - the check is done in handlers before calling lifeline methods
    // TODO: Could query database or add getSessionByGameId to SessionManager
    return false;
}

} // namespace MillionaireGame

