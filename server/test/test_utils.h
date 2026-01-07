/**
 * Test Utilities Header
 * Common utilities, macros, and mock objects for unit testing
 * 
 * Usage:
 *   #include "test_utils.h"
 *   
 *   void testSomething() {
 *       TEST_ASSERT(condition, "Description");
 *   }
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace MillionaireGame {
namespace Test {

// ============================================================================
// Test Statistics
// ============================================================================

struct TestStats {
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failed_tests;
    
    void reset() {
        passed = 0;
        failed = 0;
        failed_tests.clear();
    }
    
    void printSummary() const {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Total:  " << (passed + failed) << std::endl;
        
        if (!failed_tests.empty()) {
            std::cout << "\nFailed tests:" << std::endl;
            for (const auto& test : failed_tests) {
                std::cout << "  - " << test << std::endl;
            }
        }
        
        if (failed == 0) {
            std::cout << "\n✓ All tests passed!" << std::endl;
        } else {
            std::cout << "\n✗ Some tests failed!" << std::endl;
        }
    }
    
    int exitCode() const {
        return failed == 0 ? 0 : 1;
    }
};

// Global test stats instance
inline TestStats& getTestStats() {
    static TestStats stats;
    return stats;
}

// ============================================================================
// Test Macros
// ============================================================================

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            std::cout << "  ✓ PASS: " << message << std::endl; \
            MillionaireGame::Test::getTestStats().passed++; \
        } else { \
            std::cout << "  ✗ FAIL: " << message << std::endl; \
            MillionaireGame::Test::getTestStats().failed++; \
            MillionaireGame::Test::getTestStats().failed_tests.push_back(message); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            std::cout << "  ✓ PASS: " << message << std::endl; \
            MillionaireGame::Test::getTestStats().passed++; \
        } else { \
            std::cout << "  ✗ FAIL: " << message << " (expected: " << (expected) << ", got: " << (actual) << ")" << std::endl; \
            MillionaireGame::Test::getTestStats().failed++; \
            MillionaireGame::Test::getTestStats().failed_tests.push_back(message); \
        } \
    } while(0)

#define TEST_ASSERT_NEQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            std::cout << "  ✓ PASS: " << message << std::endl; \
            MillionaireGame::Test::getTestStats().passed++; \
        } else { \
            std::cout << "  ✗ FAIL: " << message << " (should not equal: " << (expected) << ")" << std::endl; \
            MillionaireGame::Test::getTestStats().failed++; \
            MillionaireGame::Test::getTestStats().failed_tests.push_back(message); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition, message) TEST_ASSERT((condition), message)
#define TEST_ASSERT_FALSE(condition, message) TEST_ASSERT(!(condition), message)

#define TEST_SECTION(name) \
    std::cout << "\n=== " << name << " ===" << std::endl

#define RUN_TEST(test_func) \
    do { \
        std::cout << "\n--- Running: " << #test_func << " ---" << std::endl; \
        try { \
            test_func(); \
        } catch (const std::exception& e) { \
            std::cout << "  ✗ EXCEPTION: " << e.what() << std::endl; \
            MillionaireGame::Test::getTestStats().failed++; \
            MillionaireGame::Test::getTestStats().failed_tests.push_back(std::string(#test_func) + " (exception)"); \
        } catch (...) { \
            std::cout << "  ✗ UNKNOWN EXCEPTION" << std::endl; \
            MillionaireGame::Test::getTestStats().failed++; \
            MillionaireGame::Test::getTestStats().failed_tests.push_back(std::string(#test_func) + " (unknown exception)"); \
        } \
    } while(0)

// ============================================================================
// Socket Test Utilities
// ============================================================================

/**
 * Create a pair of connected sockets for testing
 * @return pair of socket file descriptors, {-1, -1} on error
 */
inline std::pair<int, int> createSocketPair() {
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        return {-1, -1};
    }
    return {sockets[0], sockets[1]};
}

/**
 * Close a socket pair safely
 */
inline void closeSocketPair(std::pair<int, int>& sockets) {
    if (sockets.first >= 0) {
        close(sockets.first);
        sockets.first = -1;
    }
    if (sockets.second >= 0) {
        close(sockets.second);
        sockets.second = -1;
    }
}

/**
 * Send raw data to a socket
 */
inline ssize_t sendRawData(int socket_fd, const std::string& data) {
    return send(socket_fd, data.c_str(), data.length(), 0);
}

/**
 * Receive raw data from a socket
 */
inline std::string receiveRawData(int socket_fd, size_t max_size = 4096) {
    char buffer[4096];
    size_t read_size = std::min(max_size, sizeof(buffer));
    ssize_t bytes_read = recv(socket_fd, buffer, read_size, 0);
    if (bytes_read <= 0) {
        return "";
    }
    return std::string(buffer, bytes_read);
}

// ============================================================================
// Mock Database
// ============================================================================

/**
 * Mock user data for testing
 */
