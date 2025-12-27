#include "game_state_manager.h"

using namespace std;

namespace MillionaireGame {

GameStateManager& GameStateManager::getInstance() {
    static GameStateManager instance;
    return instance;
}

int GameStateManager::generateGameId() {
    return game_id_counter_.fetch_add(1);
}

GameProgress GameStateManager::loadGameProgress(const string& username) {
    // TODO: Replace with database call
    // return Database::getInstance().loadGameProgress(username);
    GameProgress progress;
    return progress;
}

void GameStateManager::saveGameProgress(const string& username, int level, int prize) {
    // TODO: Replace with database call
    // Database::getInstance().saveGameProgress(username, game_id, level, prize, score);
}

bool GameStateManager::checkAnswer(int level, const string& answer) {
    // TODO: Replace with game logic
    // return QuestionManager::getInstance().checkAnswer(question_id, answer_index);
    return true;  // Placeholder
}

} // namespace MillionaireGame

