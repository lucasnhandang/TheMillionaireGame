/**
 * Unit tests for SessionManager
 * 
 * Tests session management including:
 * - Session creation and retrieval
 * - Session removal
 * - Online user tracking
 * - Ping time updates
 * - Concurrent session operations
 * - Session data manipulation
 * - Client count tracking
 * 
 * Compile: make session_manager_test
 * Run: ./bin/session_manager_test
 */

#include "test_utils.h"
#include "../session_manager.h"
#include "../logger.h"
#include <thread>
#include <chrono>
#include <vector>
#include <set>

using namespace std;
using namespace MillionaireGame;
using namespace MillionaireGame::Test;

// Helper to reset SessionManager state between tests
// Note: Since SessionManager is a singleton, we need to clean up properly
class SessionManagerTestHelper {
public:
    static void cleanupAllSessions() {
        auto& manager = SessionManager::getInstance();
        auto fds = manager.getAllClientFds();
        for (int fd : fds) {
            manager.removeSession(fd);
        }
    }
};

// ============================================================================
// Test 1: Session Creation
// ============================================================================

void testSessionCreation() {
    TEST_SECTION("Session Creation");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Create a session
    int fake_fd = 100;
    string client_ip = "192.168.1.100";
    
    manager.createSession(fake_fd, client_ip);
    
    // Verify session exists
    ClientSession* session = manager.getSession(fake_fd);
    TEST_ASSERT(session != nullptr, "Session created and retrievable");
    
    if (session) {
        TEST_ASSERT_EQ(session->client_ip, client_ip, "Client IP stored correctly");
        TEST_ASSERT_FALSE(session->authenticated, "New session is not authenticated");
        TEST_ASSERT_FALSE(session->in_game, "New session is not in game");
        TEST_ASSERT_EQ(session->game_id, 0, "Game ID is 0");
        TEST_ASSERT_EQ(session->current_question_number, 0, "Question number is 0");
        TEST_ASSERT_EQ(session->total_score, 0, "Total score is 0");
        TEST_ASSERT_EQ(session->role, string("user"), "Default role is user");
        TEST_ASSERT(session->used_lifelines.empty(), "No lifelines used initially");
    }
    
    // Verify client count
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)1, "Client count is 1");
    
    // Create multiple sessions
    manager.createSession(101, "192.168.1.101");
    manager.createSession(102, "192.168.1.102");
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)3, "Client count is 3 after adding more");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 2: Session Retrieval
// ============================================================================

void testSessionRetrieval() {
    TEST_SECTION("Session Retrieval");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Create sessions
    manager.createSession(200, "10.0.0.1");
    manager.createSession(201, "10.0.0.2");
    manager.createSession(202, "10.0.0.3");
    
    // Retrieve existing sessions
    ClientSession* session1 = manager.getSession(200);
    ClientSession* session2 = manager.getSession(201);
    ClientSession* session3 = manager.getSession(202);
    
    TEST_ASSERT(session1 != nullptr, "Session 200 retrieved");
    TEST_ASSERT(session2 != nullptr, "Session 201 retrieved");
    TEST_ASSERT(session3 != nullptr, "Session 202 retrieved");
    
    if (session1) TEST_ASSERT_EQ(session1->client_ip, string("10.0.0.1"), "Session 1 has correct IP");
    if (session2) TEST_ASSERT_EQ(session2->client_ip, string("10.0.0.2"), "Session 2 has correct IP");
    if (session3) TEST_ASSERT_EQ(session3->client_ip, string("10.0.0.3"), "Session 3 has correct IP");
    
    // Retrieve non-existing session
    ClientSession* nonexistent = manager.getSession(999);
    TEST_ASSERT(nonexistent == nullptr, "Non-existing session returns nullptr");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 3: Session Removal
// ============================================================================

void testSessionRemoval() {
    TEST_SECTION("Session Removal");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Create sessions
    manager.createSession(300, "172.16.0.1");
    manager.createSession(301, "172.16.0.2");
    manager.createSession(302, "172.16.0.3");
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)3, "3 sessions created");
    
    // Remove middle session
    manager.removeSession(301);
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)2, "2 sessions after removal");
    TEST_ASSERT(manager.getSession(300) != nullptr, "Session 300 still exists");
    TEST_ASSERT(manager.getSession(301) == nullptr, "Session 301 removed");
    TEST_ASSERT(manager.getSession(302) != nullptr, "Session 302 still exists");
    
    // Remove remaining sessions
    manager.removeSession(300);
    manager.removeSession(302);
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)0, "0 sessions after removing all");
    
    // Remove non-existing session (should not crash)
    manager.removeSession(999);
    TEST_ASSERT(true, "Removing non-existing session doesn't crash");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 4: Online User Tracking
