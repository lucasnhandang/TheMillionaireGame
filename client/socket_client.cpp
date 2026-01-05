#include "socket_client.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <io.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    #define EAGAIN WSAEWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK
    #define EINPROGRESS WSAEINPROGRESS
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/select.h>
#endif

#include <cstring>
#include <iostream>
#include <sstream>
#include <errno.h>

SocketClient::SocketClient(const std::string& host, int port)
    : host_(host), port_(port), sockfd_(-1), connected_(false), running_(false) {
}

SocketClient::~SocketClient() {
    disconnect();
}

bool SocketClient::connect() {
    if (connected_) {
        return true;
    }
    
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif
    
    // Create socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }
    
    // Set socket to non-blocking for timeout
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd_, FIONBIO, &mode);
#else
    int flags_nonblock = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags_nonblock | O_NONBLOCK);
#endif
    
    // Setup server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host_.c_str(), &serverAddr.sin_addr) <= 0) {
        // Try to resolve hostname
        struct hostent* he = gethostbyname(host_.c_str());
        if (he == nullptr) {
            std::cerr << "Error resolving hostname: " << host_ << std::endl;
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }
        memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    // Connect with timeout
    int result = ::connect(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        if (errno == EINPROGRESS) {
            // Wait for connection
            fd_set writefds;
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            FD_ZERO(&writefds);
            FD_SET(sockfd_, &writefds);
            
            result = select(sockfd_ + 1, nullptr, &writefds, nullptr, &timeout);
            if (result <= 0) {
                std::cerr << "Connection timeout" << std::endl;
                close(sockfd_);
                sockfd_ = -1;
                return false;
            }
        } else {
            std::cerr << "Connection failed: " << strerror(errno) << std::endl;
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }
    }
    
    // Set back to blocking
#ifdef _WIN32
    mode = 0;
    ioctlsocket(sockfd_, FIONBIO, &mode);
#else
    int flags_block = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags_block & ~O_NONBLOCK);
#endif
    
    connected_ = true;
    running_ = true;
    
    // Start receive thread
    receiveThread_ = std::thread(&SocketClient::receiveLoop, this);
    
    return true;
}

void SocketClient::disconnect() {
    running_ = false;
    connected_ = false;
    
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
}

bool SocketClient::sendRequest(const std::string& requestType, const std::string& data) {
    if (!connected_ || sockfd_ < 0) {
        return false;
    }
    
    std::ostringstream request;
    request << "{\"requestType\":\"" << requestType << "\",\"data\":" << data << "}\n";
    
    std::string message = request.str();
    ssize_t sent = send(sockfd_, message.c_str(), message.length(), 0);
    
    return sent == static_cast<ssize_t>(message.length());
}

bool SocketClient::getMessage(Message& msg, int timeoutMs) {
    (void)timeoutMs; // Suppress unused parameter warning (timeout not implemented yet)
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (messageQueue_.empty()) {
        return false;
    }
    
    msg = messageQueue_.front();
    messageQueue_.pop();
    return true;
}

void SocketClient::setNotificationHandler(const std::string& notificationType,
                                          std::function<void(const std::string&)> handler) {
    notificationHandlers_[notificationType] = handler;
}

void SocketClient::receiveLoop() {
    std::string buffer;
    char recvBuffer[4096];
    
    while (running_ && connected_) {
        if (sockfd_ < 0) {
            break;
        }
        
        ssize_t received = recv(sockfd_, recvBuffer, sizeof(recvBuffer) - 1, 0);
        
        if (received <= 0) {
            if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                // Connection closed or error
                break;
            }
#ifdef _WIN32
            Sleep(10); // 10ms
#else
            usleep(10000); // 10ms
#endif
            continue;
        }
        
        recvBuffer[received] = '\0';
        buffer += recvBuffer;
        
        // Process complete messages (delimited by \n)
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            
            if (!message.empty()) {
                handleMessage(message);
            }
        }
    }
    
    connected_ = false;
}

void SocketClient::handleMessage(const std::string& message) {
    // Check if it's a notification
    if (message.find("\"question\"") != std::string::npos || 
        message.find("\"questionId\"") != std::string::npos) {
        // QUESTION_INFO
        if (notificationHandlers_.find("QUESTION_INFO") != notificationHandlers_.end()) {
            notificationHandlers_["QUESTION_INFO"](message);
        }
        Message msg;
        msg.type = "QUESTION_INFO";
        msg.data = message;
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    } else if (message.find("\"lifelineType\"") != std::string::npos) {
        // LIFELINE_INFO
        if (notificationHandlers_.find("LIFELINE_INFO") != notificationHandlers_.end()) {
            notificationHandlers_["LIFELINE_INFO"](message);
        }
        Message msg;
        msg.type = "LIFELINE_INFO";
        msg.data = message;
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    } else if (message.find("\"gameId\"") != std::string::npos && 
               message.find("\"timestamp\"") != std::string::npos) {
        // GAME_START
        if (notificationHandlers_.find("GAME_START") != notificationHandlers_.end()) {
            notificationHandlers_["GAME_START"](message);
        }
        Message msg;
        msg.type = "GAME_START";
        msg.data = message;
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    } else if (message.find("\"status\"") != std::string::npos &&
               (message.find("\"won\"") != std::string::npos || 
                message.find("\"lost\"") != std::string::npos ||
                message.find("\"quit\"") != std::string::npos)) {
        // GAME_END
        if (notificationHandlers_.find("GAME_END") != notificationHandlers_.end()) {
            notificationHandlers_["GAME_END"](message);
        }
        Message msg;
        msg.type = "GAME_END";
        msg.data = message;
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    } else {
        // Regular response
        Message msg;
        msg.type = "RESPONSE";
        msg.data = message;
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(msg);
    }
}

