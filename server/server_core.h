#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include "config.h"
#include "session_manager.h"
#include "event_loop.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>

namespace MillionaireGame {

/**
 * Server Core
 * Manages server lifecycle: start, stop, accept connections
 * Uses poll()-based I/O multiplexing with worker threads
 */
class ServerCore {
public:
    ServerCore(const ServerConfig& config);
    ~ServerCore();

    bool start();
    void run();
    void stop();

private:
    ServerConfig config_;
    std::atomic<bool> running_;
    int server_fd_;
    static ServerCore* instance_;
    
    // Event loop for I/O multiplexing
    std::unique_ptr<EventLoop> event_loop_;

    static void signalHandler(int sig);
};

} // namespace MillionaireGame

#endif // SERVER_CORE_H

