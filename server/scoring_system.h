#ifndef SCORING_SYSTEM_H
#define SCORING_SYSTEM_H

#include <vector>

namespace MillionaireGame {

/**
 * Scoring System
 * Handles score calculation and prize determination
 */
class ScoringSystem {
public:
    static ScoringSystem& getInstance();
    
    /**
     * Calculate score for a question based on time remaining and lifelines used
     * @param time_remaining Time remaining in seconds
     * @param lifelines_used Number of lifelines used
     * @return Points earned for this question
     */
    int calculateQuestionScore(int time_remaining, int lifelines_used);
    
    /**
     * Calculate total score from individual question scores
     * @param question_scores Vector of scores for each question
     * @return Total score
     */
    int calculateTotalScore(const std::vector<int>& question_scores);
    
    /**
     * Get prize amount for a given level/question number
     * @param level Question level (0-2)
     * @param question_number Question number (1-15)
     * @return Prize amount
     */
    long long getPrizeForLevel(int level, int question_number);
    
    /**
     * Get safe checkpoint prize for a question number
     * @param question_number Question number (1-15)
     * @return Safe checkpoint prize amount
     */
    long long getSafeCheckpointPrize(int question_number);

private:
    ScoringSystem() = default;
    ~ScoringSystem() = default;
    ScoringSystem(const ScoringSystem&) = delete;
    ScoringSystem& operator=(const ScoringSystem&) = delete;
};

} // namespace MillionaireGame

#endif // SCORING_SYSTEM_H

