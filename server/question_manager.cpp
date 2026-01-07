#include "question_manager.h"
#include "../database/database.h"

namespace MillionaireGame {

QuestionManager& QuestionManager::getInstance() {
    static QuestionManager instance;
    return instance;
}

Question QuestionManager::getRandomQuestion(int level) {
    return Database::getInstance().getRandomQuestion(level);
}

bool QuestionManager::checkAnswer(int question_id, int answer_index) {
    Question question = Database::getInstance().getQuestion(question_id);
    if (question.id == 0) {
        return false;  // Question not found
    }
    return question.correct_answer == answer_index;
}

int QuestionManager::getCorrectAnswer(int question_id) {
    Question question = Database::getInstance().getQuestion(question_id);
    if (question.id == 0) {
        return -1;  // Question not found
    }
    return question.correct_answer;
}

} // namespace MillionaireGame

