#ifndef GAME_TIMER_H
#define GAME_TIMER_H

#include <unordered_map>
#include <mutex>
#include <ctime>

namespace MillionaireGame {

/**
 * Game Timer
 * Handles question timeout tracking
 */
class GameTimer {
public:
    static GameTimer& getInstance();
    
    /**
     * Start timer for a question
     * @param game_id Game session ID
     */
    void startQuestionTimer(int game_id);
    
    /**
     * Check if question has timed out
     * @param game_id Game session ID
     * @return true if timed out
     */
    bool isTimeout(int game_id);
    
    /**
     * Get remaining time for current question
     * @param game_id Game session ID
     * @return Remaining time in seconds, or -1 if timer not started
     */
    int getRemainingTime(int game_id);
    
    /**
     * Stop timer for a game
     * @param game_id Game session ID
     */
    void stopTimer(int game_id);

private:
    GameTimer() : question_timeout_seconds_(60) {}
    ~GameTimer() = default;
    GameTimer(const GameTimer&) = delete;
    GameTimer& operator=(const GameTimer&) = delete;
    
    std::mutex timers_mutex_;
    std::unordered_map<int, time_t> timer_start_times_;  // game_id -> start time
    int question_timeout_seconds_;  // Default 60 seconds per question
};

} // namespace MillionaireGame

#endif // GAME_TIMER_H

