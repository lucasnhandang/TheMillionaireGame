#ifndef GAME_STATE_MANAGER_H
#define GAME_STATE_MANAGER_H

#include <string>
#include <atomic>

namespace MillionaireGame {

// Forward declarations
struct GameProgress {
    int level;
    int prize;
    GameProgress() : level(0), prize(0) {}
};

/**
 * Game State Manager
 * Manages game state operations (placeholder for future game logic integration)
 */
class GameStateManager {
public:
    static GameStateManager& getInstance();
    
    /**
     * Generate unique game ID
     */
    int generateGameId();
    
    /**
     * Load game progress for a user
     * TODO: Replace with database call
     */
    GameProgress loadGameProgress(const std::string& username);
    
    /**
     * Save game progress for a user
     * TODO: Replace with database call
     */
    void saveGameProgress(const std::string& username, int level, int prize);
    
    /**
     * Check if answer is correct
     * TODO: Replace with game logic
     */
    bool checkAnswer(int level, const std::string& answer);

private:
    GameStateManager() = default;
    ~GameStateManager() = default;
    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;
    
    std::atomic<int> game_id_counter_{1};
};

} // namespace MillionaireGame

#endif // GAME_STATE_MANAGER_H

