#include "stream_handler.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

namespace MillionaireGame {

/**
 * Internal buffer structure for managing partial messages
 */
struct StreamBuffer {
    vector<char> data;
    size_t read_pos;
    size_t write_pos;
    
    StreamBuffer(size_t initial_size) 
        : data(initial_size), read_pos(0), write_pos(0) {}
    
    size_t availableSpace() const {
        return data.size() - write_pos;
    }
    
    size_t availableData() const {
        return write_pos - read_pos;
    }
    
    void compact() {
        if (read_pos > 0 && read_pos < write_pos) {
            size_t data_size = write_pos - read_pos;
            memmove(data.data(), data.data() + read_pos, data_size);
            read_pos = 0;
            write_pos = data_size;
        } else if (read_pos >= write_pos) {
            read_pos = 0;
            write_pos = 0;
        }
    }
    
    void ensureCapacity(size_t needed) {
        if (availableSpace() < needed) {
            compact();
            if (availableSpace() < needed) {
                size_t new_size = max(data.size() * 2, write_pos + needed);
                data.resize(new_size);
            }
        }
    }
};

// StreamHandler Implementation

StreamHandler::StreamHandler(int socket_fd, size_t buffer_size)
    : socket_fd_(socket_fd), buffer_(make_unique<StreamBuffer>(buffer_size)), connected_(true) {
    if (socket_fd_ < 0) {
        throw invalid_argument("Invalid socket file descriptor");
    }
}

StreamHandler::~StreamHandler() {
    close();
}

string StreamHandler::readMessage(int timeout_seconds) {
    if (!connected_) {
        return "";
    }
    
    // First, try to extract a complete message from existing buffer
    string message = extractMessage();
    if (!message.empty()) {
        return message;
    }
    
    // No complete message in buffer, read from socket
    while (connected_) {
        ssize_t bytes_read = readToBuffer(timeout_seconds);
        
        if (bytes_read < 0) {
            // Error occurred
            connected_ = false;
            return "";
        } else if (bytes_read == 0) {
            // EOF or timeout
            if (timeout_seconds > 0) {
                // Timeout occurred
                return "";
            }
            // EOF - connection closed
            connected_ = false;
            return "";
        }
        
        // Try to extract message again
        message = extractMessage();
        if (!message.empty()) {
            return message;
        }
    }
    
    return "";
}

bool StreamHandler::writeMessage(const string& message) {
    if (!connected_ || socket_fd_ < 0) {
        return false;
    }
    
    string message_with_newline = message;
    if (message_with_newline.back() != '\n') {
        message_with_newline += '\n';
    }
    
    const char* data = message_with_newline.c_str();
    size_t total_bytes = message_with_newline.length();
    size_t bytes_sent = 0;
    
    while (bytes_sent < total_bytes) {
        ssize_t result = send(socket_fd_, data + bytes_sent, total_bytes - bytes_sent, 0);
        
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, but we should continue
                continue;
            } else if (errno == EINTR) {
                // Interrupted, retry
                continue;
            } else {
                // Error occurred
                connected_ = false;
                return false;
            }
        } else if (result == 0) {
            // Connection closed
            connected_ = false;
            return false;
        }
        
        bytes_sent += result;
    }
    
    return true;
}

bool StreamHandler::isConnected() const {
    return connected_ && socket_fd_ >= 0;
}

int StreamHandler::getSocketFd() const {
    return socket_fd_;
}

bool StreamHandler::setReadTimeout(int seconds, int microseconds) {
    if (socket_fd_ < 0) {
        return false;
    }
    
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;
    
    return setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0;
}

bool StreamHandler::setWriteTimeout(int seconds, int microseconds) {
    if (socket_fd_ < 0) {
        return false;
    }
    
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;
    
    return setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0;
}

void StreamHandler::close() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
    clearBuffer();
}

void StreamHandler::clearBuffer() {
    if (buffer_) {
        buffer_->read_pos = 0;
        buffer_->write_pos = 0;
    }
}

