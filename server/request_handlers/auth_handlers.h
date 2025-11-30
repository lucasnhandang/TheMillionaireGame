#ifndef AUTH_HANDLERS_H
#define AUTH_HANDLERS_H

#include "../session_manager.h"
#include "../auth_manager.h"
#include "../json_utils.h"
#include "../stream_handler.h"
#include <string>

namespace MillionaireGame {

/**
 * Authentication request handlers
 * Handles LOGIN, REGISTER, LOGOUT requests
 */
namespace AuthHandlers {
    std::string handleLogin(const std::string& request, ClientSession& session, int client_fd);
    std::string handleRegister(const std::string& request, ClientSession& session, int client_fd);
    std::string handleLogout(const std::string& request, ClientSession& session, int client_fd);
}

} // namespace MillionaireGame

#endif // AUTH_HANDLERS_H

