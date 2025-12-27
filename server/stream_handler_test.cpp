/**
 * Unit tests for StreamHandler
 * 
 * Compile with: g++ -std=c++11 -pthread stream_handler_test.cpp stream_handler.cpp logger.cpp -o stream_handler_test
 * Run with: ./stream_handler_test
 */

#include "stream_handler.h"
#include "logger.h"
#include <cassert>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace std;
using namespace MillionaireGame;

// Test utilities
int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            cout << "PASS: " << message << endl; \
            tests_passed++; \
        } else { \
            cout << "FAIL: " << message << endl; \
            tests_failed++; \
        } \
    } while(0)

/**
 * Create a pair of connected sockets for testing
 */
pair<int, int> createSocketPair() {
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        return {-1, -1};
    }
    return {sockets[0], sockets[1]};
}

/**
 * Test 1: Basic message reading and writing
 */
void testBasicReadWrite() {
    cout << "\n=== Test 1: Basic Read/Write ===" << endl;
    
    auto sockets = createSocketPair();
    if (sockets.first < 0) {
        cout << "FAIL: Could not create socket pair" << endl;
        tests_failed++;
        return;
    }
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Write a message
    string test_message = "{\"requestType\":\"TEST\",\"data\":{}}";
    bool write_result = handler1.writeMessage(test_message);
    TEST_ASSERT(write_result, "Write message");
    
    // Read the message
    string received = handler2.readMessage(1);
    TEST_ASSERT(received == test_message, "Read message matches");
    
    handler1.close();
    handler2.close();
}

/**
 * Test 2: Multiple messages
 */
void testMultipleMessages() {
    cout << "\n=== Test 2: Multiple Messages ===" << endl;
    
    auto sockets = createSocketPair();
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Send multiple messages
    vector<string> messages = {
        "{\"requestType\":\"MSG1\"}",
        "{\"requestType\":\"MSG2\"}",
        "{\"requestType\":\"MSG3\"}"
    };
    
    for (const auto& msg : messages) {
        handler1.writeMessage(msg);
    }
    
    // Read all messages
    for (size_t i = 0; i < messages.size(); i++) {
        string received = handler2.readMessage(1);
        TEST_ASSERT(received == messages[i], "Message " + to_string(i) + " matches");
    }
    
    handler1.close();
    handler2.close();
}

/**
 * Test 3: Partial message handling
 */
void testPartialMessages() {
    cout << "\n=== Test 3: Partial Messages ===" << endl;
    
    auto sockets = createSocketPair();
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Send message in parts
    string full_message = "{\"requestType\":\"PARTIAL\",\"data\":{\"key\":\"value\"}}";
    
    // Manually send in chunks
    string msg_with_newline = full_message + "\n";
    send(sockets.first, msg_with_newline.c_str(), 10, 0); // Send first 10 bytes
    this_thread::sleep_for(chrono::milliseconds(10));
    send(sockets.first, msg_with_newline.c_str() + 10, msg_with_newline.length() - 10, 0);
    
    // Should still receive complete message
    string received = handler2.readMessage(1);
    TEST_ASSERT(received == full_message, "Partial message reassembled correctly");
    
    handler1.close();
    handler2.close();
}

/**
 * Test 4: Timeout handling
 */
void testTimeout() {
    cout << "\n=== Test 4: Timeout Handling ===" << endl;
    
    auto sockets = createSocketPair();
    StreamHandler handler(sockets.second);
    
    // Try to read with short timeout (should timeout)
    handler.setReadTimeout(0, 100000); // 100ms
    string received = handler.readMessage(0);
    TEST_ASSERT(received.empty(), "Timeout returns empty message");
    
    handler.close();
    close(sockets.first);
}

/**
 * Test 5: Connection status
 */
void testConnectionStatus() {
    cout << "\n=== Test 5: Connection Status ===" << endl;
    
    auto sockets = createSocketPair();
    StreamHandler handler(sockets.first);
    
    TEST_ASSERT(handler.isConnected(), "Handler reports connected");
    
    close(sockets.second);
    this_thread::sleep_for(chrono::milliseconds(100));
    
    // Try to read (should detect disconnection)
    string received = handler.readMessage(1);
    TEST_ASSERT(!handler.isConnected(), "Handler detects disconnection");
    
    handler.close();
}

/**
 * Test 6: JSON utility functions
 */
void testJsonUtils() {
    cout << "\n=== Test 6: JSON Utility Functions ===" << endl;
    
    // Test request type extraction
    string json = "{\"requestType\":\"LOGIN\",\"data\":{}}";
    string request_type = StreamUtils::extractRequestType(json);
    TEST_ASSERT(request_type == "LOGIN", "Extract request type");
    
    // Test response code extraction
    string response = "{\"responseCode\":200,\"data\":{}}";
    int code = StreamUtils::extractResponseCode(response);
    TEST_ASSERT(code == 200, "Extract response code");
    
    // Test error response creation
    string error_resp = StreamUtils::createErrorResponse(401, "Login failed");
    TEST_ASSERT(!error_resp.empty(), "Create error response");
    int error_code = StreamUtils::extractResponseCode(error_resp);
    TEST_ASSERT(error_code == 401, "Error response has correct code");
    
    // Test success response creation
    string success_resp = StreamUtils::createSuccessResponse(200, "{\"userid\":1}");
    TEST_ASSERT(!success_resp.empty(), "Create success response");
    int success_code = StreamUtils::extractResponseCode(success_resp);
    TEST_ASSERT(success_code == 200, "Success response has correct code");
    
    // Test request creation
    string request = StreamUtils::createRequest("LOGIN", "{\"username\":\"test\"}");
    TEST_ASSERT(!request.empty(), "Create request");
    string req_type = StreamUtils::extractRequestType(request);
    TEST_ASSERT(req_type == "LOGIN", "Created request has correct type");
    
    // Test JSON validation
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"key\":\"value\"}"), "Valid JSON");
    TEST_ASSERT(!StreamUtils::validateJsonFormat("{\"key\":\"value\""), "Invalid JSON (missing brace)");
}

/**
 * Test 7: Large messages
 */
void testLargeMessages() {
    cout << "\n=== Test 7: Large Messages ===" << endl;
    
    auto sockets = createSocketPair();
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Create a large JSON message
    string large_data = "\"";
    for (int i = 0; i < 1000; i++) {
        large_data += "This is a test string to make the message larger. ";
    }
    large_data += "\"";
    
    string large_message = "{\"requestType\":\"LARGE\",\"data\":{\"content\":" + large_data + "}}";
    
    bool write_result = handler1.writeMessage(large_message);
    TEST_ASSERT(write_result, "Write large message");
    
    string received = handler2.readMessage(1);
    TEST_ASSERT(received == large_message, "Large message received correctly");
    
    handler1.close();
    handler2.close();
}

int main() {
    cout << "Starting StreamHandler Unit Tests\n" << endl;
    
    // Initialize logger (optional for tests)
    Logger::getInstance().initialize("", LogLevel::ERROR);
    
    // Run tests
    testBasicReadWrite();
    testMultipleMessages();
    testPartialMessages();
    testTimeout();
    testConnectionStatus();
    testJsonUtils();
    testLargeMessages();
    
    // Print summary
    cout << "\n=== Test Summary ===" << endl;
    cout << "Passed: " << tests_passed << endl;
    cout << "Failed: " << tests_failed << endl;
    cout << "Total:  " << (tests_passed + tests_failed) << endl;
    
    if (tests_failed == 0) {
        cout << "\nAll tests passed!" << endl;
        return 0;
    } else {
        cout << "\nSome tests failed!" << endl;
        return 1;
    }
}

