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
    
    // Get correct answer index
    int correct = question.correct_answer;
    
    // Create vector of incorrect options (0-3 excluding correct)
    std::vector<int> incorrect_options;
    for (int i = 0; i < 4; i++) {
        if (i != correct) {
            incorrect_options.push_back(i);
        }
    }
    
    // Randomly shuffle and pick 2 to remove
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(incorrect_options.begin(), incorrect_options.end(), gen);
    
    // Keep correct answer and one incorrect answer
    std::vector<int> remaining;
    remaining.push_back(correct);
    remaining.push_back(incorrect_options[0]);
    std::sort(remaining.begin(), remaining.end());
    
    // Build JSON result
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
    
    // Get correct answer
    int correct = question.correct_answer;
    
    // Friend suggests correct answer 70% of the time, random otherwise
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    int suggestion = correct;
    if (dis(gen) >= 70) {
        // 30% chance of wrong answer
        std::uniform_int_distribution<> wrong_dis(0, 3);
        do {
            suggestion = wrong_dis(gen);
        } while (suggestion == correct);
    }
    
    // Get option label (A, B, C, D)
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
    
    int correct = question.correct_answer;
    
    // Generate audience percentages
    // Correct answer gets highest percentage (40-60%)
    // Others get distributed remaining percentage
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
    
    // Distribute remaining percentage
    if (remaining > 0) {
        wrong_percents[0] += remaining / 3;
        wrong_percents[1] += remaining / 3;
        wrong_percents[2] += remaining % 3;
    }
    
    // Build JSON result
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

