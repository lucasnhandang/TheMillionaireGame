#include "socket_client.h"
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <sstream>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/select.h>
#endif

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
    
    struct sockaddr_in server_addr;
    struct hostent* server;
    
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }
    
    server = gethostbyname(host_.c_str());
    if (server == nullptr) {
        std::cerr << "Error: No such host" << std::endl;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port_);
    
    if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    connected_ = true;
    running_ = true;
    receiveThread_ = std::thread(&SocketClient::receiveLoop, this);
    
    return true;
}

void SocketClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    running_ = false;
    connected_ = false;
    
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
}

bool SocketClient::sendRequest(const std::string& requestType, const std::string& data) {
    if (!connected_ || sockfd_ < 0) {
        std::cerr << "[DEBUG] sendRequest failed: not connected or invalid socket" << std::endl;
        return false;
    }
    
    std::ostringstream json;
    json << "{\"requestType\":\"" << requestType << "\",\"data\":" << data << "}\n";
    std::string message = json.str();
    
    std::cerr << "[DEBUG] sendRequest: sending " << requestType << ", message=" << message.substr(0, 200) << std::endl;
    
    int n = send(sockfd_, message.c_str(), message.length(), 0);
    bool success = n >= 0;
    std::cerr << "[DEBUG] sendRequest: sent " << n << " bytes, success=" << success << std::endl;
    return success;
}

bool SocketClient::getMessage(Message& msg, int timeoutMs) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (!messageQueue_.empty()) {
        msg = messageQueue_.front();
        messageQueue_.pop();
        return true;
    }
    
    return false;
}

void SocketClient::putMessageBack(const Message& msg) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // Create a temporary queue with the message at the front
    std::queue<Message> tempQueue;
    tempQueue.push(msg);
    
    // Move all existing messages to the temp queue
    while (!messageQueue_.empty()) {
        tempQueue.push(messageQueue_.front());
        messageQueue_.pop();
    }
    
    // Replace the original queue with the temp queue
    messageQueue_ = tempQueue;
}

void SocketClient::setNotificationHandler(const std::string& notificationType,
                                          std::function<void(const std::string&)> handler) {
    notificationHandlers_[notificationType] = handler;
}

void SocketClient::receiveLoop() {
    char buffer[4096];
    std::string bufferStr;
    
    while (running_ && connected_) {
        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(sockfd_, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout
        
        int selectResult = select(sockfd_ + 1, &readfds, nullptr, nullptr, &tv);
        
        if (selectResult > 0 && FD_ISSET(sockfd_, &readfds)) {
            int n = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
            
            if (n <= 0) {
                // Connection closed or error
                connected_ = false;
                break;
            }
            
            buffer[n] = '\0';
            bufferStr += buffer;
            
            // Process complete messages (delimited by newline)
            size_t pos;
            while ((pos = bufferStr.find('\n')) != std::string::npos) {
                std::string message = bufferStr.substr(0, pos);
                bufferStr.erase(0, pos + 1);
                
                if (!message.empty()) {
                    handleMessage(message);
                }
            }
        }
    }
}

void SocketClient::handleMessage(const std::string& message) {
    // Determine message type using protocol-level classification
    std::string type = "RESPONSE"; // Default for request responses
    
    // Step 1: Check for notificationType field (server-pushed notifications)
    size_t noti_type_pos = message.find("\"notificationType\"");
    if (noti_type_pos != std::string::npos) {
        // This is a notification - extract the type value
        size_t value_start = message.find(':', noti_type_pos);
        if (value_start != std::string::npos) {
            value_start = message.find('"', value_start);
            if (value_start != std::string::npos) {
                value_start++; // Skip opening quote
                size_t value_end = message.find('"', value_start);
                if (value_end != std::string::npos) {
                    type = message.substr(value_start, value_end - value_start);
                }
            }
        }
    }
    // Step 2: If no notificationType, check for responseCode (request response)
    else if (message.find("\"responseCode\"") != std::string::npos) {
        type = "RESPONSE";
    }
    // Step 3: Unknown message type
    else {
        type = "UNKNOWN";
        std::cerr << "[WARNING] handleMessage: Unknown message type, no notificationType or responseCode field" << std::endl;
    }
    
    std::cerr << "[DEBUG] handleMessage: parsed type=" << type << ", message preview=" << message.substr(0, 150) << std::endl;
    
    Message msg;
    msg.type = type;
    msg.data = message;
    
    // Check for notification handler first
    if (notificationHandlers_.find(type) != notificationHandlers_.end()) {
        notificationHandlers_[type](message);
    }
    
    // Always queue the message
    std::lock_guard<std::mutex> lock(queueMutex_);
    messageQueue_.push(msg);
    std::cerr << "[DEBUG] handleMessage: queued message, queue size=" << messageQueue_.size() << std::endl;
}

