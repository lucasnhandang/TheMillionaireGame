#ifndef GAME_EVENT_H
#define GAME_EVENT_H

#include <string>
#include <queue>
#include <mutex>

// Event types that can occur in the game
enum GameEventType {
    EVENT_NONE,
    EVENT_GAME_START,
    EVENT_QUESTION_INFO,
    EVENT_GAME_END,
    EVENT_LIFELINE_INFO,
    EVENT_FRIEND_REQUEST,
    EVENT_FRIEND_ACCEPTED,
    EVENT_NEW_MESSAGE
};

// Game event structure
struct GameEvent {
    GameEventType type;
    std::string data;  // JSON data
    
    GameEvent() : type(EVENT_NONE), data("") {}
    GameEvent(GameEventType t, const std::string& d) : type(t), data(d) {}
};

// Thread-safe event queue
class GameEventQueue {
public:
    GameEventQueue() = default;
    
    // Push event (called from notification thread)
    void push(const GameEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(event);
    }
    
    // Pop event (called from main UI thread)
    // Returns true if event was popped, false if queue empty
    bool pop(GameEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        event = queue_.front();
        queue_.pop();
        return true;
    }
    
    // Check if queue has events (non-blocking)
    bool hasEvents() {
        std::lock_guard<std::mutex> lock(mutex_);
        return !queue_.empty();
    }
    
    // Get queue size (for debugging)
    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
private:
    std::queue<GameEvent> queue_;
    std::mutex mutex_;
};

#endif // GAME_EVENT_H

