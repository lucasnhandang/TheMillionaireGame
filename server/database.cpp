#include "database.h"
#include "logger.h"
#include <libpq-fe.h>
#include <cstdlib>
#include <random>
#include <sstream>

using namespace std;

namespace MillionaireGame {

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::connect(const string& host, int port, 
                       const string& dbname, const string& user, 
                       const string& password) {
    if (connected_) {
        disconnect();
    }
    
    ostringstream conn_str;
    conn_str << "host=" << host 
             << " port=" << port 
             << " dbname=" << dbname 
             << " user=" << user 
             << " password=" << password;
    
    PGconn* pg_conn = PQconnectdb(conn_str.str().c_str());
    
    if (PQstatus(pg_conn) != CONNECTION_OK) {
        string error = PQerrorMessage(pg_conn);
        LOG_ERROR("Database connection failed: " + error);
        PQfinish(pg_conn);
        return false;
    }
    
    conn_ = pg_conn;
    connected_ = true;
    LOG_INFO("Database connected successfully");
    return true;
}

bool Database::isConnected() const {
    return connected_ && conn_ != nullptr && PQstatus((PGconn*)conn_) == CONNECTION_OK;
}

void Database::disconnect() {
    if (conn_ != nullptr) {
        PQfinish((PGconn*)conn_);
        conn_ = nullptr;
    }
    connected_ = false;
}

string Database::escapeString(const string& str) {
    if (!isConnected()) return "";
    
    char* escaped = PQescapeLiteral((PGconn*)conn_, str.c_str(), str.length());
    if (escaped == nullptr) {
        return "";
    }
    
    string result(escaped);
    PQfreemem(escaped);
    return result;
}

Question Database::getRandomQuestion(int level) {
    Question q;
    
    if (!isConnected()) {
        LOG_ERROR("Database not connected");
        return q;
    }
    
    // Get all questions for this level
    vector<Question> questions = getQuestions(level, 100);
    
    if (questions.empty()) {
        LOG_WARNING("No questions found for level " + to_string(level));
        return q;
    }
    
    // Return random question
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, questions.size() - 1);
    
    return questions[dis(gen)];
}

vector<Question> Database::getQuestions(int level, int limit) {
    vector<Question> questions;
    
    if (!isConnected()) {
        LOG_ERROR("Database not connected");
        return questions;
    }
    
    ostringstream query;
    query << "SELECT id, question_text, option_a, option_b, option_c, option_d, "
          << "correct_answer, level FROM questions WHERE level = " << level
          << " ORDER BY RANDOM() LIMIT " << limit;
    
    LOG_INFO("Executing query: " + query.str());
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        string error = PQerrorMessage((PGconn*)conn_);
        LOG_ERROR("Query failed: " + error);
        PQclear(res);
        return questions;
    }
    
    int rows = PQntuples(res);
    LOG_INFO("Query returned " + to_string(rows) + " rows for level " + to_string(level));
    for (int i = 0; i < rows; i++) {
        Question q;
        q.id = atoi(PQgetvalue(res, i, 0));
        q.question_text = PQgetvalue(res, i, 1);
        q.option_a = PQgetvalue(res, i, 2);
        q.option_b = PQgetvalue(res, i, 3);
        q.option_c = PQgetvalue(res, i, 4);
        q.option_d = PQgetvalue(res, i, 5);
        q.correct_answer = atoi(PQgetvalue(res, i, 6));
        q.level = atoi(PQgetvalue(res, i, 7));
        questions.push_back(q);
    }
    
    PQclear(res);
    return questions;
}

bool Database::questionExists(int question_id) {
    if (!isConnected()) {
        return false;
    }
    
    ostringstream query;
    query << "SELECT COUNT(*) FROM questions WHERE id = " << question_id;
    
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return count > 0;
}

bool Database::authenticateUser(const string& username, const string& password) {
    if (!isConnected()) {
        return false;
    }
    
    // TODO: Implement password hashing check
    // For now, just check if user exists
    string escaped_username = escapeString(username);
    if (escaped_username.empty()) {
        return false;
    }
    
    ostringstream query;
    query << "SELECT password_hash FROM users WHERE username = " << escaped_username;
    
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    bool exists = PQntuples(res) > 0;
    PQclear(res);
    
    return exists;
}

bool Database::registerUser(const string& username, const string& password) {
    if (!isConnected()) {
        return false;
    }
    
    // TODO: Hash password before storing
    string escaped_username = escapeString(username);
    string escaped_password = escapeString(password);
    
    if (escaped_username.empty() || escaped_password.empty()) {
        return false;
    }
    
    ostringstream query;
    query << "INSERT INTO users (username, password_hash) VALUES ("
          << escaped_username << ", " << escaped_password << ")";
    
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        string error = PQerrorMessage((PGconn*)conn_);
        LOG_ERROR("Registration failed: " + error);
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

string Database::getUserRole(const string& username) {
    if (!isConnected()) {
        return "user";
    }
    
    string escaped_username = escapeString(username);
    if (escaped_username.empty()) {
        return "user";
    }
    
    ostringstream query;
    query << "SELECT role FROM users WHERE username = " << escaped_username;
    
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return "user";
    }
    
    string role = PQgetvalue(res, 0, 0);
    PQclear(res);
    
    return role.empty() ? "user" : role;
}

bool Database::userExists(const string& username) {
    if (!isConnected()) {
        return false;
    }
    
    string escaped_username = escapeString(username);
    if (escaped_username.empty()) {
        return false;
    }
    
    ostringstream query;
    query << "SELECT COUNT(*) FROM users WHERE username = " << escaped_username;
    
    PGresult* res = PQexec((PGconn*)conn_, query.str().c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return false;
    }
    
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return count > 0;
}

} // namespace MillionaireGame

