#include "client_core.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <thread>
#include <iostream>
#include <cstring>
#include <errno.h>

using namespace std;

namespace MillionaireGame {

ClientCore::ClientCore() : socket_fd_(-1), connected_(false), listening_(false) {
}

ClientCore::~ClientCore() {
    disconnect();
}

bool ClientCore::connect(const string& host, int port) {
    if (connected_) {
        disconnect();
    }
    
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }
    
    // Resolve hostname
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        cerr << "Error resolving hostname: " << host << endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    
    // Connect to server
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error connecting to server" << endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    return true;
}

void ClientCore::disconnect() {
    if (socket_fd_ >= 0) {
        stopListening();
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool ClientCore::isConnected() const {
    return connected_ && socket_fd_ >= 0;
}

bool ClientCore::sendMessage(const string& message) {
    if (!isConnected()) {
        return false;
    }
    
    string msg = message + "\n";
    ssize_t sent = send(socket_fd_, msg.c_str(), msg.length(), 0);
    
    if (sent < 0) {
        connected_ = false;
        return false;
    }
    
    return sent == static_cast<ssize_t>(msg.length());
}

string ClientCore::receiveMessage(int timeoutSeconds) {
    if (!isConnected()) {
        return "";
    }
    
    // Use select() for proper timeout handling
    if (timeoutSeconds > 0) {
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(socket_fd_, &read_fds);
        
        timeout.tv_sec = timeoutSeconds;
        timeout.tv_usec = 0;
        
        int select_result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (select_result < 0) {
            // Error occurred
            return "";
        } else if (select_result == 0) {
            // Timeout - normal, no need to log
            return "";
        } else if (!FD_ISSET(socket_fd_, &read_fds)) {
            // Socket not ready
            return "";
        }
    }
    
    return readLine();
}

string ClientCore::readLine() {
    if (!isConnected()) {
        return "";
    }
    
    string line;
    char buffer[1];
    
    while (true) {
        ssize_t received = recv(socket_fd_, buffer, 1, 0);
        
        if (received <= 0) {
            if (received == 0) {
                // Connection closed
                connected_ = false;
            }
            return "";
        }
        
        if (buffer[0] == '\n') {
            break;
        }
        
        if (buffer[0] != '\r') {
            line += buffer[0];
        }
    }
    
    return line;
}

void ClientCore::setNotificationCallback(function<void(const string&)> callback) {
    notification_callback_ = callback;
}

void ClientCore::startListening() {
    if (listening_ || !isConnected()) {
        return;
    }
    
    listening_ = true;
    thread listener(&ClientCore::listeningThread, this);
    listener.detach();
}

void ClientCore::stopListening() {
    listening_ = false;
}

void ClientCore::listeningThread() {
    while (listening_ && isConnected()) {
        string message = receiveMessage(1);
        if (!message.empty() && notification_callback_) {
            notification_callback_(message);
        }
    }
}

} // namespace MillionaireGame