// ============================================================================

void testOnlineUserTracking() {
    TEST_SECTION("Online User Tracking");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Initially no users online
    TEST_ASSERT_FALSE(manager.isUserOnline("testuser1"), "testuser1 not online initially");
    TEST_ASSERT_FALSE(manager.isUserOnline("testuser2"), "testuser2 not online initially");
    
    // Add online users
    manager.addOnlineUser("testuser1");
    TEST_ASSERT(manager.isUserOnline("testuser1"), "testuser1 is online after adding");
    TEST_ASSERT_FALSE(manager.isUserOnline("testuser2"), "testuser2 still not online");
    
    manager.addOnlineUser("testuser2");
    TEST_ASSERT(manager.isUserOnline("testuser1"), "testuser1 still online");
    TEST_ASSERT(manager.isUserOnline("testuser2"), "testuser2 is online after adding");
    
    // Remove online user
    manager.removeOnlineUser("testuser1");
    TEST_ASSERT_FALSE(manager.isUserOnline("testuser1"), "testuser1 not online after removal");
    TEST_ASSERT(manager.isUserOnline("testuser2"), "testuser2 still online");
    
    // Add same user again (idempotent)
    manager.addOnlineUser("testuser2");
    TEST_ASSERT(manager.isUserOnline("testuser2"), "testuser2 still online after re-adding");
    
    // Remove non-existing user (should not crash)
    manager.removeOnlineUser("nonexistent");
    TEST_ASSERT(true, "Removing non-existing user doesn't crash");
    
    // Cleanup
    manager.removeOnlineUser("testuser2");
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 5: Session with Online User Cleanup
// ============================================================================

void testSessionOnlineUserCleanup() {
    TEST_SECTION("Session Online User Cleanup");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Create session and set username
    int fd = 400;
    manager.createSession(fd, "192.168.0.1");
    
    ClientSession* session = manager.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (session) {
        session->username = "cleanup_test_user";
        session->authenticated = true;
        manager.addOnlineUser("cleanup_test_user");
    }
    
    TEST_ASSERT(manager.isUserOnline("cleanup_test_user"), "User is online");
    
    // Remove session - should also remove online user
    manager.removeSession(fd);
    
    TEST_ASSERT_FALSE(manager.isUserOnline("cleanup_test_user"), "User removed from online list when session removed");
    TEST_ASSERT(manager.getSession(fd) == nullptr, "Session removed");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 6: Ping Time Updates
// ============================================================================

void testPingTimeUpdates() {
    TEST_SECTION("Ping Time Updates");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    int fd = 500;
    manager.createSession(fd, "192.168.0.100");
    
    ClientSession* session = manager.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (session) {
        time_t initial_ping = session->last_ping_time;
        
        // Wait a bit and update ping time
        this_thread::sleep_for(chrono::seconds(1));
        
        manager.updatePingTime(fd);
        
        time_t updated_ping = session->last_ping_time;
        TEST_ASSERT(updated_ping >= initial_ping, "Ping time updated");
        TEST_ASSERT(updated_ping - initial_ping >= 1, "Ping time increased by at least 1 second");
    }
    
    // Update ping for non-existing session (should not crash)
    manager.updatePingTime(999);
    TEST_ASSERT(true, "Updating ping for non-existing session doesn't crash");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 7: Get All Client FDs
// ============================================================================

void testGetAllClientFds() {
    TEST_SECTION("Get All Client FDs");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    // Initially empty
    vector<int> fds = manager.getAllClientFds();
    TEST_ASSERT(fds.empty(), "No FDs initially");
    
    // Add sessions
    set<int> expected_fds = {600, 601, 602, 603, 604};
    for (int fd : expected_fds) {
        manager.createSession(fd, "10.0.0." + to_string(fd - 600));
    }
    
    fds = manager.getAllClientFds();
    TEST_ASSERT_EQ(fds.size(), expected_fds.size(), "Correct number of FDs returned");
    
    // Verify all FDs are present
    set<int> returned_fds(fds.begin(), fds.end());
    TEST_ASSERT(returned_fds == expected_fds, "All expected FDs returned");
    
    // Remove one and verify
    manager.removeSession(602);
    fds = manager.getAllClientFds();
    TEST_ASSERT_EQ(fds.size(), (size_t)4, "4 FDs after removal");
    
    set<int> after_removal(fds.begin(), fds.end());
    TEST_ASSERT(after_removal.find(602) == after_removal.end(), "Removed FD not in list");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 8: Session Data Modification
// ============================================================================

void testSessionDataModification() {
    TEST_SECTION("Session Data Modification");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    int fd = 700;
    manager.createSession(fd, "127.0.0.1");
    
    ClientSession* session = manager.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (session) {
        // Modify session data
        session->username = "test_player";
        session->auth_token = "abc123def456";
        session->authenticated = true;
        session->role = "admin";
        session->in_game = true;
        session->game_id = 12345;
        session->current_question_number = 5;
        session->current_level = 3;
        session->current_prize = 10000;
        session->total_score = 25000;
        session->used_lifelines.insert("50:50");
        session->used_lifelines.insert("phone_friend");
        
        // Retrieve and verify
        ClientSession* retrieved = manager.getSession(fd);
        TEST_ASSERT(retrieved != nullptr, "Session still retrievable");
        
        if (retrieved) {
            TEST_ASSERT_EQ(retrieved->username, string("test_player"), "Username stored");
            TEST_ASSERT_EQ(retrieved->auth_token, string("abc123def456"), "Auth token stored");
            TEST_ASSERT(retrieved->authenticated, "Authenticated flag stored");
            TEST_ASSERT_EQ(retrieved->role, string("admin"), "Role stored");
            TEST_ASSERT(retrieved->in_game, "In game flag stored");
            TEST_ASSERT_EQ(retrieved->game_id, 12345, "Game ID stored");
            TEST_ASSERT_EQ(retrieved->current_question_number, 5, "Question number stored");
            TEST_ASSERT_EQ(retrieved->current_level, 3, "Level stored");
            TEST_ASSERT_EQ(retrieved->current_prize, 10000, "Prize stored");
            TEST_ASSERT_EQ(retrieved->total_score, 25000, "Score stored");
            TEST_ASSERT_EQ(retrieved->used_lifelines.size(), (size_t)2, "Lifelines stored");
            TEST_ASSERT(retrieved->used_lifelines.count("50:50") == 1, "50:50 lifeline recorded");
            TEST_ASSERT(retrieved->used_lifelines.count("phone_friend") == 1, "Phone friend lifeline recorded");
        }
    }
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 9: Concurrent Session Operations
// ============================================================================

void testConcurrentSessionOperations() {
    TEST_SECTION("Concurrent Session Operations");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    const int num_threads = 10;
    const int sessions_per_thread = 20;
    atomic<int> success_count(0);
    
    // Launch multiple threads creating sessions
    vector<thread> threads;
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&manager, t, &success_count]() {
            for (int i = 0; i < sessions_per_thread; i++) {
                int fd = 1000 + t * 100 + i;
                string ip = "10." + to_string(t) + ".0." + to_string(i);
                
                manager.createSession(fd, ip);
                
                // Verify session exists
                if (manager.getSession(fd) != nullptr) {
                    success_count++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    int expected_total = num_threads * sessions_per_thread;
    TEST_ASSERT_EQ(success_count.load(), expected_total, "All concurrent creates succeeded");
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)expected_total, "Correct total client count");
    
    // Concurrent removal
    threads.clear();
    atomic<int> remove_count(0);
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&manager, t, &remove_count]() {
            for (int i = 0; i < sessions_per_thread; i++) {
                int fd = 1000 + t * 100 + i;
                manager.removeSession(fd);
                remove_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)0, "All sessions removed concurrently");
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 10: Session Timestamp Validation
// ============================================================================

void testSessionTimestamps() {
    TEST_SECTION("Session Timestamps");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    time_t before_create = time(nullptr);
    
    int fd = 800;
    manager.createSession(fd, "192.168.1.1");
    
    time_t after_create = time(nullptr);
    
    ClientSession* session = manager.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (session) {
        TEST_ASSERT(session->connected_time >= before_create, "Connected time >= before create");
        TEST_ASSERT(session->connected_time <= after_create, "Connected time <= after create");
        TEST_ASSERT(session->last_ping_time >= before_create, "Ping time >= before create");
        TEST_ASSERT(session->last_ping_time <= after_create, "Ping time <= after create");
    }
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 11: Multiple Users Same IP
// ============================================================================

void testMultipleUsersSameIP() {
    TEST_SECTION("Multiple Users Same IP");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    string shared_ip = "192.168.1.100";
    
    // Create multiple sessions from same IP (different FDs)
    manager.createSession(900, shared_ip);
    manager.createSession(901, shared_ip);
    manager.createSession(902, shared_ip);
    
    TEST_ASSERT_EQ(manager.getClientCount(), (size_t)3, "3 sessions created from same IP");
    
    // Each session should be distinct
    ClientSession* s1 = manager.getSession(900);
    ClientSession* s2 = manager.getSession(901);
    ClientSession* s3 = manager.getSession(902);
    
    TEST_ASSERT(s1 != nullptr, "Session 1 exists");
    TEST_ASSERT(s2 != nullptr, "Session 2 exists");
    TEST_ASSERT(s3 != nullptr, "Session 3 exists");
    
    if (s1 && s2 && s3) {
        TEST_ASSERT_EQ(s1->client_ip, shared_ip, "Session 1 has shared IP");
        TEST_ASSERT_EQ(s2->client_ip, shared_ip, "Session 2 has shared IP");
        TEST_ASSERT_EQ(s3->client_ip, shared_ip, "Session 3 has shared IP");
        
        // Set different usernames
        s1->username = "user1";
        s2->username = "user2";
        s3->username = "user3";
        
        // All sessions should be independent
        TEST_ASSERT_EQ(manager.getSession(900)->username, string("user1"), "Session 1 has user1");
        TEST_ASSERT_EQ(manager.getSession(901)->username, string("user2"), "Session 2 has user2");
        TEST_ASSERT_EQ(manager.getSession(902)->username, string("user3"), "Session 3 has user3");
    }
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Test 12: Lifeline Tracking
// ============================================================================

void testLifelineTracking() {
    TEST_SECTION("Lifeline Tracking");
    
    SessionManagerTestHelper::cleanupAllSessions();
    auto& manager = SessionManager::getInstance();
    
    int fd = 1000;
    manager.createSession(fd, "127.0.0.1");
    
    ClientSession* session = manager.getSession(fd);
    TEST_ASSERT(session != nullptr, "Session created");
    
    if (session) {
        // Initially no lifelines used
        TEST_ASSERT(session->used_lifelines.empty(), "No lifelines used initially");
        TEST_ASSERT(session->used_lifelines.find("50:50") == session->used_lifelines.end(), 
                   "50:50 not used");
        
        // Use lifelines
        session->used_lifelines.insert("50:50");
        TEST_ASSERT_EQ(session->used_lifelines.size(), (size_t)1, "1 lifeline used");
        TEST_ASSERT(session->used_lifelines.find("50:50") != session->used_lifelines.end(), 
                   "50:50 is used");
        
        session->used_lifelines.insert("phone_friend");
        session->used_lifelines.insert("ask_audience");
        TEST_ASSERT_EQ(session->used_lifelines.size(), (size_t)3, "3 lifelines used");
        
        // Try to insert same lifeline again
        session->used_lifelines.insert("50:50");
        TEST_ASSERT_EQ(session->used_lifelines.size(), (size_t)3, "Still 3 lifelines (no duplicates)");
    }
    
    SessionManagerTestHelper::cleanupAllSessions();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║          SessionManager Unit Tests                         ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    // Initialize logger (suppress output during tests)
    Logger::getInstance().initialize("", LogLevel::ERROR);
    
    // Reset test statistics
    getTestStats().reset();
    
    // Clean up any existing state
    SessionManagerTestHelper::cleanupAllSessions();
    
    // Run all tests
    RUN_TEST(testSessionCreation);
    RUN_TEST(testSessionRetrieval);
    RUN_TEST(testSessionRemoval);
    RUN_TEST(testOnlineUserTracking);
    RUN_TEST(testSessionOnlineUserCleanup);
    RUN_TEST(testPingTimeUpdates);
    RUN_TEST(testGetAllClientFds);
    RUN_TEST(testSessionDataModification);
    RUN_TEST(testConcurrentSessionOperations);
    RUN_TEST(testSessionTimestamps);
    RUN_TEST(testMultipleUsersSameIP);
    RUN_TEST(testLifelineTracking);
    
    // Final cleanup
    SessionManagerTestHelper::cleanupAllSessions();
    
    // Print summary
    getTestStats().printSummary();
    
    return getTestStats().exitCode();
}
