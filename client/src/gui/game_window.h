#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include "../protocol_handler.h"
#include <string>
#include <thread>
#include <atomic>

namespace MillionaireGame {

/**
 * Game window - main gameplay interface
 * Displays questions, handles answers, lifelines, etc.
 */
class GameWindow {
public:
    GameWindow(ProtocolHandler* protocol, const std::string& authToken);
    
    /**
     * Show game window and handle gameplay
     */
    void show();

private:
    ProtocolHandler* protocol_;
    std::string auth_token_;
    int current_game_id_;
    int current_question_number_;
    std::atomic<bool> game_active_;
    std::atomic<bool> waiting_for_question_;
    
    void displayQuestion(const ProtocolHandler::QuestionInfo& question);
    void displayLifelineInfo(const ProtocolHandler::LifelineInfo& lifeline);
    void handleAnswer();
    void handleLifeline();
    void handleGiveUp();
    void handleGameEnd(const ProtocolHandler::GameEndInfo& gameEnd);
    void clearScreen();
    void formatPrize(long long prize);
    void waitForEnter();
    void startNotificationListener();
    void stopNotificationListener();
    void onNotification(const std::string& message);
};

} // namespace MillionaireGame

#endif // GAME_WINDOW_H

