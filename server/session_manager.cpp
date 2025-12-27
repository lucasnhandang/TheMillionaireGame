#include "session_manager.h"
#include "logger.h"
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

namespace MillionaireGame {

ClientSession::ClientSession(unique_ptr<StreamHandler> h, const string& ip)
    : handler(move(h)), client_ip(ip), connected_time(time(nullptr)),
      last_ping_time(time(nullptr)), authenticated(false), 
      in_game(false), game_id(0), current_question_number(0),
      current_level(0), current_prize(0), total_score(0), role("user") {}

SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

void SessionManager::createSession(int client_fd, unique_ptr<StreamHandler> handler, const string& client_ip) {
    lock_guard<mutex> lock(clients_mutex_);
    active_clients_[client_fd] = ClientSession(move(handler), client_ip);
}

ClientSession* SessionManager::getSession(int client_fd) {
    lock_guard<mutex> lock(clients_mutex_);
    auto it = active_clients_.find(client_fd);
    if (it != active_clients_.end()) {
        return &it->second;
    }
    return nullptr;
}

void SessionManager::removeSession(int client_fd) {
    lock_guard<mutex> lock(clients_mutex_);
    auto it = active_clients_.find(client_fd);
    if (it != active_clients_.end()) {
        string username = it->second.username;
        if (!username.empty()) {
            online_users_.erase(username);
        }
        active_clients_.erase(it);
    }
}

void SessionManager::updatePingTime(int client_fd) {
    lock_guard<mutex> lock(clients_mutex_);
    auto it = active_clients_.find(client_fd);
    if (it != active_clients_.end()) {
        it->second.last_ping_time = time(nullptr);
    }
}

bool SessionManager::isUserOnline(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);
    return online_users_.find(username) != online_users_.end();
}

void SessionManager::addOnlineUser(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);
    online_users_.insert(username);
}

void SessionManager::removeOnlineUser(const string& username) {
    lock_guard<mutex> lock(clients_mutex_);
    online_users_.erase(username);
}

vector<int> SessionManager::getAllClientFds() {
    lock_guard<mutex> lock(clients_mutex_);
    vector<int> fds;
    for (const auto& pair : active_clients_) {
        fds.push_back(pair.first);
    }
    return fds;
}

size_t SessionManager::getClientCount() {
    lock_guard<mutex> lock(clients_mutex_);
    return active_clients_.size();
}

void SessionManager::waitForClientsToFinish() {
    LOG_INFO("Waiting for all clients to disconnect...");
    while (true) {
        {
            lock_guard<mutex> lock(clients_mutex_);
            if (active_clients_.empty()) {
                break;
            }
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
    LOG_INFO("All clients disconnected");
}

} // namespace MillionaireGame

