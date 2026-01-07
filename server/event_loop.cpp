#include "event_loop.h"
#include "logger.h"
#include "auth_manager.h"
#include "request_router.h"
#include "session_manager.h"
#include "game_timer.h"
#include "scoring_system.h"
#include "notification_utils.h"
#include "../database/database.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

using namespace std;

namespace MillionaireGame {

EventLoop::EventLoop(const ServerConfig& config)
    : config_(config), running_(false), stopping_(false), 
      server_fd_(-1), needs_rebuild_(false) {
    
    // Create wakeup pipe for cross-thread signaling
    if (pipe(wakeup_pipe_) < 0) {
        LOG_ERROR("Failed to create wakeup pipe: " + string(strerror(errno)));
        wakeup_pipe_[0] = wakeup_pipe_[1] = -1;
    } else {
        setNonBlocking(wakeup_pipe_[0]);
        setNonBlocking(wakeup_pipe_[1]);
    }
}

EventLoop::~EventLoop() {
    stop();
    
    if (wakeup_pipe_[0] >= 0) close(wakeup_pipe_[0]);
    if (wakeup_pipe_[1] >= 0) close(wakeup_pipe_[1]);
}

bool EventLoop::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERROR("fcntl F_GETFL failed: " + string(strerror(errno)));
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_ERROR("fcntl F_SETFL failed: " + string(strerror(errno)));
        return false;
    }
    
    return true;
}

void EventLoop::run(int server_fd) {
    if (running_) {
        LOG_WARNING("EventLoop is already running");
        return;
    }

    server_fd_ = server_fd;
    setNonBlocking(server_fd_);
    
    running_ = true;
    stopping_ = false;

    // Start worker threads
    int num_workers = config_.worker_threads > 0 ? config_.worker_threads : 4;
    LOG_INFO("Starting " + to_string(num_workers) + " worker threads");
    
    for (int i = 0; i < num_workers; ++i) {
        workers_.emplace_back(&EventLoop::workerThreadFunc, this);
    }

    // Initialize poll_fds with server socket and wakeup pipe
    poll_fds_.clear();
    fd_to_index_.clear();
    
    // Index 0: server socket
    struct pollfd server_pfd;
    server_pfd.fd = server_fd_;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    poll_fds_.push_back(server_pfd);
    
    // Index 1: wakeup pipe read end
    if (wakeup_pipe_[0] >= 0) {
        struct pollfd wakeup_pfd;
        wakeup_pfd.fd = wakeup_pipe_[0];
        wakeup_pfd.events = POLLIN;
        wakeup_pfd.revents = 0;
        poll_fds_.push_back(wakeup_pfd);
    }

    LOG_INFO("EventLoop started with poll()");

    while (running_ && !stopping_) {
        // Rebuild poll_fds if needed
        if (needs_rebuild_) {
            rebuildPollFds();
            needs_rebuild_ = false;
        }

        // Process any pending removals
        {
            lock_guard<mutex> lock(remove_mutex_);
            for (int fd : clients_to_remove_) {
                auto it = fd_to_index_.find(fd);
                if (it != fd_to_index_.end()) {
                    // Mark for removal by setting fd to -1
                    poll_fds_[it->second].fd = -1;
                    fd_to_index_.erase(it);
                    needs_rebuild_ = true;
                }
                
                // Cleanup buffers
                read_buffers_.erase(fd);
                write_buffers_.erase(fd);
                
                // Remove session
                ClientSession* session = SessionManager::getInstance().getSession(fd);
                if (session) {
                    if (!session->auth_token.empty()) {
                        AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
                    }
                    if (!session->username.empty()) {
                        SessionManager::getInstance().removeOnlineUser(session->username);
                    }
                }
                SessionManager::getInstance().removeSession(fd);
                
                close(fd);
                LOG_DEBUG("Client " + to_string(fd) + " removed from poll");
            }
            clients_to_remove_.clear();
        }

        // Process pending outgoing messages - update poll events for write
        processPendingMessages();

        // Poll with timeout (1 second for periodic checks)
        int poll_result = poll(poll_fds_.data(), poll_fds_.size(), 1000);

        if (poll_result < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            LOG_ERROR("poll() failed: " + string(strerror(errno)));
            break;
        }

        if (poll_result == 0) {
            // Timeout - do periodic tasks (ping check, game timeout check, etc.)
            checkGameTimeouts();
            continue;
        }

        // Process events
        for (size_t i = 0; i < poll_fds_.size() && poll_result > 0; ++i) {
            if (poll_fds_[i].revents == 0) {
                continue;
            }
            
            poll_result--;
            int fd = poll_fds_[i].fd;

            // Handle wakeup pipe
            if (fd == wakeup_pipe_[0]) {
                // Drain the pipe
                char buf[64];
                while (read(wakeup_pipe_[0], buf, sizeof(buf)) > 0);
                continue;
            }

            // Handle server socket - new connection
            if (fd == server_fd_) {
                if (poll_fds_[i].revents & POLLIN) {
                    acceptClient(server_fd_);
                }
                continue;
            }

            // Handle client socket
            if (poll_fds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Error or hangup
                LOG_DEBUG("Client " + to_string(fd) + " disconnected (poll error/hangup)");
                removeClient(fd);
                continue;
            }

            if (poll_fds_[i].revents & POLLIN) {
                handleClientRead(fd);
            }

            if (poll_fds_[i].revents & POLLOUT) {
                handleClientWrite(fd);
            }
        }
    }

    // Cleanup
    LOG_INFO("EventLoop stopping...");
    
    // Signal workers to stop
    {
        lock_guard<mutex> lock(task_mutex_);
        stopping_ = true;
    }
    task_cv_.notify_all();
    
    // Wait for workers
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    
    // Close all client connections
    for (auto& pfd : poll_fds_) {
        if (pfd.fd >= 0 && pfd.fd != server_fd_ && pfd.fd != wakeup_pipe_[0]) {
            close(pfd.fd);
        }
    }
    poll_fds_.clear();
    fd_to_index_.clear();
    
    running_ = false;
    LOG_INFO("EventLoop stopped");
}

