#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <map>
#include <netdb.h>
#include <errno.h>

class SocketClient {
public:
    SocketClient(const std::string& host = "localhost", int port = 8080);
    ~SocketClient();
    
    bool connect();
    void disconnect();
    bool sendRequest(const std::string& requestType, const std::string& data);
    bool isConnected() const { return connected_; }
    
    // Message queue
    struct Message {
        std::string type;
        std::string data;
    };
    
    bool getMessage(Message& msg, int timeoutMs = 1000);
    
    // Notification handlers
    void setNotificationHandler(const std::string& notificationType, 
                                std::function<void(const std::string&)> handler);
    
private:
    std::string host_;
    int port_;
    int sockfd_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    
    std::thread receiveThread_;
    std::mutex queueMutex_;
    std::queue<Message> messageQueue_;
    
    std::map<std::string, std::function<void(const std::string&)>> notificationHandlers_;
    
    void receiveLoop();
    void handleMessage(const std::string& message);
};

#endif // SOCKET_CLIENT_H

