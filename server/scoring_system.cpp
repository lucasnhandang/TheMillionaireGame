#include "scoring_system.h"
#include <algorithm>

namespace MillionaireGame {

ScoringSystem& ScoringSystem::getInstance() {
    static ScoringSystem instance;
    return instance;
}

int ScoringSystem::calculateQuestionScore(int time_remaining, int lifelines_used) {
    // Base score is time remaining (max 60 seconds)
    // Each lifeline used reduces score by 5 points
    int base_score = std::max(0, time_remaining);
    int lifeline_penalty = lifelines_used * 5;
    return std::max(0, base_score - lifeline_penalty);
}

int ScoringSystem::calculateTotalScore(const std::vector<int>& question_scores) {
    int total = 0;
    for (int score : question_scores) {
        total += score;
    }
    return total;
}

long long ScoringSystem::getPrizeForLevel(int level, int question_number) {
    // Prize progression: doubles each question
    // Question 1: 1,000,000
    // Question 2: 2,000,000
    // ...
    // Question 15: 1,000,000,000
    
    if (question_number < 1 || question_number > 15) {
        return 0;
    }
    
    long long base_prize = 1000000;  // 1 million
    long long prize = base_prize;
    
    for (int i = 1; i < question_number; i++) {
        prize *= 2;
    }
    
    return prize;
}

long long ScoringSystem::getSafeCheckpointPrize(int question_number) {
    // Safe checkpoints at questions 5, 10, 15
    if (question_number > 15) {
        return 1000000000;  // 1 billion (final prize)
    } else if (question_number > 10) {
        return 100000000;   // 100 million
    } else if (question_number > 5) {
        return 10000000;    // 10 million
    } else {
        return 0;  // No safe checkpoint before question 5
    }
}

} // namespace MillionaireGame

