/**
 * Unit tests for AuthManager
 * 
 * Tests authentication management including:
 * - Token generation
 * - Token registration and validation
 * - Password strength validation
 * - Authentication flow
 * - Admin role checking
 * - Concurrent authentication operations
 * 
 * Compile: make auth_manager_test
 * Run: ./bin/auth_manager_test
 */

#include "test_utils.h"
#include "../auth_manager.h"
#include "../session_manager.h"
#include "../json_utils.h"
#include "../logger.h"
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <regex>

using namespace std;
using namespace MillionaireGame;
using namespace MillionaireGame::Test;

// Helper to clean up auth state between tests
class AuthManagerTestHelper {
public:
    static void cleanup() {
        // Clean up session manager state
        auto& sessionMgr = SessionManager::getInstance();
        auto fds = sessionMgr.getAllClientFds();
        for (int fd : fds) {
            ClientSession* session = sessionMgr.getSession(fd);
            if (session && !session->auth_token.empty()) {
                AuthManager::getInstance().unregisterToken(session->auth_token, session->username);
            }
            sessionMgr.removeSession(fd);
        }
    }
};

// ============================================================================
// Test 1: Token Generation
// ============================================================================

void testTokenGeneration() {
    TEST_SECTION("Token Generation");
    
    auto& auth = AuthManager::getInstance();
    
    // Generate tokens
    string token1 = auth.generateToken();
    string token2 = auth.generateToken();
    string token3 = auth.generateToken();
    
    // Tokens should not be empty
    TEST_ASSERT(!token1.empty(), "Token 1 generated");
    TEST_ASSERT(!token2.empty(), "Token 2 generated");
    TEST_ASSERT(!token3.empty(), "Token 3 generated");
    
    // Tokens should be 32 characters (hex)
    TEST_ASSERT_EQ(token1.length(), (size_t)32, "Token 1 is 32 characters");
    TEST_ASSERT_EQ(token2.length(), (size_t)32, "Token 2 is 32 characters");
    TEST_ASSERT_EQ(token3.length(), (size_t)32, "Token 3 is 32 characters");
    
    // Tokens should be different (uniqueness)
    TEST_ASSERT_NEQ(token1, token2, "Token 1 != Token 2");
    TEST_ASSERT_NEQ(token2, token3, "Token 2 != Token 3");
    TEST_ASSERT_NEQ(token1, token3, "Token 1 != Token 3");
    
    // Tokens should be valid hex characters
    regex hex_pattern("^[0-9a-f]{32}$");
    TEST_ASSERT(regex_match(token1, hex_pattern), "Token 1 is valid hex");
    TEST_ASSERT(regex_match(token2, hex_pattern), "Token 2 is valid hex");
    TEST_ASSERT(regex_match(token3, hex_pattern), "Token 3 is valid hex");
}

// ============================================================================
// Test 2: Token Uniqueness (Large Scale)
// ============================================================================

void testTokenUniqueness() {
    TEST_SECTION("Token Uniqueness (Large Scale)");
    
    auto& auth = AuthManager::getInstance();
    
    const int num_tokens = 1000;
    set<string> tokens;
    
    for (int i = 0; i < num_tokens; i++) {
        string token = auth.generateToken();
        tokens.insert(token);
    }
    
    TEST_ASSERT_EQ(tokens.size(), (size_t)num_tokens, "All 1000 tokens are unique");
}

// ============================================================================
// Test 3: Token Registration and Validation
// ============================================================================

void testTokenRegistrationValidation() {
    TEST_SECTION("Token Registration and Validation");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    
    // Generate and register tokens
    string token1 = auth.generateToken();
    string token2 = auth.generateToken();
    
    int fd1 = 100;
    int fd2 = 101;
    
    auth.registerToken(token1, fd1, "user1");
    auth.registerToken(token2, fd2, "user2");
    
    // Validate registered tokens
    TEST_ASSERT(auth.validateToken(token1, fd1), "Token1 valid for fd1");
    TEST_ASSERT(auth.validateToken(token2, fd2), "Token2 valid for fd2");
    
    // Wrong FD should fail
    TEST_ASSERT_FALSE(auth.validateToken(token1, fd2), "Token1 invalid for fd2");
    TEST_ASSERT_FALSE(auth.validateToken(token2, fd1), "Token2 invalid for fd1");
    
    // Non-registered token should fail
    string fake_token = "00000000000000000000000000000000";
    TEST_ASSERT_FALSE(auth.validateToken(fake_token, fd1), "Fake token invalid");
    
    // Clean up
    auth.unregisterToken(token1, "user1");
    auth.unregisterToken(token2, "user2");
}

// ============================================================================
// Test 4: Token Unregistration
// ============================================================================

