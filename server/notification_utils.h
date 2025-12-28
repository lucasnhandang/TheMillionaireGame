#ifndef NOTIFICATION_UTILS_H
#define NOTIFICATION_UTILS_H

#include <string>
#include <sys/socket.h>

namespace MillionaireGame {

/**
 * Notification utilities for sending server push notifications to clients
 */
class NotificationUtils {
public:
    /**
     * Send notification directly via socket
     * @param client_fd Client socket file descriptor
     * @param type Notification type (e.g., "GAME_START", "QUESTION_INFO")
     * @param data JSON data string
     * @return true if sent successfully
     */
    static bool sendNotification(int client_fd, const std::string& type, const std::string& data);
    
    /**
     * Broadcast notification to all admin clients
     * @param type Notification type
     * @param data JSON data string
     * @param exclude_fd Optional fd to exclude from broadcast (sender)
     */
    static void broadcastToAdmins(const std::string& type, const std::string& data, int exclude_fd = -1);
    
    /**
     * Broadcast notification to all connected clients
     * @param type Notification type
     * @param data JSON data string
     * @param exclude_fd Optional fd to exclude from broadcast
     */
    static void broadcastToAll(const std::string& type, const std::string& data, int exclude_fd = -1);
};

} // namespace MillionaireGame

#endif // NOTIFICATION_UTILS_H
