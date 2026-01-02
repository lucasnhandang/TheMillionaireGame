#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>

namespace MillionaireGame {

// Question structure
struct Question {
    int id;
    std::string question_text;
    std::string option_a;
    std::string option_b;
    std::string option_c;
    std::string option_d;
    int correct_answer;  // 0=A, 1=B, 2=C, 3=D
    int level;          // 1-15
    
    Question() : id(0), correct_answer(0), level(0) {}
};

/**
 * Database Module
 * Singleton class for database operations
 */
class Database {
public:
    static Database& getInstance();
    
    // Connection
    bool connect(const std::string& host, int port, 
                 const std::string& dbname, const std::string& user, 
                 const std::string& password);
    bool isConnected() const;
    void disconnect();
    
    // Question operations (priority for game functionality)
    Question getRandomQuestion(int level);
    std::vector<Question> getQuestions(int level, int limit = 1);
    bool questionExists(int question_id);
    
    // User operations (basic)
    bool authenticateUser(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    std::string getUserRole(const std::string& username);
    bool userExists(const std::string& username);
    
private:
    Database() : connected_(false) {}
    ~Database() { disconnect(); }
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    bool connected_;
    void* conn_;  // PGconn* for libpq (using void* to avoid including libpq-fe.h in header)
    
    // Helper methods
    std::string escapeString(const std::string& str);
};

} // namespace MillionaireGame

#endif // DATABASE_H

