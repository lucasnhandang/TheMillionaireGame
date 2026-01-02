#ifndef LEADERBOARD_WINDOW_H
#define LEADERBOARD_WINDOW_H

#include "../protocol_handler.h"
#include <string>

namespace MillionaireGame {

/**
 * Leaderboard window
 * Displays global and friend leaderboards
 */
class LeaderboardWindow {
public:
    LeaderboardWindow(ProtocolHandler* protocol, const std::string& authToken);
    
    /**
     * Show leaderboard window
     */
    void show();

private:
    ProtocolHandler* protocol_;
    std::string auth_token_;
    
    void showMenu();
    void showGlobalLeaderboard(int page = 1);
    void showFriendLeaderboard(int page = 1);
    void clearScreen();
    void waitForEnter();
};

} // namespace MillionaireGame

#endif // LEADERBOARD_WINDOW_H

