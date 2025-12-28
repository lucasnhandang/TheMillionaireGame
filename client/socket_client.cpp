#include "socket_client.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cerrno>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/select.h>
    #include <netdb.h>
#endif

#ifdef _WIN32
static bool winsock_initialized = false;
#endif

SocketClient::SocketClient() 
    : socket_fd_(INVALID_SOCKET), connected_(false), should_listen_(false) {
#ifdef _WIN32
    initializeWinSock();
#endif
}

SocketClient::~SocketClient() {
    disconnect();
#ifdef _WIN32
    cleanupWinSock();
#endif
}

#ifdef _WIN32
void SocketClient::initializeWinSock() {
    if (!winsock_initialized) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
        } else {
            winsock_initialized = true;
        }
    }
}

void SocketClient::cleanupWinSock() {
    if (winsock_initialized) {
        WSACleanup();
        winsock_initialized = false;
    }
}
#else
void SocketClient::initializeWinSock() {}
void SocketClient::cleanupWinSock() {}
#endif

bool SocketClient::connect(const std::string& host, int port) {
    if (connected_) {
        disconnect();
    }

    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert hostname to IP address
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        // Try to resolve hostname
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) {
            std::cerr << "Failed to resolve hostname: " << host << std::endl;
#ifdef _WIN32
            closesocket(socket_fd_);
#else
            ::close(socket_fd_);
#endif
            socket_fd_ = INVALID_SOCKET;
            return false;
        }

        server_addr.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
        freeaddrinfo(result);
    }

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server: " << host << ":" << port << std::endl;
#ifdef _WIN32
        closesocket(socket_fd_);
#else
        ::close(socket_fd_);
#endif
        socket_fd_ = INVALID_SOCKET;
        return false;
    }

    // Set socket to non-blocking mode for timeout support
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(socket_fd_, FIONBIO, &mode);
#else
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
#endif

    connected_ = true;
    return true;
}

void SocketClient::disconnect() {
    should_listen_ = false;
    if (listen_thread_.joinable()) {
        listen_thread_.join();
    }

    if (socket_fd_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket_fd_);
#else
        ::close(socket_fd_);
#endif
        socket_fd_ = INVALID_SOCKET;
    }
    connected_ = false;
}

bool SocketClient::isConnected() const {
    return connected_ && socket_fd_ != INVALID_SOCKET;
}

bool SocketClient::sendMessage(const std::string& message) {
    if (!isConnected()) {
        return false;
    }

    std::string msg_with_newline = message + "\n";
    
#ifdef _WIN32
    int sent = send(socket_fd_, msg_with_newline.c_str(), static_cast<int>(msg_with_newline.length()), 0);
    if (sent == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            connected_ = false;
            return false;
        }
    }
#else
    ssize_t sent = ::send(socket_fd_, msg_with_newline.c_str(), msg_with_newline.length(), 0);
    if (sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            connected_ = false;
            return false;
        }
    }
#endif

    return true;
}

std::string SocketClient::receiveMessage(int timeout_seconds) {
    if (!isConnected()) {
        return "";
    }

    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    // Check if we have a complete message (ending with newline)
    size_t newline_pos = buffer_.find('\n');
    if (newline_pos != std::string::npos) {
        std::string message = buffer_.substr(0, newline_pos);
        buffer_.erase(0, newline_pos + 1);
        return message;
    }

    // Try to receive more data
    char recv_buffer[4096];
#ifdef _WIN32
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(socket_fd_, &read_fds);
    
    int select_result = select(0, &read_fds, nullptr, nullptr, &timeout);
    if (select_result > 0 && FD_ISSET(socket_fd_, &read_fds)) {
        int received = recv(socket_fd_, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (received > 0) {
            recv_buffer[received] = '\0';
            buffer_ += recv_buffer;
            
            newline_pos = buffer_.find('\n');
            if (newline_pos != std::string::npos) {
                std::string message = buffer_.substr(0, newline_pos);
                buffer_.erase(0, newline_pos + 1);
                return message;
            }
        } else if (received == 0 || WSAGetLastError() != WSAEWOULDBLOCK) {
            connected_ = false;
            return "";
        }
    }
#else
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(socket_fd_, &read_fds);
    
    int select_result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (select_result > 0 && FD_ISSET(socket_fd_, &read_fds)) {
        ssize_t received = recv(socket_fd_, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (received > 0) {
            recv_buffer[received] = '\0';
            buffer_ += recv_buffer;
            
            newline_pos = buffer_.find('\n');
            if (newline_pos != std::string::npos) {
                std::string message = buffer_.substr(0, newline_pos);
                buffer_.erase(0, newline_pos + 1);
                return message;
            }
        } else if (received == 0 || (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            connected_ = false;
            return "";
        }
    }
#endif

    return "";
}

void SocketClient::setMessageCallback(std::function<void(const std::string&)> callback) {
    message_callback_ = callback;
}

void SocketClient::startListening() {
    if (should_listen_) {
        return;
    }

    should_listen_ = true;
    listen_thread_ = std::thread(&SocketClient::listenLoop, this);
}

void SocketClient::listenLoop() {
    char recv_buffer[4096];
    
    while (should_listen_ && isConnected()) {
#ifdef _WIN32
        fd_set read_fds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        FD_ZERO(&read_fds);
        FD_SET(socket_fd_, &read_fds);
        
        int select_result = select(0, &read_fds, nullptr, nullptr, &timeout);
        if (select_result > 0 && FD_ISSET(socket_fd_, &read_fds)) {
            int received = recv(socket_fd_, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (received > 0) {
                recv_buffer[received] = '\0';
                
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex_);
                    buffer_ += recv_buffer;
                    
                    // Process all complete messages
                    size_t newline_pos;
                    while ((newline_pos = buffer_.find('\n')) != std::string::npos) {
                        std::string message = buffer_.substr(0, newline_pos);
                        buffer_.erase(0, newline_pos + 1);
                        
                        if (message_callback_) {
                            message_callback_(message);
                        }
                    }
                }
            } else if (received == 0 || (received < 0 && WSAGetLastError() != WSAEWOULDBLOCK)) {
                connected_ = false;
                break;
            }
        }
#else
        fd_set read_fds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        FD_ZERO(&read_fds);
        FD_SET(socket_fd_, &read_fds);
        
        int select_result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
        if (select_result > 0 && FD_ISSET(socket_fd_, &read_fds)) {
            ssize_t received = recv(socket_fd_, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (received > 0) {
                recv_buffer[received] = '\0';
                
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex_);
                    buffer_ += recv_buffer;
                    
                    // Process all complete messages
                    size_t newline_pos;
                    while ((newline_pos = buffer_.find('\n')) != std::string::npos) {
                        std::string message = buffer_.substr(0, newline_pos);
                        buffer_.erase(0, newline_pos + 1);
                        
                        if (message_callback_) {
                            message_callback_(message);
                        }
                    }
                }
            } else if (received == 0 || (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                connected_ = false;
                break;
            }
        }
#endif
    }
}

