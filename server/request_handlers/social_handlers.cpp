#include "social_handlers.h"
#include "../session_manager.h"
#include "../json_utils.h"
#include "../../database/database.h"
#include <vector>
#include <sstream>

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

    // Get leaderboard from database
    vector<LeaderboardEntry> entries = Database::getInstance().getLeaderboard(
        type, page, limit, session.username);
    
    stringstream ss;
    ss << "{\"rankings\":[";
    for (size_t i = 0; i < entries.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"username\":\"" << entries[i].username << "\""
           << ",\"finalQuestionNumber\":" << entries[i].final_question_number
           << ",\"totalScore\":" << entries[i].total_score
           << ",\"rank\":" << entries[i].rank
           << ",\"isWinner\":" << (entries[i].is_winner ? "true" : "false") << "}";
    }
    ss << "],\"total\":" << entries.size()
       << ",\"page\":" << page
       << ",\"limit\":" << limit << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleFriendStatus(const string& request, ClientSession& session) {
    // Get friends list from database
    vector<string> friend_list = Database::getInstance().getFriendsList(session.username);
    
    stringstream ss;
    ss << "{\"friends\":[";
    for (size_t i = 0; i < friend_list.size(); i++) {
        if (i > 0) ss << ",";
        string status = "offline";
        if (SessionManager::getInstance().isUserOnline(friend_list[i])) {
            // Check if friend is in game
            status = "online";  // Simplified - could check game status
        }
        ss << "{\"username\":\"" << friend_list[i] << "\",\"status\":\"" << status << "\"}";
    }
    ss << "]}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleAddFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    if (friend_username == session.username) {
        return StreamUtils::createErrorResponse(422, "Cannot add yourself as friend");
    }

    // Check if user exists
    if (!Database::getInstance().userExists(friend_username)) {
        return StreamUtils::createErrorResponse(404, "Friend not found");
    }
    
    // Check if already friends
    if (Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(409, "Friend already exists");
    }
    
    // Add friend request
    bool success = Database::getInstance().addFriendRequest(session.username, friend_username);
    if (!success) {
        return StreamUtils::createErrorResponse(409, "Friend request already sent or failed");
    }

    string data = "{\"message\":\"Friend request sent successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Accept friend request
    bool success = Database::getInstance().acceptFriendRequest(friend_username, session.username);
    if (!success) {
        return StreamUtils::createErrorResponse(404, "Friend request not found");
    }

    string data = "{\"message\":\"Friend request accepted successfully\",\"friendUsername\":\"" + 
                 friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleDeclineFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Decline friend request
    bool success = Database::getInstance().declineFriendRequest(friend_username, session.username);
    if (!success) {
        return StreamUtils::createErrorResponse(404, "Friend request not found");
    }

    string data = "{\"message\":\"Friend request declined successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleFriendReqList(const string& request, ClientSession& session) {
    vector<FriendRequest> requests = Database::getInstance().getFriendRequests(session.username);
    
    stringstream ss;
    ss << "{\"friendRequests\":[";
    for (size_t i = 0; i < requests.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"username\":\"" << requests[i].username 
           << "\",\"sentAt\":" << requests[i].sent_at << "}";
    }
    ss << "]}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

string handleDelFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Check if friends
    if (!Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(404, "Friend not found");
    }
    
    // Delete friend
    bool success = Database::getInstance().deleteFriend(session.username, friend_username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to remove friend");
    }

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

    // Check if recipient exists
    if (!Database::getInstance().userExists(recipient)) {
        return StreamUtils::createErrorResponse(404, "Recipient user not found");
    }
    
    // Send message (stored in database, delivered if online)
    bool success = Database::getInstance().sendMessage(session.username, recipient, message);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to send message");
    }

    string data = "{\"message\":\"Message sent successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace SocialHandlers

} // namespace MillionaireGame

