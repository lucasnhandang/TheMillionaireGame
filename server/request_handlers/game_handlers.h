#ifndef GAME_HANDLERS_H
#define GAME_HANDLERS_H

#include "../session_manager.h"
#include "../json_utils.h"
#include <string>

namespace MillionaireGame {

/**
 * Game request handlers
 * Handles START, ANSWER, LIFELINE, GIVE_UP, RESUME, LEAVE_GAME requests
 */
namespace GameHandlers {
    std::string handleStart(const std::string& request, ClientSession& session, int client_fd);
    std::string handleAnswer(const std::string& request, ClientSession& session, int client_fd);
    std::string handleLifeline(const std::string& request, ClientSession& session, int client_fd);
    std::string handleGiveUp(const std::string& request, ClientSession& session, int client_fd);
    std::string handleResume(const std::string& request, ClientSession& session, int client_fd);
    std::string handleSaveGame(const std::string& request, ClientSession& session, int client_fd);
    std::string handleLeaveGame(const std::string& request, ClientSession& session, int client_fd);
}

} // namespace MillionaireGame

#endif // GAME_HANDLERS_H