void testTokenUnregistration() {
    TEST_SECTION("Token Unregistration");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    
    string token = auth.generateToken();
    int fd = 200;
    
    // Register token
    auth.registerToken(token, fd, "test_user");
    TEST_ASSERT(auth.validateToken(token, fd), "Token valid after registration");
    
    // Unregister token
    auth.unregisterToken(token, "test_user");
    TEST_ASSERT_FALSE(auth.validateToken(token, fd), "Token invalid after unregistration");
    
    // Unregister non-existent token (should not crash)
    auth.unregisterToken("nonexistent", "nobody");
    TEST_ASSERT(true, "Unregistering non-existent token doesn't crash");
    
    // Unregister with empty token
    auth.unregisterToken("", "test_user");
    TEST_ASSERT(true, "Unregistering empty token doesn't crash");
    
    // Unregister with empty username
    auth.unregisterToken(token, "");
    TEST_ASSERT(true, "Unregistering with empty username doesn't crash");
}

// ============================================================================
// Test 5: Password Strength Validation
// ============================================================================

void testPasswordStrengthValidation() {
    TEST_SECTION("Password Strength Validation");
    
    auto& auth = AuthManager::getInstance();
    
    // Valid passwords (8+ chars, uppercase, lowercase, digit)
    TEST_ASSERT(auth.validatePasswordStrength("Password1"), "Password1 is valid");
    TEST_ASSERT(auth.validatePasswordStrength("Abcdefg1"), "Abcdefg1 is valid");
    TEST_ASSERT(auth.validatePasswordStrength("MyP@ssw0rd"), "MyP@ssw0rd is valid");
    TEST_ASSERT(auth.validatePasswordStrength("A1bcdefgh"), "A1bcdefgh is valid");
    TEST_ASSERT(auth.validatePasswordStrength("VeryLongPassword123"), "Long password is valid");
    
    // Invalid: too short
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("Pass1"), "Pass1 too short");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("Ab1"), "Ab1 too short");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("Abcdef1"), "Abcdef1 too short (7 chars)");
    
    // Invalid: missing uppercase
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("password1"), "password1 missing uppercase");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("abcdefgh1"), "abcdefgh1 missing uppercase");
    
    // Invalid: missing lowercase
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("PASSWORD1"), "PASSWORD1 missing lowercase");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("ABCDEFGH1"), "ABCDEFGH1 missing lowercase");
    
    // Invalid: missing digit
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("Passwordd"), "Passwordd missing digit");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("ABCDEfgh"), "ABCDEfgh missing digit");
    
    // Edge cases
    TEST_ASSERT(auth.validatePasswordStrength("Aaaaaaa1"), "Minimum valid: Aaaaaaa1");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength(""), "Empty password is invalid");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("12345678"), "Only digits is invalid");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("abcdefgh"), "Only lowercase is invalid");
    TEST_ASSERT_FALSE(auth.validatePasswordStrength("ABCDEFGH"), "Only uppercase is invalid");
}

// ============================================================================
// Test 6: RequireAuth Function
// ============================================================================

void testRequireAuth() {
    TEST_SECTION("RequireAuth Function");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    auto& sessionMgr = SessionManager::getInstance();
    
    // Setup: Create session and register token
    int fd = 300;
    sessionMgr.createSession(fd, "192.168.1.1");
    ClientSession* session = sessionMgr.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (!session) return;
    
    // Generate and register token
    string token = auth.generateToken();
    auth.registerToken(token, fd, "auth_test_user");
    session->auth_token = token;
    session->username = "auth_test_user";
    session->authenticated = true;
    
    // Test valid authentication
    string valid_request = "{\"requestType\":\"GET_PROFILE\",\"authToken\":\"" + token + "\",\"data\":{}}";
    string result = auth.requireAuth(valid_request, *session, fd);
    TEST_ASSERT_EQ(result, string("auth_test_user"), "Valid auth returns username");
    
    // Test missing token
    string no_token_request = "{\"requestType\":\"GET_PROFILE\",\"data\":{}}";
    result = auth.requireAuth(no_token_request, *session, fd);
    TEST_ASSERT(result.empty(), "Missing token returns empty");
    
    // Test invalid token
    string invalid_request = "{\"requestType\":\"GET_PROFILE\",\"authToken\":\"invalid_token_here\",\"data\":{}}";
    result = auth.requireAuth(invalid_request, *session, fd);
    TEST_ASSERT(result.empty(), "Invalid token returns empty");
    
    // Test wrong FD
    string other_fd_result = auth.requireAuth(valid_request, *session, fd + 1);
    TEST_ASSERT(other_fd_result.empty(), "Wrong FD returns empty");
    
    // Test token mismatch in session
    session->auth_token = "different_token";
    result = auth.requireAuth(valid_request, *session, fd);
    TEST_ASSERT(result.empty(), "Token mismatch returns empty");
    
    // Cleanup
    auth.unregisterToken(token, "auth_test_user");
    AuthManagerTestHelper::cleanup();
}

