#include "database.h"
#include "../server/logger.h"
#include <libpq-fe.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <ctime>

// For password hashing (simple hash for demo - in production use bcrypt)
#include <functional>
#include <sstream>

using namespace std;

namespace MillionaireGame {

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    disconnect();
}

// ============================================
// Connection Methods
// ============================================

bool Database::connect(const string& connection_string) {
    if (conn_ != nullptr) {
        disconnect();
    }
    
    conn_ = PQconnectdb(connection_string.c_str());
    
    if (PQstatus(conn_) != CONNECTION_OK) {
        last_error_ = PQerrorMessage(conn_);
        LOG_ERROR("Database connection failed: " + last_error_);
        PQfinish(conn_);
        conn_ = nullptr;
        return false;
    }
    
    LOG_INFO("Database connected successfully");
    return true;
}

bool Database::isConnected() const {
    return conn_ != nullptr && PQstatus(conn_) == CONNECTION_OK;
}

void Database::disconnect() {
    if (conn_ != nullptr) {
        PQfinish(conn_);
        conn_ = nullptr;
        LOG_INFO("Database disconnected");
    }
}

string Database::getLastError() const {
    if (conn_ != nullptr) {
        return PQerrorMessage(conn_);
    }
    return last_error_;
}

string Database::escapeString(const string& str) {
    if (!isConnected()) return "";
    char* escaped = PQescapeLiteral(conn_, str.c_str(), str.length());
    if (escaped == nullptr) return "";
    string result(escaped);
    PQfreemem(escaped);
    return result;
}

// ============================================
// Password Hashing (Simple SHA256 - use bcrypt in production)
// ============================================

string Database::hashPassword(const string& password) {
    // Simple hash function for demo (use bcrypt in production)
    hash<string> hasher;
    size_t hash_value = hasher(password);
    
    stringstream ss;
    ss << hex << hash_value;
    return ss.str();
}

bool Database::verifyPassword(const string& password, const string& hash) {
    string computed_hash = hashPassword(password);
    return computed_hash == hash;
}

int Database::getUserId(const string& username) {
    if (!isConnected()) return 0;
    
    string query = "SELECT id FROM users WHERE username = " + escapeString(username);
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to get user ID: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return 0;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return 0;
    }
    
    int user_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return user_id;
}

// ============================================
// User Operations
// ============================================

bool Database::authenticateUser(const string& username, const string& password) {
    if (!isConnected()) return false;
    
    string query = "SELECT password_hash, is_banned FROM users WHERE username = " + escapeString(username);
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Authentication query failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    string stored_hash = PQgetvalue(res, 0, 0);
    bool is_banned = (PQgetvalue(res, 0, 1)[0] == 't');
    
    PQclear(res);
    
    if (is_banned) {
        return false;
    }
    
    return verifyPassword(password, stored_hash);
}

bool Database::registerUser(const string& username, const string& password) {
    if (!isConnected()) return false;
    if (userExists(username)) return false;
    
    string password_hash = hashPassword(password);
    string query = "INSERT INTO users (username, password_hash) VALUES (" +
                   escapeString(username) + ", " + escapeString(password_hash) + ")";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Registration failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    LOG_INFO("User registered: " + username);
    return true;
}

string Database::getUserRole(const string& username) {
    if (!isConnected()) return "";
    
    string query = "SELECT role FROM users WHERE username = " + escapeString(username);
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return "";
    }
    
    string role = PQgetvalue(res, 0, 0);
    PQclear(res);
    return role;
}