struct MockUser {
    int user_id;
    std::string username;
    std::string password_hash;  // In real tests, store hashed passwords
    std::string email;
    std::string role;           // "user" or "admin"
    bool is_active;
    int total_score;
    int games_played;
    
    MockUser() : user_id(0), role("user"), is_active(true), total_score(0), games_played(0) {}
    
    MockUser(int id, const std::string& uname, const std::string& pwd, 
             const std::string& mail = "", const std::string& r = "user")
        : user_id(id), username(uname), password_hash(pwd), email(mail),
          role(r), is_active(true), total_score(0), games_played(0) {}
};

/**
 * Mock Database class for testing
 * Simulates database operations without actual database connection
 */
class MockDatabase {
public:
    static MockDatabase& getInstance() {
        static MockDatabase instance;
        return instance;
    }
    
    void reset() {
        users_.clear();
        next_user_id_ = 1;
        setupDefaultUsers();
    }
    
    // User operations
    bool addUser(const std::string& username, const std::string& password, 
                 const std::string& email = "") {
        if (findUserByUsername(username) != nullptr) {
            return false;  // User already exists
        }
        
        MockUser user(next_user_id_++, username, password, email);
        users_.push_back(user);
        return true;
    }
    
    MockUser* findUserByUsername(const std::string& username) {
        for (auto& user : users_) {
            if (user.username == username) {
                return &user;
            }
        }
        return nullptr;
    }
    
    MockUser* findUserById(int user_id) {
        for (auto& user : users_) {
            if (user.user_id == user_id) {
                return &user;
            }
        }
        return nullptr;
    }
    
    bool validateCredentials(const std::string& username, const std::string& password) {
        MockUser* user = findUserByUsername(username);
        if (user == nullptr) {
            return false;
        }
        // Simple comparison for testing (in production, use proper hash comparison)
        return user->password_hash == password && user->is_active;
    }
    
    bool updateUserScore(const std::string& username, int score) {
        MockUser* user = findUserByUsername(username);
        if (user == nullptr) {
            return false;
        }
        user->total_score += score;
        user->games_played++;
        return true;
    }
    
    bool setUserRole(const std::string& username, const std::string& role) {
        MockUser* user = findUserByUsername(username);
        if (user == nullptr) {
            return false;
        }
        user->role = role;
        return true;
    }
    
    bool deactivateUser(const std::string& username) {
        MockUser* user = findUserByUsername(username);
        if (user == nullptr) {
            return false;
        }
        user->is_active = false;
        return true;
    }
    
    std::vector<MockUser> getAllUsers() const {
        return users_;
    }
    
    size_t getUserCount() const {
        return users_.size();
    }

private:
    MockDatabase() {
        setupDefaultUsers();
    }
    
    void setupDefaultUsers() {
        // Add some default test users
        users_.clear();
        next_user_id_ = 1;
        
        addUser("testuser", "Password123", "test@example.com");
        addUser("admin", "AdminPass123", "admin@example.com");
        setUserRole("admin", "admin");
        addUser("player1", "Player123", "player1@example.com");
        addUser("player2", "Player123", "player2@example.com");
        addUser("inactive_user", "Inactive123", "inactive@example.com");
        deactivateUser("inactive_user");
    }
    
    std::vector<MockUser> users_;
    int next_user_id_ = 1;
};

// ============================================================================
// Test Timer Utility
// ============================================================================

/**
 * Simple timer for measuring test execution time
 */
class TestTimer {
public:
    TestTimer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    double elapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
    double elapsedSec() const {
        return elapsedMs() / 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// ============================================================================
// JSON Test Utilities
// ============================================================================

/**
 * Create a test JSON request
 */
inline std::string createTestRequest(const std::string& request_type, 
                                     const std::string& data = "{}") {
    return "{\"requestType\":\"" + request_type + "\",\"data\":" + data + "}";
}

/**
 * Create a test JSON request with auth token
 */
inline std::string createAuthenticatedRequest(const std::string& request_type,
                                              const std::string& auth_token,
                                              const std::string& data = "{}") {
    return "{\"requestType\":\"" + request_type + 
           "\",\"authToken\":\"" + auth_token + 
           "\",\"data\":" + data + "}";
}

/**
 * Create login request JSON
 */
inline std::string createLoginRequest(const std::string& username, 
                                      const std::string& password) {
    return "{\"requestType\":\"LOGIN\",\"data\":{\"username\":\"" + username + 
           "\",\"password\":\"" + password + "\"}}";
}

/**
 * Create register request JSON
 */
inline std::string createRegisterRequest(const std::string& username,
                                         const std::string& password,
                                         const std::string& email = "") {
    std::string data = "{\"username\":\"" + username + 
                       "\",\"password\":\"" + password + "\"";
    if (!email.empty()) {
        data += ",\"email\":\"" + email + "\"";
    }
    data += "}";
    return "{\"requestType\":\"REGISTER\",\"data\":" + data + "}";
}

} // namespace Test
} // namespace MillionaireGame

#endif // TEST_UTILS_H
