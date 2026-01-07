#include "notification_utils.h"
#include "stream_handler.h"
#include "session_manager.h"
#include <unistd.h>

using namespace std;

namespace MillionaireGame {

bool NotificationUtils::sendNotification(int client_fd, const string& type, const string& data) {
    string notification = StreamUtils::createNotification(type, data);
    notification += "\n";  // Add delimiter
    
    ssize_t sent = send(client_fd, notification.c_str(), notification.length(), 0);
    return sent > 0;
}

void NotificationUtils::broadcastToAdmins(const string& type, const string& data, int exclude_fd) {
    string notification = StreamUtils::createNotification(type, data);
    notification += "\n";
    
    vector<int> client_fds = SessionManager::getInstance().getAllClientFds();
    for (int fd : client_fds) {
        if (fd == exclude_fd) continue;
        
        ClientSession* session = SessionManager::getInstance().getSession(fd);
        if (session && session->role == "admin") {
            send(fd, notification.c_str(), notification.length(), 0);
        }
    }
}

void NotificationUtils::broadcastToAll(const string& type, const string& data, int exclude_fd) {
    string notification = StreamUtils::createNotification(type, data);
    notification += "\n";
    
    vector<int> client_fds = SessionManager::getInstance().getAllClientFds();
    for (int fd : client_fds) {
        if (fd == exclude_fd) continue;
        send(fd, notification.c_str(), notification.length(), 0);
    }
}

} // namespace MillionaireGame
