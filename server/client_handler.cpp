#include "client_handler.h"
#include "logger.h"
#include "auth_manager.h"
#include <unistd.h>
#include <exception>
#include <ctime>

using namespace std;

namespace MillionaireGame {

void ClientHandler::handleClient(int client_fd, const string& client_ip, const ServerConfig& config) {
    std::unique_ptr<StreamHandler> handler(new StreamHandler(client_fd));
    handler->setReadTimeout(config.connection_timeout_seconds, 0);
    handler->setWriteTimeout(10, 0);

    StreamHandler* handler_ptr = handler.get();

    SessionManager::getInstance().createSession(client_fd, move(handler), client_ip);

    LOG_INFO("Client handler started for " + client_ip);

    sendConnectionMessage(handler_ptr);

    RequestRouter router;
    
    try {
        while (handler_ptr->isConnected()) {
            LOG_INFO("Waiting for message from client " + client_ip);
            string request = handler_ptr->readMessage(config.ping_timeout_seconds + 5);

            if (request.empty()) {
                LOG_INFO("Empty message received from " + client_ip + " (timeout or disconnected)");
                if (!handler_ptr->isConnected()) {
                    LOG_INFO("Client " + client_ip + " disconnected");
                    break;
                }
                continue;
            }

            LOG_INFO("Received request from " + client_ip + ": " + request.substr(0, 100) + "...");

            if (!StreamUtils::validateJsonFormat(request)) {
                LOG_INFO("Invalid JSON format from " + client_ip);
                string error = StreamUtils::createErrorResponse(400, "Invalid JSON format");
                handler_ptr->writeMessage(error);
                continue;
            }

            LOG_INFO("Processing request from " + client_ip);
            string response = router.processRequest(request, client_fd);
            if (!response.empty()) {
                LOG_INFO("Sending response to " + client_ip + ": " + response.substr(0, 100) + "...");
                handler_ptr->writeMessage(response);
            } else {
                LOG_INFO("No response generated for " + client_ip);
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
    string connection_msg = StreamUtils::createNotification("CONNECTION", 
        "{\"serverName\":\"Millionaire Game Server\",\"timestamp\":" + to_string(time(nullptr)) + "}");
    LOG_INFO("Sending CONNECTION notification to client");
    handler->writeMessage(connection_msg);
    LOG_INFO("CONNECTION notification sent");
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