ssize_t StreamHandler::readToBuffer(int timeout_seconds) {
    if (!buffer_ || socket_fd_ < 0) {
        return -1;
    }
    
    // Check if data is available (with timeout if specified)
    if (timeout_seconds > 0 && !hasDataAvailable(timeout_seconds)) {
        return 0; // Timeout
    }
    
    buffer_->ensureCapacity(1024); // Ensure space for at least 1KB
    
    ssize_t bytes_read = recv(socket_fd_, 
                              buffer_->data.data() + buffer_->write_pos,
                              buffer_->availableSpace(),
                              0);
    
    if (bytes_read > 0) {
        buffer_->write_pos += bytes_read;
    } else if (bytes_read == 0) {
        // EOF - connection closed by peer
        connected_ = false;
    } else {
        // Error
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available (non-blocking)
        } else if (errno == EINTR) {
            // Interrupted, retry
            return readToBuffer(timeout_seconds);
        } else {
            connected_ = false;
        }
    }
    
    return bytes_read;
}

string StreamHandler::extractMessage() {
    if (!buffer_ || buffer_->availableData() == 0) {
        return "";
    }
    
    // Find newline delimiter
    char* start = buffer_->data.data() + buffer_->read_pos;
    char* end = buffer_->data.data() + buffer_->write_pos;
    char* newline = find(start, end, '\n');
    
    if (newline == end) {
        // No complete message found
        return "";
    }
    
    // Extract message (excluding newline)
    size_t message_length = newline - start;
    string message(start, message_length);
    
    // Update read position (skip newline)
    buffer_->read_pos = (newline - buffer_->data.data()) + 1;
    
    // Compact buffer if needed
    if (buffer_->read_pos > buffer_->data.size() / 2) {
        buffer_->compact();
    }
    
    return message;
}

bool StreamHandler::hasDataAvailable(int timeout_seconds) {
    if (socket_fd_ < 0) {
        return false;
    }
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd_, &read_fds);
    
    struct timeval timeout;
    struct timeval* timeout_ptr = nullptr;
    
    if (timeout_seconds > 0) {
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;
        timeout_ptr = &timeout;
    }
    
    int result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, timeout_ptr);
    
    if (result < 0) {
        return false; // Error
    } else if (result == 0) {
        return false; // Timeout
    }
    
    return FD_ISSET(socket_fd_, &read_fds);
}

// StreamUtils Implementation

namespace StreamUtils {

bool validateJsonFormat(const string& json) {
    if (json.empty()) {
        return false;
    }
    
    int brace_count = 0;
    int bracket_count = 0;
    bool in_string = false;
    bool escaped = false;
    
    for (char c : json) {
        if (escaped) {
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            continue;
        }
        
        if (c == '"') {
            in_string = !in_string;
            continue;
        }
        
        if (in_string) {
            continue;
        }
        
        if (c == '{') {
            brace_count++;
        } else if (c == '}') {
            brace_count--;
            if (brace_count < 0) return false;
        } else if (c == '[') {
            bracket_count++;
        } else if (c == ']') {
            bracket_count--;
            if (bracket_count < 0) return false;
        }
    }
    
    return brace_count == 0 && bracket_count == 0 && !in_string;
}

string extractRequestType(const string& json) {
    // Simple extraction - look for "requestType":"VALUE"
    size_t pos = json.find("\"requestType\"");
    if (pos == string::npos) {
        return "";
    }
    
    pos = json.find(':', pos);
    if (pos == string::npos) {
        return "";
    }
    
    pos = json.find('"', pos);
    if (pos == string::npos) {
        return "";
    }
    pos++; // Skip opening quote
    
    size_t end_pos = json.find('"', pos);
    if (end_pos == string::npos) {
        return "";
    }
    
    return json.substr(pos, end_pos - pos);
}

int extractResponseCode(const string& json) {
    // Simple extraction - look for "responseCode":VALUE
    size_t pos = json.find("\"responseCode\"");
    if (pos == string::npos) {
        return -1;
    }
    
    pos = json.find(':', pos);
    if (pos == string::npos) {
        return -1;
    }
    
    pos++; // Skip colon
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) {
        return -1;
    }
    
    // Parse integer
    try {
        return stoi(json.substr(pos));
    } catch (...) {
        return -1;
    }
}

string createErrorResponse(int response_code, const string& message) {
    return "{\"responseCode\":" + to_string(response_code) + 
           ",\"message\":\"" + message + "\"}";
}

string createSuccessResponse(int response_code, const string& data) {
    return "{\"responseCode\":" + to_string(response_code) + 
           ",\"data\":" + data + "}";
}

string createRequest(const string& request_type, const string& data) {
    return "{\"requestType\":\"" + request_type + 
           "\",\"data\":" + data + "}";
}

} // namespace StreamUtils

} // namespace MillionaireGame

