#include "request_router.h"
#include "request_handlers/auth_handlers.h"
#include "request_handlers/game_handlers.h"
#include "request_handlers/social_handlers.h"
#include "request_handlers/user_handlers.h"
#include "request_handlers/admin_handlers.h"
#include "request_handlers/connection_handlers.h"
#include "json_utils.h"

using namespace std;

namespace MillionaireGame {

string RequestRouter::processRequest(const string& request, int client_fd) {
    string request_type = StreamUtils::extractRequestType(request);

    if (request_type.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing requestType");
    }

    ClientSession* session = SessionManager::getInstance().getSession(client_fd);
    if (!session) {
        return StreamUtils::createErrorResponse(500, "Client session not found");
    }

    // Authentication requests (no auth required)
    if (request_type == "LOGIN") {
        return AuthHandlers::handleLogin(request, *session, client_fd);
    } else if (request_type == "REGISTER") {
        return AuthHandlers::handleRegister(request, *session, client_fd);
    } else if (request_type == "CONNECTION") {
        return ConnectionHandlers::handleConnection(request, *session);
    }
    
    // All other requests require authentication
    string username = AuthManager::getInstance().requireAuth(request, *session);
    if (username.empty()) {
        return StreamUtils::createErrorResponse(402, "Not authenticated or invalid authToken");
    }

    // Game actions
    if (request_type == "START") {
        return GameHandlers::handleStart(request, *session);
    } else if (request_type == "ANSWER") {
        return GameHandlers::handleAnswer(request, *session);
    } else if (request_type == "LIFELINE") {
        return GameHandlers::handleLifeline(request, *session);
    } else if (request_type == "GIVE_UP") {
        return GameHandlers::handleGiveUp(request, *session);
    } else if (request_type == "RESUME") {
        return GameHandlers::handleResume(request, *session);
    } else if (request_type == "LEAVE_GAME") {
        return GameHandlers::handleLeaveGame(request, *session);
    } else if (request_type == "LOGOUT") {
        return AuthHandlers::handleLogout(request, *session, client_fd);
    } else if (request_type == "PING") {
        return ConnectionHandlers::handlePing(request, *session);
    } else if (request_type == "LEADERBOARD") {
        return SocialHandlers::handleLeaderboard(request, *session);
    } else if (request_type == "FRIEND_STATUS") {
        return SocialHandlers::handleFriendStatus(request, *session);
    } else if (request_type == "ADD_FRIEND") {
        return SocialHandlers::handleAddFriend(request, *session);
    } else if (request_type == "ACCEPT_FRIEND") {
        return SocialHandlers::handleAcceptFriend(request, *session);
    } else if (request_type == "DECLINE_FRIEND") {
        return SocialHandlers::handleDeclineFriend(request, *session);
    } else if (request_type == "FRIEND_REQ_LIST") {
        return SocialHandlers::handleFriendReqList(request, *session);
    } else if (request_type == "DEL_FRIEND") {
        return SocialHandlers::handleDelFriend(request, *session);
    } else if (request_type == "CHAT") {
        return SocialHandlers::handleChat(request, *session);
    } else if (request_type == "USER_INFO") {
        return UserHandlers::handleUserInfo(request, *session);
    } else if (request_type == "VIEW_HISTORY") {
        return UserHandlers::handleViewHistory(request, *session);
    } else if (request_type == "CHANGE_PASS") {
        return UserHandlers::handleChangePass(request, *session);
    } else if (request_type == "ADD_QUES") {
        return AdminHandlers::handleAddQues(request, *session);
    } else if (request_type == "CHANGE_QUES") {
        return AdminHandlers::handleChangeQues(request, *session);
    } else if (request_type == "VIEW_QUES") {
        return AdminHandlers::handleViewQues(request, *session);
    } else if (request_type == "DEL_QUES") {
        return AdminHandlers::handleDelQues(request, *session);
    } else if (request_type == "BAN_USER") {
        return AdminHandlers::handleBanUser(request, *session);
    } else {
        return StreamUtils::createErrorResponse(415, "Unknown request type");
    }
}

bool RequestRouter::requiresAuth(const string& request_type) {
    return request_type != "LOGIN" && request_type != "REGISTER" && request_type != "CONNECTION";
}

} // namespace MillionaireGame

