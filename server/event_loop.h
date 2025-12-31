#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "session_manager.h"
#include "config.h"
#include <poll.h>
#include <vector>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <thread>
#include <unordered_map>

namespace MillionaireGame {

/**
 * EventLoop - I/O Multiplexing using poll()
 * 
 * Manages all client connections in a single thread using poll().
 * Delegates heavy work (database queries, etc.) to worker threads.
 */
class EventLoop {
public:
    explicit EventLoop(const ServerConfig& config);
    ~EventLoop();

    // Disable copy
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    /**
     * Start the event loop
     * @param server_fd The server socket file descriptor
     */
    void run(int server_fd);

    /**
     * Stop the event loop gracefully
     */
    void stop();

    /**
     * Check if event loop is running
     */
    bool isRunning() const { return running_; }

    /**
     * Queue a message to be sent to a specific client
     * Thread-safe: can be called from worker threads
     */
    void queueMessage(int client_fd, const std::string& message);

    /**
     * Queue a task to be executed by worker thread
     * Used for blocking operations like database queries
     */
    using Task = std::function<void()>;
    void queueTask(Task task);

private:
    /**
     * Accept new client connection
     */
    void acceptClient(int server_fd);

    /**
     * Handle client read event
     */
    void handleClientRead(int client_fd);

    /**
     * Handle client write event (flush pending messages)
     */
    void handleClientWrite(int client_fd);

    /**
     * Remove client from poll list
     */
    void removeClient(int client_fd);

    /**
     * Process pending outgoing messages
     */
    void processPendingMessages();

    /**
     * Rebuild pollfd array from active clients
     */
    void rebuildPollFds();

    /**
     * Set socket to non-blocking mode
     */
    bool setNonBlocking(int fd);

    /**
     * Worker thread function
     */
    void workerThreadFunc();

    /**
     * Check for timed-out games and end them automatically
     */
    void checkGameTimeouts();

private:
    ServerConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> stopping_;

    // Poll structures
    std::vector<struct pollfd> poll_fds_;
    int server_fd_;

    // Client fd to index mapping for quick lookup
    std::unordered_map<int, size_t> fd_to_index_;

    // Pending outgoing messages per client
    struct PendingMessage {
        int client_fd;
        std::string message;
    };
    std::queue<PendingMessage> outgoing_queue_;
    std::mutex outgoing_mutex_;

    // Partial read buffers per client (for incomplete messages)
    std::unordered_map<int, std::string> read_buffers_;

    // Write buffers per client (for partial writes)
    std::unordered_map<int, std::string> write_buffers_;

    // Worker thread pool
    std::vector<std::thread> workers_;
    std::queue<Task> task_queue_;
    std::mutex task_mutex_;
    std::condition_variable task_cv_;

    // Clients to remove (deferred removal)
    std::vector<int> clients_to_remove_;
    std::mutex remove_mutex_;

    // Flag to indicate poll_fds_ needs rebuilding
    std::atomic<bool> needs_rebuild_;

    // Pipe for waking up poll() from other threads
    int wakeup_pipe_[2];
};

} // namespace MillionaireGame

#endif // EVENT_LOOP_H
