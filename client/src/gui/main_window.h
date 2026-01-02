#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../protocol_handler.h"
#include <string>

namespace MillionaireGame {

/**
 * Main menu window
 * Shows main menu options after login
 */
class MainWindow {
public:
    MainWindow(ProtocolHandler* protocol, const std::string& authToken, const std::string& username, const std::string& role);
    
    /**
     * Show main menu and handle user choices
     * @return true to continue, false to logout
     */
    bool show();

private:
    ProtocolHandler* protocol_;
    std::string auth_token_;
    std::string username_;
    std::string role_;
    
    void showMenu();
    void handleStartGame();
    void handleResumeGame();
    void handleLeaderboard();
    void handleUserInfo();
    void handleViewHistory();
    void handleChangePassword();
    void handleFriends();
    void clearScreen();
    void waitForEnter();
    void formatPrize(long long prize);
};

} // namespace MillionaireGame

#endif // MAIN_WINDOW_H

