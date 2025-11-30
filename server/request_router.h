#ifndef REQUEST_ROUTER_H
#define REQUEST_ROUTER_H

#include "session_manager.h"
#include "auth_manager.h"
#include "stream_handler.h"
#include <string>

namespace MillionaireGame {

/**
 * Request Router
 * Routes incoming requests to appropriate handlers
 */
class RequestRouter {
public:
    RequestRouter() = default;
    
    /**
     * Process incoming request and return response
     * Handles authentication and routing to appropriate handler
     */
    std::string processRequest(const std::string& request, int client_fd);

private:
    /**
     * Check if request requires authentication
     */
    bool requiresAuth(const std::string& request_type);
};

} // namespace MillionaireGame

#endif // REQUEST_ROUTER_H

