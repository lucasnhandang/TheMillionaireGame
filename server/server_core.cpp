#include "server_core.h"
#include "logger.h"
#include "session_manager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <cstring>

using namespace std;
using namespace MillionaireGame;

ServerCore* ServerCore::instance_ = nullptr;

ServerCore::ServerCore(const ServerConfig& config)
    : config_(config), running_(false), server_fd_(-1), event_loop_(nullptr) {
    LogLevel log_level = LogLevel::INFO;
    if (config.log_level == "DEBUG") log_level = LogLevel::DEBUG;
    else if (config.log_level == "WARNING") log_level = LogLevel::WARNING;
    else if (config.log_level == "ERROR") log_level = LogLevel::ERROR;
    
    Logger::getInstance().initialize(config.log_file, log_level);
}

ServerCore::~ServerCore() {
    stop();
}

bool ServerCore::start() {
    if (running_) {
        LOG_WARNING("Server is already running");
        return false;
    }

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        LOG_ERROR("Failed to create socket: " + string(strerror(errno)));
        return false;
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("Failed to set SO_REUSEADDR: " + string(strerror(errno)));
        close(server_fd_);
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config_.port);

    if (::bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_ERROR("Failed to bind socket on port " + to_string(config_.port) + ": " + string(strerror(errno)));
        close(server_fd_);
        return false;
    }

    if (listen(server_fd_, config_.max_clients) < 0) {
        LOG_ERROR("Failed to listen on socket: " + string(strerror(errno)));
        close(server_fd_);
        return false;
    }

    running_ = true;
    LOG_INFO("Server started on port " + to_string(config_.port));

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    ServerCore::instance_ = this;

    return true;
}

void ServerCore::run() {
    if (!running_) {
        LOG_ERROR("Server not started. Call start() first.");
        return;
    }

    LOG_INFO("Running in I/O Multiplexing mode (poll) with " + to_string(config_.worker_threads) + " worker threads");
    
    event_loop_ = std::unique_ptr<EventLoop>(new EventLoop(config_));
    event_loop_->run(server_fd_);
}

void ServerCore::stop() {
    if (!running_) {
        return;
    }

    LOG_INFO("Stopping server...");
    
    // Stop event loop
    if (event_loop_) {
        event_loop_->stop();
    }
    
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    running_ = false;

    Logger::getInstance().close();
    LOG_INFO("Server stopped");
}

void ServerCore::signalHandler(int /* sig */) {
    if (instance_) {
        instance_->stop();
    }
}

