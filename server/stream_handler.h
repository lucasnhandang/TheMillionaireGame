#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <memory>

/**
 * StreamHandler - TCP stream handling for client-server communication
 * 
 * Handles:
 * - Reading/writing JSON messages over TCP sockets
 * - Buffer management for partial messages
 * - Message framing (newline-delimited)
 * - Error handling and timeout management
 */

namespace MillionaireGame {

// Forward declarations
struct StreamBuffer;

/**
 * StreamHandler class for managing TCP stream communication
 */
class StreamHandler {
public:
    /**
     * Constructor
     * @param socket_fd File descriptor of the socket
     * @param buffer_size Initial buffer size (default: 4096 bytes)
     */
    explicit StreamHandler(int socket_fd, size_t buffer_size = 4096);
    
    /**
     * Destructor
     */
    ~StreamHandler();
    
    // Disable copy constructor and assignment operator
    StreamHandler(const StreamHandler&) = delete;
    StreamHandler& operator=(const StreamHandler&) = delete;
    
    /**
     * Read a complete JSON message from the stream
     * Messages are newline-delimited
     * @param timeout_seconds Timeout in seconds (0 = no timeout)
     * @return Complete JSON message string, empty if error or timeout
     */
    std::string readMessage(int timeout_seconds = 0);
    
    /**
     * Write a JSON message to the stream
     * Appends newline delimiter automatically
     * @param message JSON message string
     * @return true on success, false on error
     */
    bool writeMessage(const std::string& message);
    
    /**
     * Check if socket is still connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * Get the socket file descriptor
     * @return Socket file descriptor
     */
    int getSocketFd() const;
    
    /**
     * Set socket timeout for read operations
     * @param seconds Timeout in seconds
     * @param microseconds Timeout in microseconds
     * @return true on success, false on error
     */
    bool setReadTimeout(int seconds, int microseconds);
    
    /**
     * Set socket timeout for write operations
     * @param seconds Timeout in seconds
     * @param microseconds Timeout in microseconds
     * @return true on success, false on error
     */
    bool setWriteTimeout(int seconds, int microseconds);
    
    /**
     * Close the socket connection
     */
    void close();
    
    /**
     * Clear internal buffer
     */
    void clearBuffer();

private:
    int socket_fd_;
    std::unique_ptr<StreamBuffer> buffer_;
    bool connected_;
    
    /**
     * Read data from socket into internal buffer
     * @param timeout_seconds Timeout in seconds
     * @return Number of bytes read, -1 on error, 0 on timeout/EOF
     */
    ssize_t readToBuffer(int timeout_seconds = 0);
    
    /**
     * Extract a complete message (ending with '\n') from buffer
     * @return Complete message string, empty if no complete message available
     */
    std::string extractMessage();
    
    /**
     * Check if socket has data available for reading
     * @param timeout_seconds Timeout in seconds
     * @return true if data available, false on timeout or error
     */
    bool hasDataAvailable(int timeout_seconds = 0);
};

/**
 * Utility functions for JSON message handling
 */
namespace StreamUtils {
    /**
     * Validate JSON message format
     * Basic validation - checks for balanced braces and brackets
     * @param json JSON string to validate
     * @return true if format appears valid, false otherwise
     */
    bool validateJsonFormat(const std::string& json);
    
    /**
     * Extract request type from JSON message
     * @param json JSON message string
     * @return Request type string, empty if not found
     */
    std::string extractRequestType(const std::string& json);
    
    /**
     * Extract response code from JSON message
     * @param json JSON message string
     * @return Response code, -1 if not found or invalid
     */
    int extractResponseCode(const std::string& json);
    
    /**
     * Create error response JSON
     * @param response_code HTTP-like response code
     * @param message Error message
     * @return JSON string
     */
    std::string createErrorResponse(int response_code, const std::string& message);
    
    /**
     * Create success response JSON
     * @param response_code HTTP-like response code (typically 200)
     * @param data JSON data object as string
     * @return JSON string
     */
    std::string createSuccessResponse(int response_code, const std::string& data);
    
    /**
     * Create request JSON
     * @param request_type Request type string
     * @param data JSON data object as string
     * @return JSON string
     */
    std::string createRequest(const std::string& request_type, const std::string& data);
}

} // namespace MillionaireGame

#endif // STREAM_HANDLER_H