void EventLoop::stop() {
    stopping_ = true;
    running_ = false;
    
    // Wake up poll()
    if (wakeup_pipe_[1] >= 0) {
        char c = 'W';
        ssize_t written = write(wakeup_pipe_[1], &c, 1);
        (void)written; // Suppress unused warning
    }
}

void EventLoop::acceptClient(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Accept all pending connections (non-blocking)
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more pending connections
            }
            LOG_ERROR("accept() failed: " + string(strerror(errno)));
            break;
        }

        // Check max clients
        if (SessionManager::getInstance().getClientCount() >= static_cast<size_t>(config_.max_clients)) {
            LOG_WARNING("Max clients reached, rejecting connection");
            close(client_fd);
            continue;
        }

        // Set non-blocking
        setNonBlocking(client_fd);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        LOG_INFO("New client connected from " + string(client_ip) + ":" + 
                 to_string(ntohs(client_addr.sin_port)) + " (fd=" + to_string(client_fd) + ")");

        // Add to poll_fds
        struct pollfd client_pfd;
        client_pfd.fd = client_fd;
        client_pfd.events = POLLIN; // Initially only interested in reading
        client_pfd.revents = 0;
        
        fd_to_index_[client_fd] = poll_fds_.size();
        poll_fds_.push_back(client_pfd);

        // Create session
        SessionManager::getInstance().createSession(client_fd, string(client_ip));

        // Initialize buffers
        read_buffers_[client_fd] = "";
        write_buffers_[client_fd] = "";

        // Send connection notification
        string connection_msg = StreamUtils::createNotification("CONNECTION", 
            "{\"serverName\":\"Millionaire Game Server\",\"timestamp\":" + to_string(time(nullptr)) + "}") + "\n";
        queueMessage(client_fd, connection_msg);
    }
}

void EventLoop::handleClientRead(int client_fd) {
    char buffer[4096];
    
    while (true) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more data available
            }
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            LOG_ERROR("recv() failed for client " + to_string(client_fd) + ": " + string(strerror(errno)));
            removeClient(client_fd);
            return;
        }
        
        if (bytes_read == 0) {
            // Connection closed by peer
            LOG_INFO("Client " + to_string(client_fd) + " disconnected");
            removeClient(client_fd);
            return;
        }
        
        // Append to read buffer
        read_buffers_[client_fd].append(buffer, bytes_read);
    }
    
    // Process complete messages (newline-delimited)
    string& read_buf = read_buffers_[client_fd];
    size_t pos;
    
    while ((pos = read_buf.find('\n')) != string::npos) {
        string message = read_buf.substr(0, pos);
        read_buf.erase(0, pos + 1);
        
        if (message.empty()) {
            continue;
        }
        
        // Validate JSON format
        if (!StreamUtils::validateJsonFormat(message)) {
            string error = StreamUtils::createErrorResponse(400, "Invalid JSON format") + "\n";
            queueMessage(client_fd, error);
            continue;
        }
        
        // Process request in worker thread (for blocking operations)
        // Capture by value for safety
        int fd = client_fd;
        string request = message;
        
        queueTask([this, fd, request]() {
            RequestRouter router;
            string response = router.processRequest(request, fd);
            
            if (!response.empty()) {
                this->queueMessage(fd, response + "\n");
            }
            
            // Update ping time
            SessionManager::getInstance().updatePingTime(fd);
        });
    }
}