bool Database::changePassword(const string& username, const string& old_password, const string& new_password) {
    if (!isConnected()) return false;
    if (!authenticateUser(username, old_password)) return false;
    
    string new_hash = hashPassword(new_password);
    string query = "UPDATE users SET password_hash = " + escapeString(new_hash) +
                   " WHERE username = " + escapeString(username);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Password change failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::banUser(const string& username, const string& reason) {
    if (!isConnected()) return false;
    
    int user_id = getUserId(username);
    if (user_id == 0) return false;
    
    string query = "UPDATE users SET is_banned = TRUE, ban_reason = " + escapeString(reason) +
                   " WHERE username = " + escapeString(username);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Ban user failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::userExists(const string& username) {
    if (!isConnected()) return false;
    
    string query = "SELECT COUNT(*) FROM users WHERE username = " + escapeString(username);
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count > 0;
}

User Database::getUser(const string& username) {
    User user;
    if (!isConnected()) return user;
    
    string query = "SELECT id, username, password_hash, role, is_banned, ban_reason, "
                   "EXTRACT(EPOCH FROM created_at)::bigint, "
                   "EXTRACT(EPOCH FROM last_login)::bigint "
                   "FROM users WHERE username = " + escapeString(username);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.id = atoi(PQgetvalue(res, 0, 0));
    user.username = PQgetvalue(res, 0, 1);
    user.password_hash = PQgetvalue(res, 0, 2);
    user.role = PQgetvalue(res, 0, 3);
    user.is_banned = (PQgetvalue(res, 0, 4)[0] == 't');
    if (PQgetvalue(res, 0, 5)) user.ban_reason = PQgetvalue(res, 0, 5);
    user.created_at = atol(PQgetvalue(res, 0, 6));
    if (PQgetvalue(res, 0, 7)) user.last_login = atol(PQgetvalue(res, 0, 7));
    
    PQclear(res);
    return user;
}

bool Database::isUserBanned(const string& username) {
    if (!isConnected()) return false;
    
    string query = "SELECT is_banned FROM users WHERE username = " + escapeString(username);
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    bool banned = (PQgetvalue(res, 0, 0)[0] == 't');
    PQclear(res);
    return banned;
}

// ============================================
// Game Operations
// ============================================

int Database::createGameSession(const string& username) {
    if (!isConnected()) return 0;
    
    int user_id = getUserId(username);
    if (user_id == 0) return 0;
    
    string query = "INSERT INTO game_sessions (user_id, status, current_question_number, "
                   "current_level, current_prize, total_score) VALUES (" +
                   to_string(user_id) + ", 'active', 1, 1, 1000000, 0) RETURNING id";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Create game session failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return 0;
    }
    
    int game_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return game_id;
}

bool Database::updateGameSession(const GameSession& session) {
    if (!isConnected()) return false;
    
    string query = "UPDATE game_sessions SET status = " + escapeString(session.status) +
                   ", current_question_number = " + to_string(session.current_question_number) +
                   ", current_level = " + to_string(session.current_level) +
                   ", current_prize = " + to_string(session.current_prize) +
                   ", total_score = " + to_string(session.total_score) +
                   ", final_prize = " + (session.final_prize > 0 ? to_string(session.final_prize) : "NULL") +
                   " WHERE id = " + to_string(session.id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Update game session failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

GameSession Database::getActiveGameSession(const string& username) {
    GameSession session;
    if (!isConnected()) return session;
    
    int user_id = getUserId(username);
    if (user_id == 0) return session;
    
    string query = "SELECT id, user_id, status, current_question_number, current_level, "
                   "current_prize, total_score, final_prize, "
                   "EXTRACT(EPOCH FROM started_at)::bigint, "
                   "EXTRACT(EPOCH FROM ended_at)::bigint "
                   "FROM game_sessions WHERE user_id = " + to_string(user_id) +
                   " AND status = 'active' ORDER BY started_at DESC LIMIT 1";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return session;
    }
    
    session.id = atoi(PQgetvalue(res, 0, 0));
    session.user_id = atoi(PQgetvalue(res, 0, 1));
    session.status = PQgetvalue(res, 0, 2);
    session.current_question_number = atoi(PQgetvalue(res, 0, 3));
    session.current_level = atoi(PQgetvalue(res, 0, 4));
    session.current_prize = atoll(PQgetvalue(res, 0, 5));
    session.total_score = atoi(PQgetvalue(res, 0, 6));
    if (PQgetvalue(res, 0, 7)) session.final_prize = atoll(PQgetvalue(res, 0, 7));
    session.started_at = atol(PQgetvalue(res, 0, 8));
    if (PQgetvalue(res, 0, 9)) session.ended_at = atol(PQgetvalue(res, 0, 9));
    
    PQclear(res);
    return session;
}

bool Database::saveGameProgress(const string& username, int game_id, int question_number, 
                                long long prize, int score, const vector<string>& used_lifelines) {
    if (!isConnected()) return false;
    
    int user_id = getUserId(username);
    if (user_id == 0) return false;
    
    // Create JSON array for used_lifelines
    string lifelines_json = "[";
    for (size_t i = 0; i < used_lifelines.size(); i++) {
        if (i > 0) lifelines_json += ",";
        lifelines_json += "\"" + used_lifelines[i] + "\"";
    }
    lifelines_json += "]";
    
    // Delete existing saved game for this user
    string delete_query = "DELETE FROM saved_games WHERE user_id = " + to_string(user_id);
    PQexec(conn_, delete_query.c_str());
    
    string query = "INSERT INTO saved_games (user_id, game_id, question_number, prize, score, used_lifelines) "
                   "VALUES (" + to_string(user_id) + ", " + to_string(game_id) + ", " +
                   to_string(question_number) + ", " + to_string(prize) + ", " +
                   to_string(score) + ", " + escapeString(lifelines_json) + ")";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Save game progress failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

GameSession Database::loadGameProgress(const string& username) {
    GameSession session;
    if (!isConnected()) return session;
    
    int user_id = getUserId(username);
    if (user_id == 0) return session;
    
    string query = "SELECT sg.game_id, sg.question_number, sg.prize, sg.score, "
                   "gs.status, gs.current_level, gs.total_score "
                   "FROM saved_games sg "
                   "JOIN game_sessions gs ON sg.game_id = gs.id "
                   "WHERE sg.user_id = " + to_string(user_id) + " ORDER BY sg.saved_at DESC LIMIT 1";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return session;
    }
    
    session.id = atoi(PQgetvalue(res, 0, 0));
    session.current_question_number = atoi(PQgetvalue(res, 0, 1));
    session.current_prize = atoll(PQgetvalue(res, 0, 2));
    session.total_score = atoi(PQgetvalue(res, 0, 3));
    session.status = PQgetvalue(res, 0, 4);
    session.current_level = atoi(PQgetvalue(res, 0, 5));
    
    PQclear(res);
    return session;
}

bool Database::endGame(int game_id, const string& status, int total_score, long long final_prize) {
    if (!isConnected()) return false;
    
    string query = "UPDATE game_sessions SET status = " + escapeString(status) +
                   ", total_score = " + to_string(total_score) +
                   ", final_prize = " + to_string(final_prize) +
                   ", ended_at = CURRENT_TIMESTAMP "
                   "WHERE id = " + to_string(game_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("End game failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    
    // Update leaderboard
    query = "SELECT user_id, final_prize FROM game_sessions WHERE id = " + to_string(game_id);
    res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        int user_id = atoi(PQgetvalue(res, 0, 0));
        long long prize = atoll(PQgetvalue(res, 0, 1));
        int final_q = (status == "won") ? 15 : 0; // Calculate from game state
        updateLeaderboard(user_id, final_q, total_score, prize);
    }
    PQclear(res);
    
    return true;
}

bool Database::addGameQuestion(int game_id, int question_order, int question_id) {
    if (!isConnected()) return false;
    
    string query = "INSERT INTO game_questions (game_id, question_order, question_id) "
                   "VALUES (" + to_string(game_id) + ", " + to_string(question_order) + ", " +
                   to_string(question_id) + ") ON CONFLICT DO NOTHING";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Add game question failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::addGameAnswer(int game_id, int question_order, int selected_option, 
                            bool is_correct, int response_time_second) {
    if (!isConnected()) return false;
    
    string query = "INSERT INTO game_answers (game_id, question_order, selected_option, "
                   "is_correct, response_time_second) VALUES (" +
                   to_string(game_id) + ", " + to_string(question_order) + ", " +
                   to_string(selected_option) + ", " + (is_correct ? "TRUE" : "FALSE") + ", " +
                   to_string(response_time_second) + ") ON CONFLICT DO UPDATE SET "
                   "selected_option = EXCLUDED.selected_option, "
                   "is_correct = EXCLUDED.is_correct, "
                   "response_time_second = EXCLUDED.response_time_second";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Add game answer failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

// ============================================
// Leaderboard Operations
// ============================================

vector<LeaderboardEntry> Database::getLeaderboard(const string& type, int page, int limit, const string& username) {
    vector<LeaderboardEntry> entries;
    if (!isConnected()) return entries;
    
    string query;
    if (type == "friend" && !username.empty()) {
        int user_id = getUserId(username);
        if (user_id == 0) return entries;
        
        query = "SELECT DISTINCT u.id, u.username, COALESCE(l.final_question_number, 0), "
                "COALESCE(l.total_score, 0), COALESCE(l.highest_prize, 0) "
                "FROM users u "
                "LEFT JOIN leaderboard l ON u.id = l.user_id "
                "WHERE u.id IN ("
                "  SELECT CASE WHEN user1_id = " + to_string(user_id) + " THEN user2_id ELSE user1_id END "
                "  FROM friendships WHERE user1_id = " + to_string(user_id) + " OR user2_id = " + to_string(user_id) +
                ") OR u.id = " + to_string(user_id) +
                " ORDER BY COALESCE(l.final_question_number, 0) DESC, COALESCE(l.total_score, 0) DESC "
                "LIMIT " + to_string(limit) + " OFFSET " + to_string((page - 1) * limit);
    } else {
        query = "SELECT u.id, u.username, COALESCE(l.final_question_number, 0), "
                "COALESCE(l.total_score, 0), COALESCE(l.highest_prize, 0) "
                "FROM users u "
                "LEFT JOIN leaderboard l ON u.id = l.user_id "
                "ORDER BY COALESCE(l.final_question_number, 0) DESC, COALESCE(l.total_score, 0) DESC "
                "LIMIT " + to_string(limit) + " OFFSET " + to_string((page - 1) * limit);
    }
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Get leaderboard failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return entries;
    }
    
    int rank = (page - 1) * limit + 1;
    for (int i = 0; i < PQntuples(res); i++) {
        LeaderboardEntry entry;
        entry.user_id = atoi(PQgetvalue(res, i, 0));
        entry.username = PQgetvalue(res, i, 1);
        entry.final_question_number = atoi(PQgetvalue(res, i, 2));
        entry.total_score = atoll(PQgetvalue(res, i, 3));
        entry.is_winner = (entry.final_question_number == 15);
        entry.rank = rank++;
        entries.push_back(entry);
    }
    
    PQclear(res);
    return entries;
}

bool Database::updateLeaderboard(int user_id, int final_question_number, long long total_score, long long highest_prize) {
    if (!isConnected()) return false;
    
    // Get current best stats
    string query = "SELECT final_question_number, total_score, highest_prize, games_played "
                   "FROM leaderboard WHERE user_id = " + to_string(user_id);
    PGresult* res = PQexec(conn_, query.c_str());
    
    int best_final_q = final_question_number;
    long long best_score = total_score;
    long long best_prize = highest_prize;
    int games_played = 1;
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        int current_final_q = atoi(PQgetvalue(res, 0, 0));
        long long current_score = atoll(PQgetvalue(res, 0, 1));
        long long current_prize = atoll(PQgetvalue(res, 0, 2));
        games_played = atoi(PQgetvalue(res, 0, 3)) + 1;
        
        // Keep best stats
        if (current_final_q > best_final_q) best_final_q = current_final_q;
        if (current_score > best_score) best_score = current_score;
        if (current_prize > best_prize) best_prize = current_prize;
    }
    PQclear(res);
    
    query = "INSERT INTO leaderboard (user_id, final_question_number, total_score, highest_prize, games_played) "
            "VALUES (" + to_string(user_id) + ", " + to_string(best_final_q) + ", " +
            to_string(best_score) + ", " + to_string(best_prize) + ", " + to_string(games_played) + ") "
            "ON CONFLICT (user_id) DO UPDATE SET "
            "final_question_number = GREATEST(leaderboard.final_question_number, EXCLUDED.final_question_number), "
            "total_score = GREATEST(leaderboard.total_score, EXCLUDED.total_score), "
            "highest_prize = GREATEST(leaderboard.highest_prize, EXCLUDED.highest_prize), "
            "games_played = leaderboard.games_played + 1, "
            "last_updated = CURRENT_TIMESTAMP";
    
    res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Update leaderboard failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

// ============================================
// Friend Operations
// ============================================

vector<string> Database::getFriendsList(const string& username) {
    vector<string> friends;
    if (!isConnected()) return friends;
    
    int user_id = getUserId(username);
    if (user_id == 0) return friends;
    
    string query = "SELECT u.username FROM users u "
                   "JOIN friendships f ON (f.user1_id = u.id OR f.user2_id = u.id) "
                   "WHERE (f.user1_id = " + to_string(user_id) + " OR f.user2_id = " + to_string(user_id) + ") "
                   "AND u.id != " + to_string(user_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return friends;
    }
    
    for (int i = 0; i < PQntuples(res); i++) {
        friends.push_back(PQgetvalue(res, i, 0));
    }
    
    PQclear(res);
    return friends;
}

bool Database::addFriendRequest(const string& from_user, const string& to_user) {
    if (!isConnected()) return false;
    if (from_user == to_user) return false;
    if (friendshipExists(from_user, to_user)) return false;
    
    int from_id = getUserId(from_user);
    int to_id = getUserId(to_user);
    if (from_id == 0 || to_id == 0) return false;
    
    string query = "INSERT INTO friend_requests (from_user_id, to_user_id, status) "
                   "VALUES (" + to_string(from_id) + ", " + to_string(to_id) + ", 'pending') "
                   "ON CONFLICT (from_user_id, to_user_id) DO UPDATE SET status = 'pending'";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Add friend request failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::acceptFriendRequest(const string& from_user, const string& to_user) {
    if (!isConnected()) return false;
    
    int from_id = getUserId(from_user);
    int to_id = getUserId(to_user);
    if (from_id == 0 || to_id == 0) return false;
    
    // Update friend request status
    string query = "UPDATE friend_requests SET status = 'accepted' "
                   "WHERE from_user_id = " + to_string(from_id) + " AND to_user_id = " + to_string(to_id);
    PGresult* res = PQexec(conn_, query.c_str());
    PQclear(res);
    
    // Add to friendships (ensure user1_id < user2_id)
    int user1_id = min(from_id, to_id);
    int user2_id = max(from_id, to_id);
    
    query = "INSERT INTO friendships (user1_id, user2_id) "
            "VALUES (" + to_string(user1_id) + ", " + to_string(user2_id) + ") "
            "ON CONFLICT DO NOTHING";
    
    res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Accept friend request failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::declineFriendRequest(const string& from_user, const string& to_user) {
    if (!isConnected()) return false;
    
    int from_id = getUserId(from_user);
    int to_id = getUserId(to_user);
    if (from_id == 0 || to_id == 0) return false;
    
    string query = "UPDATE friend_requests SET status = 'declined' "
                   "WHERE from_user_id = " + to_string(from_id) + " AND to_user_id = " + to_string(to_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::deleteFriend(const string& user1, const string& user2) {
    if (!isConnected()) return false;
    
    int user1_id = getUserId(user1);
    int user2_id = getUserId(user2);
    if (user1_id == 0 || user2_id == 0) return false;
    
    int id1 = min(user1_id, user2_id);
    int id2 = max(user1_id, user2_id);
    
    string query = "DELETE FROM friendships WHERE user1_id = " + to_string(id1) +
                   " AND user2_id = " + to_string(id2);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

vector<FriendRequest> Database::getFriendRequests(const string& username) {
    vector<FriendRequest> requests;
    if (!isConnected()) return requests;
    
    int user_id = getUserId(username);
    if (user_id == 0) return requests;
    
    string query = "SELECT fr.id, u.username, EXTRACT(EPOCH FROM fr.created_at)::bigint "
                   "FROM friend_requests fr "
                   "JOIN users u ON fr.from_user_id = u.id "
                   "WHERE fr.to_user_id = " + to_string(user_id) + " AND fr.status = 'pending'";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return requests;
    }
    
    for (int i = 0; i < PQntuples(res); i++) {
        FriendRequest req;
        req.id = atoi(PQgetvalue(res, i, 0));
        req.username = PQgetvalue(res, i, 1);
        req.sent_at = atol(PQgetvalue(res, i, 2));
        requests.push_back(req);
    }
    
    PQclear(res);
    return requests;
}

bool Database::friendshipExists(const string& user1, const string& user2) {
    if (!isConnected()) return false;
    
    int user1_id = getUserId(user1);
    int user2_id = getUserId(user2);
    if (user1_id == 0 || user2_id == 0) return false;
    
    int id1 = min(user1_id, user2_id);
    int id2 = max(user1_id, user2_id);
    
    string query = "SELECT COUNT(*) FROM friendships WHERE user1_id = " + to_string(id1) +
                   " AND user2_id = " + to_string(id2);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count > 0;
}

// ============================================
// Message Operations
// ============================================

bool Database::sendMessage(const string& sender, const string& receiver, const string& content, int game_id) {
    if (!isConnected()) return false;
    
    int sender_id = getUserId(sender);
    int receiver_id = getUserId(receiver);
    if (sender_id == 0 || receiver_id == 0) return false;
    
    string query = "INSERT INTO messages (sender_id, receiver_id, content, game_id) "
                   "VALUES (" + to_string(sender_id) + ", " + to_string(receiver_id) + ", " +
                   escapeString(content) + ", " + (game_id > 0 ? to_string(game_id) : "NULL") + ")";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Send message failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

vector<pair<string, string>> Database::getMessages(const string& username) {
    vector<pair<string, string>> messages;
    if (!isConnected()) return messages;
    
    int user_id = getUserId(username);
    if (user_id == 0) return messages;
    
    string query = "SELECT u.username, m.content FROM messages m "
                   "JOIN users u ON m.sender_id = u.id "
                   "WHERE m.receiver_id = " + to_string(user_id) + " AND m.is_read = FALSE "
                   "ORDER BY m.sent_at DESC";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return messages;
    }
    
    for (int i = 0; i < PQntuples(res); i++) {
        messages.push_back(make_pair(PQgetvalue(res, i, 0), PQgetvalue(res, i, 1)));
    }
    
    PQclear(res);
    return messages;
}

// ============================================
// Game History
// ============================================

vector<GameSession> Database::getGameHistory(const string& username, int limit) {
    vector<GameSession> sessions;
    if (!isConnected()) return sessions;
    
    int user_id = getUserId(username);
    if (user_id == 0) return sessions;
    
    string query = "SELECT id, user_id, status, current_question_number, current_level, "
                   "current_prize, total_score, final_prize, "
                   "EXTRACT(EPOCH FROM started_at)::bigint, "
                   "EXTRACT(EPOCH FROM ended_at)::bigint "
                   "FROM game_sessions WHERE user_id = " + to_string(user_id) +
                   " AND status != 'active' ORDER BY ended_at DESC LIMIT " + to_string(limit);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return sessions;
    }
    
    for (int i = 0; i < PQntuples(res); i++) {
        GameSession session;
        session.id = atoi(PQgetvalue(res, i, 0));
        session.user_id = atoi(PQgetvalue(res, i, 1));
        session.status = PQgetvalue(res, i, 2);
        session.current_question_number = atoi(PQgetvalue(res, i, 3));
        session.current_level = atoi(PQgetvalue(res, i, 4));
        session.current_prize = atoll(PQgetvalue(res, i, 5));
        session.total_score = atoi(PQgetvalue(res, i, 6));
        if (PQgetvalue(res, i, 7)) session.final_prize = atoll(PQgetvalue(res, i, 7));
        session.started_at = atol(PQgetvalue(res, i, 8));
        if (PQgetvalue(res, i, 9)) session.ended_at = atol(PQgetvalue(res, i, 9));
        sessions.push_back(session);
    }
    
    PQclear(res);
    return sessions;
}

// ============================================
// Admin Operations
// ============================================

int Database::addQuestion(const Question& question) {
    if (!isConnected()) return 0;
    
    string query = "INSERT INTO questions (question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level, is_active, updated_by) VALUES (" +
                   escapeString(question.question_text) + ", " +
                   escapeString(question.option_a) + ", " +
                   escapeString(question.option_b) + ", " +
                   escapeString(question.option_c) + ", " +
                   escapeString(question.option_d) + ", " +
                   to_string(question.correct_answer) + ", " +
                   to_string(question.level) + ", " +
                   (question.is_active ? "TRUE" : "FALSE") + ", " +
                   (question.updated_by > 0 ? to_string(question.updated_by) : "NULL") + ") RETURNING id";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Add question failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return 0;
    }
    
    int question_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return question_id;
}

bool Database::updateQuestion(int question_id, const Question& question) {
    if (!isConnected()) return false;
    
    string query = "UPDATE questions SET question_text = " + escapeString(question.question_text) +
                   ", option_a = " + escapeString(question.option_a) +
                   ", option_b = " + escapeString(question.option_b) +
                   ", option_c = " + escapeString(question.option_c) +
                   ", option_d = " + escapeString(question.option_d) +
                   ", correct_answer = " + to_string(question.correct_answer) +
                   ", level = " + to_string(question.level) +
                   ", updated_at = CURRENT_TIMESTAMP" +
                   (question.updated_by > 0 ? ", updated_by = " + to_string(question.updated_by) : "") +
                   " WHERE id = " + to_string(question_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Update question failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::deleteQuestion(int question_id) {
    if (!isConnected()) return false;
    
    // Soft delete
    string query = "UPDATE questions SET is_active = FALSE WHERE id = " + to_string(question_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Delete question failed: " + string(PQerrorMessage(conn_)));
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

Question Database::getQuestion(int question_id) {
    Question question;
    if (!isConnected()) return question;
    
    string query = "SELECT id, question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level, is_active, "
                   "EXTRACT(EPOCH FROM created_at)::bigint, "
                   "EXTRACT(EPOCH FROM updated_at)::bigint, updated_by "
                   "FROM questions WHERE id = " + to_string(question_id);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return question;
    }
    
    question.id = atoi(PQgetvalue(res, 0, 0));
    question.question_text = PQgetvalue(res, 0, 1);
    question.option_a = PQgetvalue(res, 0, 2);
    question.option_b = PQgetvalue(res, 0, 3);
    question.option_c = PQgetvalue(res, 0, 4);
    question.option_d = PQgetvalue(res, 0, 5);
    question.correct_answer = atoi(PQgetvalue(res, 0, 6));
    question.level = atoi(PQgetvalue(res, 0, 7));
    question.is_active = (PQgetvalue(res, 0, 8)[0] == 't');
    question.created_at = atol(PQgetvalue(res, 0, 9));
    if (PQgetvalue(res, 0, 10)) question.updated_at = atol(PQgetvalue(res, 0, 10));
    if (PQgetvalue(res, 0, 11)) question.updated_by = atoi(PQgetvalue(res, 0, 11));
    
    PQclear(res);
    return question;
}

vector<Question> Database::getQuestions(int level, int page, int limit) {
    vector<Question> questions;
    if (!isConnected()) return questions;
    
    string query = "SELECT id, question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level, is_active FROM questions WHERE is_active = TRUE";
    
    if (level > 0) {
        query += " AND level = " + to_string(level);
    }
    
    query += " ORDER BY id LIMIT " + to_string(limit) + " OFFSET " + to_string((page - 1) * limit);
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return questions;
    }
    
    for (int i = 0; i < PQntuples(res); i++) {
        Question q;
        q.id = atoi(PQgetvalue(res, i, 0));
        q.question_text = PQgetvalue(res, i, 1);
        q.option_a = PQgetvalue(res, i, 2);
        q.option_b = PQgetvalue(res, i, 3);
        q.option_c = PQgetvalue(res, i, 4);
        q.option_d = PQgetvalue(res, i, 5);
        q.correct_answer = atoi(PQgetvalue(res, i, 6));
        q.level = atoi(PQgetvalue(res, i, 7));
        q.is_active = (PQgetvalue(res, i, 8)[0] == 't');
        questions.push_back(q);
    }
    
    PQclear(res);
    return questions;
}

bool Database::questionExists(int question_id) {
    if (!isConnected()) return false;
    
    string query = "SELECT COUNT(*) FROM questions WHERE id = " + to_string(question_id) + " AND is_active = TRUE";
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count > 0;
}

Question Database::getRandomQuestion(int level) {
    Question question;
    if (!isConnected()) return question;
    
    string query = "SELECT id, question_text, option_a, option_b, option_c, option_d, "
                   "correct_answer, level FROM questions "
                   "WHERE level = " + to_string(level) + " AND is_active = TRUE "
                   "ORDER BY RANDOM() LIMIT 1";
    
    PGresult* res = PQexec(conn_, query.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return question;
    }
    
    question.id = atoi(PQgetvalue(res, 0, 0));
    question.question_text = PQgetvalue(res, 0, 1);
    question.option_a = PQgetvalue(res, 0, 2);
    question.option_b = PQgetvalue(res, 0, 3);
    question.option_c = PQgetvalue(res, 0, 4);
    question.option_d = PQgetvalue(res, 0, 5);
    question.correct_answer = atoi(PQgetvalue(res, 0, 6));
    question.level = atoi(PQgetvalue(res, 0, 7));
    question.is_active = true;
    
    PQclear(res);
    return question;
}

} // namespace MillionaireGame

