#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "stream_handler.h"
#include "request_router.h"
#include "session_manager.h"
#include "config.h"
#include <string>
#include <memory>

namespace MillionaireGame {

/**
 * Client Connection Handler
 * Handles individual client connections in separate threads
 */
class ClientHandler {
public:
    /**
     * Handle client connection
     * Main loop for processing client requests
     */
    static void handleClient(int client_fd, const std::string& client_ip, const ServerConfig& config);

private:
    /**
     * Send connection message to client
     */
    static void sendConnectionMessage(StreamHandler* handler);
    
    /**
     * Update last ping time for session
     */
    static void updatePingTime(int client_fd);
    
    /**
     * Cleanup client session on disconnect
     */
    static void cleanupClient(int client_fd);
};

} // namespace MillionaireGame

#endif // CLIENT_HANDLER_H

