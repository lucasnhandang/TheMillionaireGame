#include "client_handler.h"
#include "logger.h"
#include "auth_manager.h"
#include <unistd.h>
#include <exception>

using namespace std;

namespace MillionaireGame {

void ClientHandler::handleClient(int client_fd, const string& client_ip, const ServerConfig& config) {
    auto handler = unique_ptr<StreamHandler>(new StreamHandler(client_fd));
    handler->setReadTimeout(config.connection_timeout_seconds, 0);
    handler->setWriteTimeout(10, 0);

    StreamHandler* handler_ptr = handler.get();

    SessionManager::getInstance().createSession(client_fd, std::move(handler), client_ip);

    LOG_INFO("Client handler started for " + client_ip);

    sendConnectionMessage(handler_ptr);

    RequestRouter router;
    
    try {
        while (handler_ptr->isConnected()) {
            string request = handler_ptr->readMessage(config.ping_timeout_seconds + 5);

            if (request.empty()) {
                if (!handler_ptr->isConnected()) {
                    LOG_INFO("Client " + client_ip + " disconnected");
                    break;
                }
                continue;
            }

            if (!StreamUtils::validateJsonFormat(request)) {
                string error = StreamUtils::createErrorResponse(400, "Invalid JSON format");
                handler_ptr->writeMessage(error);
                continue;
            }

            string response = router.processRequest(request, client_fd);
            if (!response.empty()) {
                handler_ptr->writeMessage(response);
            }

            updatePingTime(client_fd);
        }
    } catch (const exception& e) {
        LOG_ERROR("Exception in client handler: " + string(e.what()));
    }

    cleanupClient(client_fd);
    LOG_INFO("Client handler finished for " + client_ip);
    close(client_fd);
}

void ClientHandler::sendConnectionMessage(StreamHandler* handler) {
    string connection_msg = StreamUtils::createSuccessResponse(200, 
        "{\"message\":\"Connected to Millionaire Game Server\"}");
    handler->writeMessage(connection_msg);
}

void ClientHandler::updatePingTime(int client_fd) {
    SessionManager::getInstance().updatePingTime(client_fd);
}

void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        if (!session->auth_token.empty()) {
            AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
        }
        if (!session->username.empty()) {
            SessionManager::getInstance().removeOnlineUser(session->username);
        }
    }
    SessionManager::getInstance().removeSession(client_fd);
}

} // namespace MillionaireGame

