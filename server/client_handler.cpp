#include "client_handler.h"
#include "logger.h"
#include "auth_manager.h"
#include "game_timer.h"
#include "scoring_system.h"
#include "notification_utils.h"
#include "../database/database.h"
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

    SessionManager::getInstance().createSession(client_fd, client_ip);

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
    string connection_msg = StreamUtils::createNotification("CONNECTION", 
        "{\"serverName\":\"Millionaire Game Server\",\"timestamp\":" + to_string(time(nullptr)) + "}");
    handler->writeMessage(connection_msg);
}

void ClientHandler::updatePingTime(int client_fd) {
    SessionManager::getInstance().updatePingTime(client_fd);
}

void ClientHandler::cleanupClient(int client_fd) {
    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (session) {
        // Check for active game and end it if user disconnects
        if (!session->username.empty()) {
            GameSession active_game = Database::getInstance().getActiveGameSession(session->username);
            if (active_game.id > 0) {
                // User was in a game - end it due to disconnect
                int game_id = active_game.id;
                GameTimer::getInstance().stopTimer(game_id);
                
                // Calculate final score and prize (use safe checkpoint)
                long long final_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(active_game.current_question_number);
                
                // End game in database
                bool end_success = Database::getInstance().endGame(game_id, "quit", active_game.total_score, final_prize);
                
                if (end_success) {
                    // Send GAME_END notification if database update succeeded
                    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                                          ",\"status\":\"quit\"" +
                                          ",\"finalLevel\":" + to_string(active_game.current_question_number) +
                                          ",\"finalQuestionNumber\":" + to_string(active_game.current_question_number) +
                                          ",\"safeCheckpointPrize\":" + to_string(final_prize) +
                                          ",\"safeCheckpointScore\":" + to_string(active_game.total_score) +
                                          ",\"finalPrize\":" + to_string(final_prize) +
                                          ",\"totalScore\":" + to_string(active_game.total_score) +
                                          ",\"isWinner\":false}";
                    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
                    
                    LOG_INFO("Ended game " + to_string(game_id) + " for disconnected user " + session->username);
                } else {
                    LOG_ERROR("Failed to end game " + to_string(game_id) + " for disconnected user " + session->username);
                }
            }
        }
        
        // Cleanup authentication
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

