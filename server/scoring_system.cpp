#include "scoring_system.h"
#include <algorithm>

namespace MillionaireGame {

// Prize ladder matching the official "Who Wants to Be a Millionaire?" game
// These values match client/gamestate.cpp PRIZE_LADDER
static const long long PRIZE_LADDER[15] = {
    100,      // Q1: $100
    200,      // Q2: $200
    300,      // Q3: $300
    500,      // Q4: $500
    1000,     // Q5: $1,000 (checkpoint)
    2000,     // Q6: $2,000
    4000,     // Q7: $4,000
    8000,     // Q8: $8,000
    16000,    // Q9: $16,000
    32000,    // Q10: $32,000 (checkpoint)
    64000,    // Q11: $64,000
    125000,   // Q12: $125,000
    250000,   // Q13: $250,000
    500000,   // Q14: $500,000
    1000000   // Q15: $1,000,000 (checkpoint - $1 MILLION)
};

ScoringSystem& ScoringSystem::getInstance() {
    static ScoringSystem instance;
    return instance;
}

int ScoringSystem::calculateQuestionScore(int time_remaining, int lifelines_used) {
    // Score is simply the time remaining for this question
    // No penalty for lifelines used
    return std::max(0, time_remaining);
}

int ScoringSystem::calculateTotalScore(const std::vector<int>& question_scores) {
    int total = 0;
    for (int score : question_scores) {
        total += score;
    }
    return total;
}

long long ScoringSystem::getPrizeForLevel(int level, int question_number) {
    // Use the official prize ladder
    // Question numbers are 1-indexed (Q1 to Q15)
    
    if (question_number < 1 || question_number > 15) {
        return 0;
    }
    
    return PRIZE_LADDER[question_number - 1];
}

long long ScoringSystem::getSafeCheckpointPrize(int question_number) {
    // Safe checkpoints at questions 5, 10, 15
    // If player loses after checkpoint, they keep the checkpoint prize
    if (question_number >= 15) {
        return PRIZE_LADDER[14];  // $1,000,000 (Q15 checkpoint)
    } else if (question_number >= 10) {
        return PRIZE_LADDER[9];   // $32,000 (Q10 checkpoint)
    } else if (question_number >= 5) {
        return PRIZE_LADDER[4];   // $1,000 (Q5 checkpoint)
    } else {
        return 0;  // No safe checkpoint before question 5
    }
}

} // namespace MillionaireGame

