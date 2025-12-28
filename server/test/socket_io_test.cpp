/**
 * Unit tests for Socket I/O Operations
 * 
 * Tests low-level socket operations including:
 * - Socket creation and configuration
 * - Non-blocking I/O
 * - Read/Write operations
 * - Error handling
 * - Edge cases and boundary conditions
 * - Select/Poll operations
 * 
 * Compile: make socket_io_test
 * Run: ./bin/socket_io_test
 */

#include "test_utils.h"
#include "../stream_handler.h"
#include "../logger.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <chrono>
#include <cstring>

using namespace std;
using namespace MillionaireGame;
using namespace MillionaireGame::Test;

// ============================================================================
// Test 1: Socket Pair Creation
// ============================================================================

void testSocketPairCreation() {
    TEST_SECTION("Socket Pair Creation");
    
    // Test successful creation
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "First socket FD is valid");
    TEST_ASSERT(sockets.second >= 0, "Second socket FD is valid");
    TEST_ASSERT_NEQ(sockets.first, sockets.second, "Socket FDs are different");
    
    closeSocketPair(sockets);
    
    // Test multiple pairs
    vector<pair<int, int>> pairs;
    for (int i = 0; i < 5; i++) {
        auto p = createSocketPair();
        TEST_ASSERT(p.first >= 0, "Multiple pair " + to_string(i) + " created");
        pairs.push_back(p);
    }
    
    // Clean up
    for (auto& p : pairs) {
        closeSocketPair(p);
    }
}

// ============================================================================
// Test 2: Raw Socket Read/Write
// ============================================================================

void testRawSocketReadWrite() {
    TEST_SECTION("Raw Socket Read/Write");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Test simple write and read
    const char* test_data = "Hello, Socket!";
    ssize_t bytes_written = send(sockets.first, test_data, strlen(test_data), 0);
    TEST_ASSERT_EQ((int)bytes_written, (int)strlen(test_data), "All bytes written");
    
    char buffer[256];
    ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, (int)strlen(test_data), "All bytes read");
    
    buffer[bytes_read] = '\0';
    TEST_ASSERT_EQ(string(buffer), string(test_data), "Data matches");
    
    // Test utility functions
    string response = "Response data";
    sendRawData(sockets.second, response);
    string received = receiveRawData(sockets.first);
    TEST_ASSERT_EQ(received, response, "Utility functions work correctly");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 3: Non-Blocking Socket Operations
// ============================================================================

void testNonBlockingSocket() {
    TEST_SECTION("Non-Blocking Socket Operations");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Set socket to non-blocking
    int flags = fcntl(sockets.second, F_GETFL, 0);
    TEST_ASSERT(flags >= 0, "Get socket flags");
    
    int result = fcntl(sockets.second, F_SETFL, flags | O_NONBLOCK);
    TEST_ASSERT_EQ(result, 0, "Set non-blocking mode");
    
    // Read from empty socket (should return EAGAIN/EWOULDBLOCK)
    char buffer[256];
    ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, -1, "Non-blocking read on empty socket returns -1");
    TEST_ASSERT(errno == EAGAIN || errno == EWOULDBLOCK, "Error is EAGAIN or EWOULDBLOCK");
    
    // Write data
    send(sockets.first, "test", 4, 0);
    this_thread::sleep_for(chrono::milliseconds(10));
    
    // Now read should succeed
    bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, 4, "Non-blocking read with data succeeds");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 4: Select/Poll for Data Availability
// ============================================================================

