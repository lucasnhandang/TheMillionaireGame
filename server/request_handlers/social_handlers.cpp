#include "social_handlers.h"
#include "../session_manager.h"
#include "../json_utils.h"
#include "../stream_handler.h"
#include "../notification_utils.h"
#include "../../database/database.h"
#include <vector>
#include <sstream>
#include <ctime>

using namespace std;

namespace MillionaireGame {

namespace SocialHandlers {

namespace {
std::string jsonEscape(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}
}

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

string handleFindFriend(const string& request, ClientSession& session) {
    (void)session; // not used beyond auth
    string target_username = JsonUtils::extractString(request, "username");
    
    if (target_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing username");
    }
    
    if (!Database::getInstance().userExists(target_username)) {
        return StreamUtils::createErrorResponse(404, "User not found");
    }
    
    bool already_friend = Database::getInstance().friendshipExists(session.username, target_username);
    string status = already_friend ? "friend" : "not_friend";
    if (target_username == session.username) {
        status = "self";
    }
    
    string data = "{\"username\":\"" + target_username + "\","
                  "\"status\":\"" + status + "\"}";
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

    // Notify recipient if online
    int friend_fd = SessionManager::getInstance().getClientFdByUsername(friend_username);
    if (friend_fd != -1) {
        string notif = "{\"from\":\"" + jsonEscape(session.username) + "\","
                       "\"sentAt\":" + to_string(time(nullptr)) + "}";
        NotificationUtils::sendNotification(friend_fd, "FRIEND_REQUEST", notif);
    }

    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAcceptFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Check if friend request exists (from friend_username to session.username)
    vector<FriendRequest> requests = Database::getInstance().getFriendRequests(session.username);
    bool request_exists = false;
    for (const auto& req : requests) {
        if (req.username == friend_username) {
            request_exists = true;
            break;
        }
    }
    
    if (!request_exists) {
        return StreamUtils::createErrorResponse(404, "Friend request not found");
    }
    
    // Check if already friends
    if (Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(409, "Friend already exists");
    }
    
    // Accept friend request (from friend_username to session.username)
    bool success = Database::getInstance().acceptFriendRequest(friend_username, session.username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to accept friend request");
    }

    int friend_fd = SessionManager::getInstance().getClientFdByUsername(friend_username);
    if (friend_fd != -1) {
        string notif = "{\"friend\":\"" + jsonEscape(session.username) + "\"}";
        NotificationUtils::sendNotification(friend_fd, "FRIEND_REQUEST_ACCEPTED", notif);
    }

    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleDeclineFriend(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");

    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }

    // Check if friend request exists (from friend_username to session.username)
    vector<FriendRequest> requests = Database::getInstance().getFriendRequests(session.username);
    bool request_exists = false;
    for (const auto& req : requests) {
        if (req.username == friend_username) {
            request_exists = true;
            break;
        }
    }
    
    if (!request_exists) {
        return StreamUtils::createErrorResponse(404, "Friend request not found");
    }
    
    // Decline friend request (from friend_username to session.username)
    bool success = Database::getInstance().declineFriendRequest(friend_username, session.username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to decline friend request");
    }

    string data = "{\"friendUsername\":\"" + friend_username + "\"}";
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

    // Check if friendship exists
    if (!Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(404, "Friend not found");
    }
    
    // Delete friendship
    bool success = Database::getInstance().deleteFriend(session.username, friend_username);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to delete friend");
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
    
    // Only allow chat between friends
    if (!Database::getInstance().friendshipExists(session.username, recipient)) {
        return StreamUtils::createErrorResponse(403, "Cannot chat with non-friend user");
    }
    
    // Send message (stored in database, delivered if online)
    bool success = Database::getInstance().sendMessage(session.username, recipient, message);
    if (!success) {
        return StreamUtils::createErrorResponse(500, "Failed to send message");
    }

    // Notify recipient if online
    int recipient_fd = SessionManager::getInstance().getClientFdByUsername(recipient);
    if (recipient_fd != -1) {
        string notif = "{\"from\":\"" + jsonEscape(session.username) + "\","
                       "\"content\":\"" + jsonEscape(message) + "\","
                       "\"timestamp\":" + to_string(time(nullptr)) + "}";
        NotificationUtils::sendNotification(recipient_fd, "NEW_MESSAGE", notif);
    }

    string data = "{\"recipient\":\"" + recipient + "\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleGetMessages(const string& request, ClientSession& session) {
    string friend_username = JsonUtils::extractString(request, "friendUsername");
    int page = JsonUtils::extractInt(request, "page", 1);
    int limit = JsonUtils::extractInt(request, "limit", 50);
    
    if (friend_username.empty()) {
        return StreamUtils::createErrorResponse(400, "Missing friendUsername");
    }
    
    if (page < 1 || limit <= 0 || limit > 200) {
        return StreamUtils::createErrorResponse(422, "Invalid page or limit");
    }
    
    // Only allow viewing chat history with friends
    if (!Database::getInstance().friendshipExists(session.username, friend_username)) {
        return StreamUtils::createErrorResponse(403, "Cannot view messages with non-friend user");
    }
    
    vector<ChatMessage> messages = Database::getInstance().getConversationMessages(
        session.username, friend_username, page, limit);
    
    stringstream ss;
    ss << "{\"messages\":[";
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{"
           << "\"from\":\"" << messages[i].sender << "\","
           << "\"to\":\"" << messages[i].receiver << "\","
           << "\"content\":\"" << messages[i].content << "\","
           << "\"timestamp\":" << messages[i].timestamp
           << "}";
    }
    ss << "],\"page\":" << page << ",\"limit\":" << limit << "}";
    
    return StreamUtils::createSuccessResponse(200, ss.str());
}

} // namespace SocialHandlers

} // namespace MillionaireGame

