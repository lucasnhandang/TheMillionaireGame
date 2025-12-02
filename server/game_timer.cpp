#include "game_timer.h"
#include <ctime>
#include <algorithm>

namespace MillionaireGame {

GameTimer& GameTimer::getInstance() {
    static GameTimer instance;
    return instance;
}

void GameTimer::startQuestionTimer(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    timer_start_times_[game_id] = time(nullptr);
}

bool GameTimer::isTimeout(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    auto it = timer_start_times_.find(game_id);
    if (it == timer_start_times_.end()) {
        return false;  // Timer not started
    }
    
    time_t elapsed = time(nullptr) - it->second;
    return elapsed >= question_timeout_seconds_;
}

int GameTimer::getRemainingTime(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    auto it = timer_start_times_.find(game_id);
    if (it == timer_start_times_.end()) {
        return -1;  // Timer not started
    }
    
    time_t elapsed = time(nullptr) - it->second;
    int remaining = question_timeout_seconds_ - static_cast<int>(elapsed);
    return std::max(0, remaining);
}

void GameTimer::stopTimer(int game_id) {
    std::lock_guard<std::mutex> lock(timers_mutex_);
    timer_start_times_.erase(game_id);
}

} // namespace MillionaireGame

