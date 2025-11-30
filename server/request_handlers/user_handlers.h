#ifndef USER_HANDLERS_H
#define USER_HANDLERS_H

#include "../session_manager.h"
#include "../json_utils.h"
#include <string>

namespace MillionaireGame {

/**
 * User request handlers
 * Handles USER_INFO, VIEW_HISTORY, CHANGE_PASS
 */
namespace UserHandlers {
    std::string handleUserInfo(const std::string& request, ClientSession& session);
    std::string handleViewHistory(const std::string& request, ClientSession& session);
    std::string handleChangePass(const std::string& request, ClientSession& session);
}

} // namespace MillionaireGame

#endif // USER_HANDLERS_H

