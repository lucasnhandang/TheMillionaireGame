#ifndef QUESTION_MANAGER_H
#define QUESTION_MANAGER_H

#include "../database/database.h"
#include <string>

namespace MillionaireGame {

/**
 * Question Manager
 * Handles question retrieval and answer validation
 */
class QuestionManager {
public:
    static QuestionManager& getInstance();
    
    /**
     * Get random question for a given level
     * @param level Question level (0=easy, 1=medium, 2=hard)
     * @return Question object
     */
    Question getRandomQuestion(int level);
    
    /**
     * Check if answer is correct
     * @param question_id Question ID
     * @param answer_index Answer index (0-3)
     * @return true if correct, false otherwise
     */
    bool checkAnswer(int question_id, int answer_index);
    
    /**
     * Get correct answer for a question
     * @param question_id Question ID
     * @return Correct answer index (0-3)
     */
    int getCorrectAnswer(int question_id);

private:
    QuestionManager() = default;
    ~QuestionManager() = default;
    QuestionManager(const QuestionManager&) = delete;
    QuestionManager& operator=(const QuestionManager&) = delete;
};

} // namespace MillionaireGame

#endif // QUESTION_MANAGER_H

