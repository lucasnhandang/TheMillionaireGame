#include "server_core.h"
#include "logger.h"
#include "session_manager.h"
#include "../database/database.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <thread>
#include <chrono>

using namespace std;
using namespace MillionaireGame;

ServerCore* ServerCore::instance_ = nullptr;

ServerCore::ServerCore(const ServerConfig& config)
    : config_(config), running_(false), accepting_(true), server_fd_(-1) {
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

    // Initialize database connection
    string conn_string = "host=" + config_.db_host + 
                       " port=" + to_string(config_.db_port) +
                       " dbname=" + config_.db_name +
                       " user=" + config_.db_user +
                       " password=" + config_.db_password;
    
    if (!Database::getInstance().connect(conn_string)) {
        LOG_ERROR("Failed to connect to database: " + Database::getInstance().getLastError());
        close(server_fd_);
        return false;
    }
    
    LOG_INFO("Database connected successfully");

    running_ = true;
    accepting_ = true;
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

    while (running_) {
        if (!accepting_) {
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (running_ && accepting_) {
                LOG_ERROR("Failed to accept connection: " + string(strerror(errno)));
            }
            continue;
        }

        if (SessionManager::getInstance().getClientCount() >= static_cast<size_t>(config_.max_clients)) {
            LOG_WARNING("Max clients reached, rejecting connection");
            close(client_fd);
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        LOG_INFO("New client connected from " + string(client_ip) + ":" + to_string(ntohs(client_addr.sin_port)));

        thread client_thread(ClientHandler::handleClient, client_fd, string(client_ip), config_);
        client_thread.detach();
    }

    waitForClientsToFinish();
}

void ServerCore::stopAccepting() {
    accepting_ = false;
    LOG_INFO("Stopped accepting new connections. Waiting for existing clients to finish...");
}

void ServerCore::stop() {
    if (!running_) {
        return;
    }

    stopAccepting();
    
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    running_ = false;

    // Disconnect database
    Database::getInstance().disconnect();

    Logger::getInstance().close();
    LOG_INFO("Server stopped");
}

void ServerCore::signalHandler(int sig) {
    if (instance_) {
        instance_->stopAccepting();
    }
}

void ServerCore::waitForClientsToFinish() {
    SessionManager::getInstance().waitForClientsToFinish();
}

