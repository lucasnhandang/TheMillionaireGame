#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include "../protocol_handler.h"
#include <string>

namespace MillionaireGame {

/**
 * Login/Register window
 * Handles user authentication
 */
class LoginWindow {
public:
    LoginWindow(ProtocolHandler* protocol);
    
    /**
     * Show login window and handle authentication
     * @return true if login successful, false if user wants to exit
     */
    bool show();
    
    /**
     * Get auth token after successful login
     */
    std::string getAuthToken() const;
    
    /**
     * Get username after successful login
     */
    std::string getUsername() const;
    
    /**
     * Get user role after successful login
     */
    std::string getRole() const;

private:
    ProtocolHandler* protocol_;
    std::string auth_token_;
    std::string username_;
    std::string role_;
    
    void showLoginMenu();
    bool handleLogin();
    bool handleRegister();
    void clearScreen();
    void waitForEnter();
};

} // namespace MillionaireGame

#endif // LOGIN_WINDOW_H

