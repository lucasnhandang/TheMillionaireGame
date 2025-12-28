#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <ctime>

// Include PostgreSQL headers - needed for PGconn type
// Include path is set in Makefile to handle versioned directories
#include <libpq-fe.h>

namespace MillionaireGame {

// ============================================
// Data Structures
// ============================================

struct User {
    int id;
    std::string username;
    std::string password_hash;
    std::string role;  // "user" or "admin"
    bool is_banned;
    std::string ban_reason;
    time_t created_at;
    time_t last_login;
    
    User() : id(0), is_banned(false), created_at(0), last_login(0) {}
};

struct Question {
    int id;
    std::string question_text;
    std::string option_a;
    std::string option_b;
    std::string option_c;
    std::string option_d;
    int correct_answer;  // 0-3
    int level;  // 1-15
    bool is_active;
    time_t created_at;
    time_t updated_at;
    int updated_by;
    
    Question() : id(0), correct_answer(0), level(1), is_active(true), 
                 created_at(0), updated_at(0), updated_by(0) {}
};

struct GameSession {
    int id;
    int user_id;
    std::string status;  // "active", "won", "lost", "quit"
    int current_question_number;  // 1-15
    int current_level;  // 1-15
    long long current_prize;  // BIGINT
    int total_score;
    long long final_prize;  // BIGINT
    time_t started_at;
    time_t ended_at;
    
    GameSession() : id(0), user_id(0), status("active"), 
                    current_question_number(1), current_level(1),
                    current_prize(1000000), total_score(0), final_prize(0),
                    started_at(0), ended_at(0) {}
};

struct LeaderboardEntry {
    int user_id;
    std::string username;
    int final_question_number;  // 1-15
    long long total_score;  // BIGINT
    bool is_winner;  // true if final_question_number == 15
    int rank;
    
    LeaderboardEntry() : user_id(0), final_question_number(0), 
                         total_score(0), is_winner(false), rank(0) {}
};

struct FriendRequest {
    int id;
    std::string username;
    time_t sent_at;
    
    FriendRequest() : id(0), sent_at(0) {}
};

/**
 * Database Module
 * Singleton class for database operations using PostgreSQL (libpq)
 */
class Database {
public:
    static Database& getInstance();
    
    // Connection
    bool connect(const std::string& connection_string);
    bool isConnected() const;
    void disconnect();
    
    // User operations
    bool authenticateUser(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    std::string getUserRole(const std::string& username);
    bool changePassword(const std::string& username, const std::string& old_password, const std::string& new_password);
    bool banUser(const std::string& username, const std::string& reason);
    bool userExists(const std::string& username);
    User getUser(const std::string& username);
    bool isUserBanned(const std::string& username);
    bool updateLastLogin(const std::string& username);
    
    // Game operations
    int createGameSession(const std::string& username);
    bool updateGameSession(const GameSession& session);
    GameSession getActiveGameSession(const std::string& username);
    bool saveGameProgress(const std::string& username, int game_id, int question_number, long long prize, int score, const std::vector<std::string>& used_lifelines);
    GameSession loadGameProgress(const std::string& username);
    bool endGame(int game_id, const std::string& status, int total_score, long long final_prize);
    bool addGameQuestion(int game_id, int question_order, int question_id);
    bool addGameAnswer(int game_id, int question_order, int selected_option, bool is_correct, int response_time_second);
    
    // Leaderboard
    std::vector<LeaderboardEntry> getLeaderboard(const std::string& type, int page, int limit, const std::string& username = "");
    bool updateLeaderboard(int user_id, int final_question_number, long long total_score, long long highest_prize);
    
    // Friends
    std::vector<std::string> getFriendsList(const std::string& username);
    bool addFriendRequest(const std::string& from_user, const std::string& to_user);
    bool acceptFriendRequest(const std::string& from_user, const std::string& to_user);
    bool declineFriendRequest(const std::string& from_user, const std::string& to_user);
    bool deleteFriend(const std::string& user1, const std::string& user2);
    std::vector<FriendRequest> getFriendRequests(const std::string& username);
    bool friendshipExists(const std::string& user1, const std::string& user2);
    
    // Messages
    bool sendMessage(const std::string& sender, const std::string& receiver, const std::string& content, int game_id = 0);
    std::vector<std::pair<std::string, std::string>> getMessages(const std::string& username);
    
    // Game history
    std::vector<GameSession> getGameHistory(const std::string& username, int limit = 20);
    
    // Admin operations
    int addQuestion(const Question& question);
    bool updateQuestion(int question_id, const Question& question);
    bool deleteQuestion(int question_id);  // Soft delete (sets is_active = false)
    Question getQuestion(int question_id);
    Question getGameQuestion(int game_id, int question_order);  // Get question assigned to game
    std::vector<Question> getQuestions(int level, int page, int limit);
    bool questionExists(int question_id);
    Question getRandomQuestion(int level);  // Get random active question for level
    
    // Error handling
    std::string getLastError() const;

private:
    Database() : conn_(nullptr) {}
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    PGconn* conn_;
    mutable std::string last_error_;
    
    // Helper methods
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    bool executeQuery(const std::string& query);
    int getUserId(const std::string& username);
    std::string escapeString(const std::string& str);
};

} // namespace MillionaireGame

#endif // DATABASE_H

