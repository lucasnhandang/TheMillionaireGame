#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <string>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketFD;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int SocketFD;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

/**
 * SocketClient - TCP socket client for connecting to game server
 * Handles TCP connection, message sending/receiving with newline delimiter
 */
class SocketClient {
public:
    SocketClient();
    ~SocketClient();

    // Connection management
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    // Message sending/receiving
    bool sendMessage(const std::string& message);
    std::string receiveMessage(int timeout_seconds = 5);

    // Callback for incoming messages
    void setMessageCallback(std::function<void(const std::string&)> callback);
    
    // Start listening thread for async message receiving
    void startListening();

private:
    SocketFD socket_fd_;
    bool connected_;
    std::string buffer_;
    std::mutex buffer_mutex_;
    std::thread listen_thread_;
    std::atomic<bool> should_listen_;
    std::function<void(const std::string&)> message_callback_;

    void listenLoop();
    void initializeWinSock();
    void cleanupWinSock();
};

#endif // SOCKET_CLIENT_H