void EventLoop::handleClientWrite(int client_fd) {
    auto it = write_buffers_.find(client_fd);
    if (it == write_buffers_.end() || it->second.empty()) {
        // Nothing to write, disable POLLOUT
        auto idx_it = fd_to_index_.find(client_fd);
        if (idx_it != fd_to_index_.end()) {
            poll_fds_[idx_it->second].events &= ~POLLOUT;
        }
        return;
    }
    
    string& write_buf = it->second;
    
    while (!write_buf.empty()) {
        ssize_t bytes_sent = send(client_fd, write_buf.c_str(), write_buf.length(), 0);
        
        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // Socket buffer full, try again later
            }
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            LOG_ERROR("send() failed for client " + to_string(client_fd) + ": " + string(strerror(errno)));
            removeClient(client_fd);
            return;
        }
        
        if (bytes_sent == 0) {
            break;
        }
        
        write_buf.erase(0, bytes_sent);
    }
    
    // If buffer is empty, disable POLLOUT
    if (write_buf.empty()) {
        auto idx_it = fd_to_index_.find(client_fd);
        if (idx_it != fd_to_index_.end()) {
            poll_fds_[idx_it->second].events &= ~POLLOUT;
        }
    }
}

void EventLoop::removeClient(int client_fd) {
    lock_guard<mutex> lock(remove_mutex_);
    clients_to_remove_.push_back(client_fd);
    
    // Wake up poll to process removal
    if (wakeup_pipe_[1] >= 0) {
        char c = 'R';
        ssize_t written = write(wakeup_pipe_[1], &c, 1);
        (void)written;
    }
}

void EventLoop::processPendingMessages() {
    lock_guard<mutex> lock(outgoing_mutex_);
    
    while (!outgoing_queue_.empty()) {
        PendingMessage& msg = outgoing_queue_.front();
        
        auto it = write_buffers_.find(msg.client_fd);
        if (it != write_buffers_.end()) {
            it->second += msg.message;
            
            // Enable POLLOUT for this client
            auto idx_it = fd_to_index_.find(msg.client_fd);
            if (idx_it != fd_to_index_.end()) {
                poll_fds_[idx_it->second].events |= POLLOUT;
            }
        }
        
        outgoing_queue_.pop();
    }
}

void EventLoop::queueMessage(int client_fd, const string& message) {
    {
        lock_guard<mutex> lock(outgoing_mutex_);
        outgoing_queue_.push({client_fd, message});
    }
    
    // Wake up poll to send the message
    if (wakeup_pipe_[1] >= 0) {
        char c = 'M';
        ssize_t written = write(wakeup_pipe_[1], &c, 1);
        (void)written;
    }
}

void EventLoop::queueTask(Task task) {
    {
        lock_guard<mutex> lock(task_mutex_);
        task_queue_.push(move(task));
    }
    task_cv_.notify_one();
}

void EventLoop::workerThreadFunc() {
    LOG_DEBUG("Worker thread started");
    
    while (!stopping_) {
        Task task;
        
        {
            unique_lock<mutex> lock(task_mutex_);
            task_cv_.wait(lock, [this]() {
                return stopping_ || !task_queue_.empty();
            });
            
            if (stopping_ && task_queue_.empty()) {
                break;
            }
            
            if (!task_queue_.empty()) {
                task = move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const exception& e) {
                LOG_ERROR("Worker task exception: " + string(e.what()));
            }
        }
    }
    
    LOG_DEBUG("Worker thread stopped");
}

void EventLoop::rebuildPollFds() {
    // Remove entries with fd = -1
    poll_fds_.erase(
        remove_if(poll_fds_.begin(), poll_fds_.end(),
            [](const struct pollfd& pfd) { return pfd.fd < 0; }),
        poll_fds_.end()
    );
    
    // Rebuild fd_to_index mapping
    fd_to_index_.clear();
    for (size_t i = 0; i < poll_fds_.size(); ++i) {
        if (poll_fds_[i].fd != server_fd_ && poll_fds_[i].fd != wakeup_pipe_[0]) {
            fd_to_index_[poll_fds_[i].fd] = i;
        }
    }
}

void EventLoop::checkGameTimeouts() {
    // Get all timed-out games
    vector<int> timed_out_games = GameTimer::getInstance().getTimedOutGames();
    
    for (int game_id : timed_out_games) {
        // Find client session for this game
        int client_fd = SessionManager::getInstance().getClientFdByGameId(game_id);
        if (client_fd < 0) {
            // No active session found - client may have disconnected
            // Stop the timer, database cleanup will happen on next START
            GameTimer::getInstance().stopTimer(game_id);
            continue;
        }
        
        ClientSession* session = SessionManager::getInstance().getSession(client_fd);
        if (!session || !session->in_game || session->game_id != game_id) {
            // Session state doesn't match - stop timer and continue
            GameTimer::getInstance().stopTimer(game_id);
            continue;
        }
        
        // End the game due to timeout
        session->in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session->current_question_number);
        int safe_checkpoint_score = session->total_score;
        
        // Update game session in database
        Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
        
        // Send GAME_END notification
        string game_end_data = "{\"gameId\":" + to_string(game_id) +
                              ",\"status\":\"lost\"" +
                              ",\"finalLevel\":" + to_string(session->current_question_number) +
                              ",\"finalQuestionNumber\":" + to_string(session->current_question_number) +
                              ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                              ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                              ",\"isWinner\":false}";
        NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
        
        LOG_INFO("Game " + to_string(game_id) + " timed out and ended automatically");
    }
}

} // namespace MillionaireGame
