#include "game_state_manager.h"
#include "../database/database.h"
#include "question_manager.h"

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
    // Load from database saved_games table
    // For now, return empty progress (would need to query saved_games)
    GameProgress progress;
    return progress;
}

void GameStateManager::saveGameProgress(const string& username, int level, int prize) {
    // Save to database saved_games table
    // Would need game_id and score, which aren't passed here
    // This is called from handleLeaveGame, so we'd need to get game_id from session
}

bool GameStateManager::checkAnswer(int level, const string& answer) {
    // This method signature is wrong - it needs question_id, not level
    // For backward compatibility, we'll parse answer_index from string
    try {
        int answer_index = stoi(answer);
        // Note: This is a placeholder - we need question_id to check answer properly
        // The actual check should be done in game handlers with question_id
        return answer_index >= 0 && answer_index <= 3;
    } catch (...) {
        return false;
    }
}

} // namespace MillionaireGame

