#ifndef CONNECTION_HANDLERS_H
#define CONNECTION_HANDLERS_H

#include "../session_manager.h"
#include <string>

namespace MillionaireGame {

/**
 * Connection request handlers
 * Handles PING, CONNECTION
 */
namespace ConnectionHandlers {
    std::string handlePing(const std::string& request, ClientSession& session);
    std::string handleConnection(const std::string& request, ClientSession& session);
}

} // namespace MillionaireGame

#endif // CONNECTION_HANDLERS_H

