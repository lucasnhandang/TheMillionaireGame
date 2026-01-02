#ifndef CLIENT_CORE_H
#define CLIENT_CORE_H

#include <string>
#include <functional>

namespace MillionaireGame {

/**
 * Client core for TCP socket communication
 * Handles connection, sending, and receiving messages
 */
class ClientCore {
public:
    ClientCore();
    ~ClientCore();
    
    /**
     * Connect to server
     * @param host Server hostname or IP
     * @param port Server port
     * @return true if connected successfully
     */
    bool connect(const std::string& host, int port);
    
    /**
     * Disconnect from server
     */
    void disconnect();
    
    /**
     * Check if connected
     */
    bool isConnected() const;
    
    /**
     * Send message to server
     * @param message JSON message to send
     * @return true if sent successfully
     */
    bool sendMessage(const std::string& message);
    
    /**
     * Receive message from server (blocking)
     * @param timeoutSeconds Timeout in seconds (0 = no timeout)
     * @return Received message or empty string on error/timeout
     */
    std::string receiveMessage(int timeoutSeconds = 0);
    
    /**
     * Set callback for notifications (server push messages)
     */
    void setNotificationCallback(std::function<void(const std::string&)> callback);
    
    /**
     * Start listening thread for notifications
     */
    void startListening();
    
    /**
     * Stop listening thread
     */
    void stopListening();

private:
    int socket_fd_;
    bool connected_;
    bool listening_;
    std::function<void(const std::string&)> notification_callback_;
    
    void listeningThread();
    std::string readLine();
};

} // namespace MillionaireGame

#endif // CLIENT_CORE_H

