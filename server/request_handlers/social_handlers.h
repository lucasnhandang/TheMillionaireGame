#ifndef SOCIAL_HANDLERS_H
#define SOCIAL_HANDLERS_H

#include "../session_manager.h"
#include "../json_utils.h"
#include <string>

namespace MillionaireGame {

/**
 * Social request handlers
 * Handles LEADERBOARD, FRIEND_STATUS, ADD_FRIEND, ACCEPT_FRIEND, DECLINE_FRIEND, FRIEND_REQ_LIST, DEL_FRIEND, CHAT
 */
namespace SocialHandlers {
    std::string handleLeaderboard(const std::string& request, ClientSession& session);
    std::string handleFriendStatus(const std::string& request, ClientSession& session);
    std::string handleAddFriend(const std::string& request, ClientSession& session);
    std::string handleAcceptFriend(const std::string& request, ClientSession& session);
    std::string handleDeclineFriend(const std::string& request, ClientSession& session);
    std::string handleFriendReqList(const std::string& request, ClientSession& session);
    std::string handleDelFriend(const std::string& request, ClientSession& session);
    std::string handleChat(const std::string& request, ClientSession& session);
}

} // namespace MillionaireGame

#endif // SOCIAL_HANDLERS_H