// ============================================================================
// Test 7: Admin Role Check
// ============================================================================

void testAdminRoleCheck() {
    TEST_SECTION("Admin Role Check");
    
    auto& auth = AuthManager::getInstance();
    
    // Note: Current implementation returns false for all users
    // This test documents expected behavior when database integration is complete
    TEST_ASSERT_FALSE(auth.isAdmin("regular_user"), "Regular user is not admin");
    TEST_ASSERT_FALSE(auth.isAdmin("admin"), "Admin check (needs DB integration)");
    TEST_ASSERT_FALSE(auth.isAdmin(""), "Empty username is not admin");
    TEST_ASSERT_FALSE(auth.isAdmin("nonexistent"), "Nonexistent user is not admin");
}

// ============================================================================
// Test 8: Concurrent Token Operations
// ============================================================================

void testConcurrentTokenOperations() {
    TEST_SECTION("Concurrent Token Operations");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    
    const int num_threads = 10;
    const int tokens_per_thread = 50;
    
    vector<thread> threads;
    vector<vector<string>> all_tokens(num_threads);
    atomic<int> validation_success(0);
    
    // Concurrent token generation and registration
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&auth, t, &all_tokens]() {
            for (int i = 0; i < tokens_per_thread; i++) {
                string token = auth.generateToken();
                int fd = 1000 + t * 100 + i;
                string username = "user_" + to_string(t) + "_" + to_string(i);
                
                auth.registerToken(token, fd, username);
                all_tokens[t].push_back(token);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all tokens were registered
    threads.clear();
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&auth, t, &all_tokens, &validation_success]() {
            for (int i = 0; i < tokens_per_thread; i++) {
                int fd = 1000 + t * 100 + i;
                if (auth.validateToken(all_tokens[t][i], fd)) {
                    validation_success++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int expected = num_threads * tokens_per_thread;
    TEST_ASSERT_EQ(validation_success.load(), expected, "All concurrent tokens validated");
    
    // Cleanup: unregister all tokens
    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < tokens_per_thread; i++) {
            string username = "user_" + to_string(t) + "_" + to_string(i);
            auth.unregisterToken(all_tokens[t][i], username);
        }
    }
    
    AuthManagerTestHelper::cleanup();
}

// ============================================================================
// Test 9: Token Format Validation
// ============================================================================

void testTokenFormat() {
    TEST_SECTION("Token Format");
    
    auto& auth = AuthManager::getInstance();
    
    // Generate many tokens and verify format
    for (int i = 0; i < 100; i++) {
        string token = auth.generateToken();
        
        // Length check
        bool length_ok = (token.length() == 32);
        
        // Hex character check
        bool hex_ok = true;
        for (char c : token) {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                hex_ok = false;
                break;
            }
        }
        
        if (!length_ok || !hex_ok) {
            TEST_ASSERT(false, "Token " + to_string(i) + " has invalid format: " + token);
            return;
        }
    }
    
    TEST_ASSERT(true, "All 100 tokens have valid format");
}

// ============================================================================
// Test 10: Session Token Integration
// ============================================================================

void testSessionTokenIntegration() {
    TEST_SECTION("Session Token Integration");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    auto& sessionMgr = SessionManager::getInstance();
    
    // Simulate login flow
    int fd = 400;
    string username = "integration_user";
    string ip = "192.168.1.50";
    
    // 1. Create session
    sessionMgr.createSession(fd, ip);
    ClientSession* session = sessionMgr.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created for login");
    
    if (!session) return;
    
    // 2. Generate and register token
    string token = auth.generateToken();
    auth.registerToken(token, fd, username);
    
    // 3. Update session with auth info
    session->auth_token = token;
    session->username = username;
    session->authenticated = true;
    
    // 4. Add user to online list
    sessionMgr.addOnlineUser(username);
    
    // 5. Verify complete auth state
    TEST_ASSERT(auth.validateToken(token, fd), "Token validates");
    TEST_ASSERT(sessionMgr.isUserOnline(username), "User is online");
    TEST_ASSERT(session->authenticated, "Session marked authenticated");
    
    // 6. Simulate logout/disconnect
    auth.unregisterToken(token, username);
    sessionMgr.removeSession(fd);  // This should also remove online user
    
    // 7. Verify cleanup
    TEST_ASSERT_FALSE(auth.validateToken(token, fd), "Token invalidated after logout");
    TEST_ASSERT_FALSE(sessionMgr.isUserOnline(username), "User offline after logout");
    TEST_ASSERT(sessionMgr.getSession(fd) == nullptr, "Session removed");
    
    AuthManagerTestHelper::cleanup();
}

