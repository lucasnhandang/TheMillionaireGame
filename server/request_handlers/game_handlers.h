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
    std::string handleStart(const std::string& request, ClientSession& session);
    std::string handleAnswer(const std::string& request, ClientSession& session);
    std::string handleLifeline(const std::string& request, ClientSession& session);
    std::string handleGiveUp(const std::string& request, ClientSession& session);
    std::string handleResume(const std::string& request, ClientSession& session);
    std::string handleLeaveGame(const std::string& request, ClientSession& session);
}

} // namespace MillionaireGame

#endif // GAME_HANDLERS_H

