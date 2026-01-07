#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <map>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #define close closesocket
    #define errno WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
#endif

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
    void putMessageBack(const Message& msg);
    
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