// ============================================================================
// Test 11: Multiple Sessions Same User
// ============================================================================

void testMultipleSessionsSameUser() {
    TEST_SECTION("Multiple Sessions Same User");
    
    AuthManagerTestHelper::cleanup();
    auto& auth = AuthManager::getInstance();
    auto& sessionMgr = SessionManager::getInstance();
    
    string username = "multi_session_user";
    
    // Create first session
    int fd1 = 500;
    sessionMgr.createSession(fd1, "192.168.1.1");
    string token1 = auth.generateToken();
    auth.registerToken(token1, fd1, username);
    
    ClientSession* session1 = sessionMgr.getSession(fd1);
    if (session1) {
        session1->auth_token = token1;
        session1->username = username;
    }
    
    TEST_ASSERT(auth.validateToken(token1, fd1), "First session token valid");
    
    // Create second session for same user (new login from different device)
    int fd2 = 501;
    sessionMgr.createSession(fd2, "192.168.1.2");
    string token2 = auth.generateToken();
    auth.registerToken(token2, fd2, username);
    
    ClientSession* session2 = sessionMgr.getSession(fd2);
    if (session2) {
        session2->auth_token = token2;
        session2->username = username;
    }
    
    // Both tokens should be valid
    TEST_ASSERT(auth.validateToken(token1, fd1), "First token still valid");
    TEST_ASSERT(auth.validateToken(token2, fd2), "Second token valid");
    
    // Tokens should not be interchangeable
    TEST_ASSERT_FALSE(auth.validateToken(token1, fd2), "Token1 invalid for fd2");
    TEST_ASSERT_FALSE(auth.validateToken(token2, fd1), "Token2 invalid for fd1");
    
    // Cleanup
    auth.unregisterToken(token1, username);
    auth.unregisterToken(token2, username);
    AuthManagerTestHelper::cleanup();
}

// ============================================================================
// Test 12: Password Edge Cases
// ============================================================================

void testPasswordEdgeCases() {
    TEST_SECTION("Password Edge Cases");
    
    auto& auth = AuthManager::getInstance();
    
    // Special characters
    TEST_ASSERT(auth.validatePasswordStrength("P@ssw0rd!"), "Special chars allowed");
    TEST_ASSERT(auth.validatePasswordStrength("Test#123$"), "Multiple special chars");
    TEST_ASSERT(auth.validatePasswordStrength("User_Pass1"), "Underscore allowed");
    
    // Unicode (depends on implementation)
    // These might pass or fail depending on how unicode is handled
    
    // Spaces
    TEST_ASSERT(auth.validatePasswordStrength("Pass 123A"), "Space in password");
    
    // Very long password
    string long_pass = "Aa1";
    for (int i = 0; i < 100; i++) long_pass += "x";
    TEST_ASSERT(auth.validatePasswordStrength(long_pass), "Very long password valid");
    
    // Numbers at different positions
    TEST_ASSERT(auth.validatePasswordStrength("1Abcdefg"), "Digit at start");
    TEST_ASSERT(auth.validatePasswordStrength("Abcdefg1"), "Digit at end");
    TEST_ASSERT(auth.validatePasswordStrength("Abcd1efg"), "Digit in middle");
    
    // Multiple digits
    TEST_ASSERT(auth.validatePasswordStrength("Pass1234"), "Multiple digits");
    TEST_ASSERT(auth.validatePasswordStrength("A1234567b"), "Many digits");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║           AuthManager Unit Tests                           ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    // Initialize logger (suppress output during tests)
    Logger::getInstance().initialize("", LogLevel::ERROR);
    
    // Reset test statistics
    getTestStats().reset();
    
    // Initial cleanup
    AuthManagerTestHelper::cleanup();
    
    // Run all tests
    RUN_TEST(testTokenGeneration);
    RUN_TEST(testTokenUniqueness);
    RUN_TEST(testTokenRegistrationValidation);
    RUN_TEST(testTokenUnregistration);
    RUN_TEST(testPasswordStrengthValidation);
    RUN_TEST(testRequireAuth);
    RUN_TEST(testAdminRoleCheck);
    RUN_TEST(testConcurrentTokenOperations);
    RUN_TEST(testTokenFormat);
    RUN_TEST(testSessionTokenIntegration);
    RUN_TEST(testMultipleSessionsSameUser);
    RUN_TEST(testPasswordEdgeCases);
    
    // Final cleanup
    AuthManagerTestHelper::cleanup();
    
    // Print summary
    getTestStats().printSummary();
    
    return getTestStats().exitCode();
}
