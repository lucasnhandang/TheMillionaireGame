#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include "config.h"
#include "client_handler.h"
#include "session_manager.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

namespace MillionaireGame {

/**
 * Server Core
 * Manages server lifecycle: start, stop, accept connections
 */
class ServerCore {
public:
    ServerCore(const ServerConfig& config);
    ~ServerCore();

    bool start();
    void run();
    void stopAccepting();
    void stop();

private:
    ServerConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> accepting_;
    int server_fd_;
    static ServerCore* instance_;

    static void signalHandler(int sig);
    void waitForClientsToFinish();
};

} // namespace MillionaireGame

#endif // SERVER_CORE_H

