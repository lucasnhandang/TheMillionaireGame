#include "social_handlers.h"
#include "../session_manager.h"
#include "../json_utils.h"
#include <vector>

using namespace std;

namespace MillionaireGame {

namespace SocialHandlers {

string handleLeaderboard(const string& request, ClientSession& session) {
    string type = JsonUtils::extractString(request, "type");
    int page = JsonUtils::extractInt(request, "page", 1);
    int limit = JsonUtils::extractInt(request, "limit", 20);

    if (type != "global" && type != "friend") {
        return StreamUtils::createErrorResponse(422, "Invalid type: must be 'global' or 'friend'");
    }

    if (page < 1 || limit < 1) {
        return StreamUtils::createErrorResponse(422, "Page and limit must be positive");
    }

    // TODO: Replace with database call
    string data = "{\"rankings\":[],\"total\":0,\"page\":" + to_string(page) + 
                 ",\"limit\":" + to_string(limit) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleFriendStatus(const string& request, ClientSession& session) {
    // TODO: Get friends list from database
    vector<string> friend_list;  // Placeholder
    
    // TODO: Check online status for each friend
    string data = "{\"friends\":[]}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAddFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    if (friend_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot add yourself as friend");
    }

    // TODO: Replace with database call
    // bool user_exists = Database::getInstance().userExists(friend_username);
    // if (!user_exists) {
    //     return StreamUtils::createErrorResponse(404, "Friend not found");
    // }
    // 
    // bool already_friends = Database::getInstance().areFriends(session.username, friend_username);
    // if (already_friends) {
    //     return StreamUtils::createErrorResponse(409, "Friend already exists");
    // }
    // 
    // bool request_exists = Database::getInstance().friendRequestExists(session.username, friend_username);
    // if (request_exists) {
    //     return StreamUtils::createErrorResponse(409, "Friend request already sent");
    // }
    // 
    // bool success = Database::getInstance().addFriendRequest(session.username, friend_username);

    string data = "{\"message\":\"Friend request sent successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // TODO: Replace with database call
    // bool request_exists = Database::getInstance().friendRequestExists(friend_username, session.username);
    // if (!request_exists) {
    //     return StreamUtils::createErrorResponse(404, "Friend request not found");
    // }
    // 
    // bool already_friends = Database::getInstance().areFriends(session.username, friend_username);
    // if (already_friends) {
    //     return StreamUtils::createErrorResponse(409, "Friend already exists");
    // }
    // 
    // bool success = Database::getInstance().acceptFriendRequest(friend_username, session.username);

    string data = "{\"message\":\"Friend request accepted successfully\",\"friendUsername\":\"" + 
                 friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleDeclineFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // TODO: Replace with database call
    // bool request_exists = Database::getInstance().friendRequestExists(friend_username, session.username);
    // if (!request_exists) {
    //     return StreamUtils::createErrorResponse(404, "Friend request not found");
    // }
    // 
    // bool success = Database::getInstance().declineFriendRequest(friend_username, session.username);

    string data = "{\"message\":\"Friend request declined successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleFriendReqList(const string& request, ClientSession& session) {
    // TODO: Replace with database call
    string data = "{\"friendRequests\":[]}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleDelFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // TODO: Replace with database call
    // bool are_friends = Database::getInstance().areFriends(session.username, friend_username);
    // if (!are_friends) {
    //     return StreamUtils::createErrorResponse(404, "Friend not found");
    // }
    // 
    // bool success = Database::getInstance().deleteFriend(session.username, friend_username);

    string data = "{\"message\":\"Friend removed successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleChat(const string& request, ClientSession& session) {
    string recipient = JsonUtils::extractString(request, "recipient");
    string message = JsonUtils::extractString(request, "message");

    if (recipient.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing recipient");
    }

    if (message.empty()) {
        return StreamUtils::createErrorResponse(422, "Invalid message format or empty message");
    }

    // TODO: Replace with database call
    // bool user_exists = Database::getInstance().userExists(recipient);
    // if (!user_exists) {
    //     return StreamUtils::createErrorResponse(404, "Recipient user not found");
    // }
    // 
    // // Send message to recipient if online, or store for later
    // sendChatMessage(recipient, session.username, message);

    string data = "{\"message\":\"Message sent successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace SocialHandlers

} // namespace MillionaireGame