void testSelectOperation() {
    TEST_SECTION("Select Operation");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Test select with no data (should timeout)
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockets.second, &read_fds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;  // 50ms
    
    int result = select(sockets.second + 1, &read_fds, nullptr, nullptr, &timeout);
    TEST_ASSERT_EQ(result, 0, "Select times out with no data");
    
    // Send data
    send(sockets.first, "data", 4, 0);
    this_thread::sleep_for(chrono::milliseconds(10));
    
    // Test select with data available
    FD_ZERO(&read_fds);
    FD_SET(sockets.second, &read_fds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    result = select(sockets.second + 1, &read_fds, nullptr, nullptr, &timeout);
    TEST_ASSERT(result > 0, "Select returns positive with data available");
    TEST_ASSERT(FD_ISSET(sockets.second, &read_fds), "Socket is in ready set");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 5: Multiple Socket Select
// ============================================================================

void testMultipleSocketSelect() {
    TEST_SECTION("Multiple Socket Select");
    
    vector<pair<int, int>> socket_pairs;
    for (int i = 0; i < 3; i++) {
        auto p = createSocketPair();
        TEST_ASSERT(p.first >= 0, "Socket pair " + to_string(i) + " created");
        socket_pairs.push_back(p);
    }
    
    // Set up select for all reading sockets
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = 0;
    
    for (const auto& p : socket_pairs) {
        FD_SET(p.second, &read_fds);
        max_fd = max(max_fd, p.second);
    }
    
    // Send data to only one socket
    send(socket_pairs[1].first, "test", 4, 0);
    this_thread::sleep_for(chrono::milliseconds(10));
    
    struct timeval timeout = {0, 100000};  // 100ms
    int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    
    TEST_ASSERT_EQ(result, 1, "Select returns 1 when 1 socket has data");
    TEST_ASSERT_FALSE(FD_ISSET(socket_pairs[0].second, &read_fds), "Socket 0 not ready");
    TEST_ASSERT(FD_ISSET(socket_pairs[1].second, &read_fds), "Socket 1 is ready");
    TEST_ASSERT_FALSE(FD_ISSET(socket_pairs[2].second, &read_fds), "Socket 2 not ready");
    
    // Clean up
    for (auto& p : socket_pairs) {
        closeSocketPair(p);
    }
}

// ============================================================================
// Test 6: Socket Buffer Limits
// ============================================================================

void testSocketBufferLimits() {
    TEST_SECTION("Socket Buffer Limits");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Get socket buffer size
    int send_buf_size;
    socklen_t opt_len = sizeof(send_buf_size);
    int result = getsockopt(sockets.first, SOL_SOCKET, SO_SNDBUF, &send_buf_size, &opt_len);
    TEST_ASSERT_EQ(result, 0, "Get send buffer size");
    TEST_ASSERT(send_buf_size > 0, "Send buffer size is positive: " + to_string(send_buf_size));
    
    int recv_buf_size;
    result = getsockopt(sockets.second, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, &opt_len);
    TEST_ASSERT_EQ(result, 0, "Get receive buffer size");
    TEST_ASSERT(recv_buf_size > 0, "Receive buffer size is positive: " + to_string(recv_buf_size));
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 7: Zero-Length Read/Write
// ============================================================================

void testZeroLengthOperations() {
    TEST_SECTION("Zero-Length Operations");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Zero-length write
    ssize_t bytes_written = send(sockets.first, "", 0, 0);
    TEST_ASSERT(bytes_written == 0, "Zero-length write succeeds");
    
    // Write some data first
    send(sockets.first, "test", 4, 0);
    this_thread::sleep_for(chrono::milliseconds(10));
    
    // Zero-length buffer read (edge case)
    char empty_buffer[1];
    ssize_t bytes_read = recv(sockets.second, empty_buffer, 0, 0);
    TEST_ASSERT(bytes_read == 0, "Zero-length read returns 0");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 8: Socket Close Detection
// ============================================================================

void testSocketCloseDetection() {
    TEST_SECTION("Socket Close Detection");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Close one end
    close(sockets.first);
    sockets.first = -1;
    this_thread::sleep_for(chrono::milliseconds(50));
    
    // Read from other end should return 0 (EOF)
    char buffer[256];
    ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, 0, "Read returns 0 on closed connection (EOF)");
    
    close(sockets.second);
}

// ============================================================================
// Test 9: Partial Write Handling
// ============================================================================

void testPartialWriteHandling() {
    TEST_SECTION("Partial Write Handling");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Create smaller data to ensure test completes quickly
    const size_t data_size = 16000;  // 16KB - can be read in ~2 iterations
    string large_data(data_size, 'X');
    
    size_t total_sent = 0;
    const char* data_ptr = large_data.c_str();
    
    // Send in a separate thread to avoid blocking
    thread sender([&]() {
        auto start_time = chrono::steady_clock::now();
        const int max_attempts = 20;
        int attempts = 0;
        
        while (total_sent < data_size && attempts < max_attempts) {
            // Check timeout (2 seconds)
            auto elapsed = chrono::duration_cast<chrono::seconds>(
                chrono::steady_clock::now() - start_time).count();
            if (elapsed > 2) {
                break;
            }
            
            ssize_t sent = send(sockets.first, data_ptr + total_sent, 
                              data_size - total_sent, MSG_DONTWAIT);
            if (sent > 0) {
                total_sent += sent;
            } else if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // Would block, need to wait for space
                this_thread::sleep_for(chrono::milliseconds(5));
                attempts++;
            } else {
                break;
            }
        }
    });
    
    // Read all data with timeout
    size_t total_read = 0;
    char buffer[8192];
    auto start_time = chrono::steady_clock::now();
    int read_iterations = 0;
    const int max_read_iterations = 20;
    
    while (total_read < data_size && read_iterations < max_read_iterations) {
        // Check timeout (2 seconds)
        auto elapsed = chrono::duration_cast<chrono::seconds>(
            chrono::steady_clock::now() - start_time).count();
        if (elapsed > 2) {
            break;
        }
        read_iterations++;
        
        // Use select to check if data is available
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockets.second, &read_fds);
        struct timeval timeout = {0, 50000};  // 50ms
        
        int select_result = select(sockets.second + 1, &read_fds, nullptr, nullptr, &timeout);
        if (select_result > 0 && FD_ISSET(sockets.second, &read_fds)) {
            ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
            if (bytes_read > 0) {
                total_read += bytes_read;
            } else if (bytes_read == 0) {
                break;  // Connection closed
            }
        }
    }
    
    sender.join();
    
    TEST_ASSERT_EQ(total_sent, data_size, "All data eventually sent");
    TEST_ASSERT_EQ(total_read, data_size, "All data received");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 10: Socket Options
// ============================================================================

void testSocketOptions() {
    TEST_SECTION("Socket Options");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Test SO_KEEPALIVE
    int keepalive = 1;
    int result = setsockopt(sockets.first, SOL_SOCKET, SO_KEEPALIVE, 
                           &keepalive, sizeof(keepalive));
    // Note: May fail on socket pairs, just test it doesn't crash
    
    // Test SO_RCVTIMEO
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    result = setsockopt(sockets.first, SOL_SOCKET, SO_RCVTIMEO, 
                       &timeout, sizeof(timeout));
    TEST_ASSERT_EQ(result, 0, "Set receive timeout");
    
    // Verify it was set
    struct timeval get_timeout;
    socklen_t len = sizeof(get_timeout);
    result = getsockopt(sockets.first, SOL_SOCKET, SO_RCVTIMEO, 
                       &get_timeout, &len);
    TEST_ASSERT_EQ(result, 0, "Get receive timeout");
    TEST_ASSERT(get_timeout.tv_sec >= 0, "Timeout value valid");
    
    // Test SO_SNDTIMEO
    result = setsockopt(sockets.first, SOL_SOCKET, SO_SNDTIMEO, 
                       &timeout, sizeof(timeout));
    TEST_ASSERT_EQ(result, 0, "Set send timeout");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 11: Concurrent Read/Write
// ============================================================================

void testConcurrentReadWrite() {
    TEST_SECTION("Concurrent Read/Write");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    const int message_count = 20;  // Reduced to 20 for faster testing
    atomic<int> received_count(0);
    atomic<bool> writer_done(false);
    
    // Writer thread
    thread writer([&]() {
        for (int i = 0; i < message_count; i++) {
            string msg = "Message " + to_string(i) + "\n";
            send(sockets.first, msg.c_str(), msg.length(), 0);
            this_thread::sleep_for(chrono::milliseconds(2));
        }
        writer_done = true;
    });
    
    // Reader thread with better timeout logic
    thread reader([&]() {
        char buffer[256];
        auto start_time = chrono::steady_clock::now();
        
        while (received_count < message_count) {
            // Check timeout (3 seconds total)
            auto elapsed = chrono::duration_cast<chrono::seconds>(
                chrono::steady_clock::now() - start_time).count();
            if (elapsed > 3) {
                break;
            }
            
            // Also break if writer is done and we haven't received anything for a while
            if (writer_done) {
                auto wait_time = chrono::duration_cast<chrono::milliseconds>(
                    chrono::steady_clock::now() - start_time).count();
                // If writer done and no new messages in 200ms, break
                if (wait_time > 200 && received_count >= message_count) {
                    break;
                }
            }
            
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sockets.second, &read_fds);
            
            struct timeval timeout = {0, 50000};  // 50ms (reduced from 100ms)
            int result = select(sockets.second + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (result > 0 && FD_ISSET(sockets.second, &read_fds)) {
                ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
                if (bytes_read > 0) {
                    // Count newlines as messages
                    for (ssize_t i = 0; i < bytes_read; i++) {
                        if (buffer[i] == '\n') received_count++;
                    }
                } else if (bytes_read == 0) {
                    break;  // Connection closed
                }
            }
        }
    });
    
    writer.join();
    reader.join();
    
    TEST_ASSERT_EQ(received_count.load(), message_count, "All concurrent messages received");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 12: MSG_PEEK Flag
// ============================================================================

void testMsgPeekFlag() {
    TEST_SECTION("MSG_PEEK Flag");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Send data
    const char* test_data = "Peek Test Data";
    send(sockets.first, test_data, strlen(test_data), 0);
    this_thread::sleep_for(chrono::milliseconds(10));
    
    char buffer1[256], buffer2[256];
    
    // Peek at data (should not remove from buffer)
    ssize_t peek_bytes = recv(sockets.second, buffer1, sizeof(buffer1), MSG_PEEK);
    buffer1[peek_bytes] = '\0';
    TEST_ASSERT(peek_bytes > 0, "Peek received data");
    TEST_ASSERT_EQ(string(buffer1), string(test_data), "Peeked data matches");
    
    // Regular read (should get same data)
    ssize_t read_bytes = recv(sockets.second, buffer2, sizeof(buffer2), 0);
    buffer2[read_bytes] = '\0';
    TEST_ASSERT_EQ(string(buffer2), string(test_data), "Regular read gets same data");
    
    // Another peek should find nothing (data was consumed)
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockets.second, &read_fds);
    struct timeval timeout = {0, 10000};  // 10ms
    int result = select(sockets.second + 1, &read_fds, nullptr, nullptr, &timeout);
    TEST_ASSERT_EQ(result, 0, "No more data after regular read");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 13: Socket Shutdown
// ============================================================================

void testSocketShutdown() {
    TEST_SECTION("Socket Shutdown");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    // Send some data before shutdown
    send(sockets.first, "data", 4, 0);
    
    // Shutdown write side
    int result = shutdown(sockets.first, SHUT_WR);
    TEST_ASSERT_EQ(result, 0, "Shutdown write side");
    
    // Other end should still be able to read existing data
    char buffer[256];
    ssize_t bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, 4, "Can still read after shutdown");
    
    // Next read should return 0 (EOF)
    bytes_read = recv(sockets.second, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)bytes_read, 0, "EOF after shutdown");
    
    closeSocketPair(sockets);
}

// ============================================================================
// Test 14: Error Handling
// ============================================================================

void testErrorHandling() {
    TEST_SECTION("Error Handling");
    
    // Test operations on invalid socket
    char buffer[256];
    ssize_t result = recv(-1, buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQ((int)result, -1, "Read from invalid socket returns -1");
    TEST_ASSERT_EQ(errno, EBADF, "Error is EBADF (bad file descriptor)");
    
    result = send(-1, "test", 4, 0);
    TEST_ASSERT_EQ((int)result, -1, "Write to invalid socket returns -1");
    
    // Test operations on closed socket
    auto sockets = createSocketPair();
    close(sockets.first);
    
    result = send(sockets.first, "test", 4, 0);
    TEST_ASSERT_EQ((int)result, -1, "Write to closed socket returns -1");
    
    close(sockets.second);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║            Socket I/O Unit Tests                           ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    // Initialize logger (suppress output during tests)
    Logger::getInstance().initialize("", LogLevel::ERROR);
    
    // Reset test statistics
    getTestStats().reset();
    
    // Run all tests
    RUN_TEST(testSocketPairCreation);
    RUN_TEST(testRawSocketReadWrite);
    RUN_TEST(testNonBlockingSocket);
    RUN_TEST(testSelectOperation);
    RUN_TEST(testMultipleSocketSelect);
    RUN_TEST(testSocketBufferLimits);
    RUN_TEST(testZeroLengthOperations);
    RUN_TEST(testSocketCloseDetection);
    RUN_TEST(testPartialWriteHandling);
    RUN_TEST(testSocketOptions);
    RUN_TEST(testConcurrentReadWrite);
    RUN_TEST(testMsgPeekFlag);
    RUN_TEST(testSocketShutdown);
    RUN_TEST(testErrorHandling);
    
    // Print summary
    getTestStats().printSummary();
    
    return getTestStats().exitCode();
}
