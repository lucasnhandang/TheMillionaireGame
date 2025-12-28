/**
 * Unit tests for StreamHandler
 * 
 * Tests TCP stream handling including:
 * - Basic read/write operations
 * - Multiple message handling
 * - Partial message reassembly
 * - Timeout handling
 * - Connection status detection
 * - JSON utility functions
 * - Large message handling
 * - Buffer management
 * - Error conditions
 * 
 * Compile: make stream_handler_test
 * Run: ./bin/stream_handler_test
 */

#include "test_utils.h"
#include "../stream_handler.h"
#include "../logger.h"
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace std;
using namespace MillionaireGame;
using namespace MillionaireGame::Test;

// ============================================================================
// Test 1: Basic Read/Write Operations
// ============================================================================

void testBasicReadWrite() {
    TEST_SECTION("Basic Read/Write");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Test simple message
    string test_message = "{\"requestType\":\"TEST\",\"data\":{}}";
    bool write_result = handler1.writeMessage(test_message);
    TEST_ASSERT(write_result, "Write message succeeds");
    
    string received = handler2.readMessage(1);
    TEST_ASSERT_EQ(received, test_message, "Read message matches written");
    
    // Test empty data object
    string empty_data = "{\"requestType\":\"EMPTY\",\"data\":{}}";
    handler1.writeMessage(empty_data);
    received = handler2.readMessage(1);
    TEST_ASSERT_EQ(received, empty_data, "Empty data message handled");
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 2: Multiple Messages in Sequence
// ============================================================================

void testMultipleMessages() {
    TEST_SECTION("Multiple Messages");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Send multiple messages
    vector<string> messages = {
        "{\"requestType\":\"MSG1\",\"id\":1}",
        "{\"requestType\":\"MSG2\",\"id\":2}",
        "{\"requestType\":\"MSG3\",\"id\":3}",
        "{\"requestType\":\"MSG4\",\"id\":4}",
        "{\"requestType\":\"MSG5\",\"id\":5}"
    };
    
    // Send all messages
    for (const auto& msg : messages) {
        bool sent = handler1.writeMessage(msg);
        TEST_ASSERT(sent, "Message sent: " + msg.substr(0, 30));
    }
    
    // Allow data to be transmitted
    this_thread::sleep_for(chrono::milliseconds(50));
    
    // Read and verify all messages in order
    for (size_t i = 0; i < messages.size(); i++) {
        string received = handler2.readMessage(1);
        TEST_ASSERT_EQ(received, messages[i], "Message " + to_string(i + 1) + " received correctly");
    }
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 3: Partial Message Handling (Fragmentation)
// ============================================================================

void testPartialMessages() {
    TEST_SECTION("Partial Message Handling");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler2(sockets.second);
    
    // Send message in fragments manually
    string full_message = "{\"requestType\":\"PARTIAL\",\"data\":{\"key\":\"value\",\"number\":12345}}";
    string msg_with_newline = full_message + "\n";
    
    // Fragment 1: First 10 bytes
    send(sockets.first, msg_with_newline.c_str(), 10, 0);
    this_thread::sleep_for(chrono::milliseconds(20));
    
    // Fragment 2: Next 20 bytes
    send(sockets.first, msg_with_newline.c_str() + 10, 20, 0);
    this_thread::sleep_for(chrono::milliseconds(20));
    
    // Fragment 3: Remaining bytes
    send(sockets.first, msg_with_newline.c_str() + 30, msg_with_newline.length() - 30, 0);
    
    // Should receive complete message
    string received = handler2.readMessage(2);
    TEST_ASSERT_EQ(received, full_message, "Fragmented message reassembled correctly");
    
    // Test single-byte fragmentation
    string short_msg = "{\"requestType\":\"TINY\"}";
    string short_with_newline = short_msg + "\n";
    for (size_t i = 0; i < short_with_newline.length(); i++) {
        send(sockets.first, short_with_newline.c_str() + i, 1, 0);
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    
    received = handler2.readMessage(2);
    TEST_ASSERT_EQ(received, short_msg, "Single-byte fragmented message reassembled");
    
    close(sockets.first);
    handler2.close();
}

// ============================================================================
// Test 4: Timeout Handling
// ============================================================================

void testTimeout() {
    TEST_SECTION("Timeout Handling");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler(sockets.second);
    
    // Set short read timeout
    bool timeout_set = handler.setReadTimeout(0, 100000);  // 100ms
    TEST_ASSERT(timeout_set, "Read timeout set successfully");
    
    // Try to read with no data available (should timeout)
    TestTimer timer;
    string received = handler.readMessage(0);
    double elapsed = timer.elapsedMs();
    
    TEST_ASSERT(received.empty(), "Timeout returns empty message");
    TEST_ASSERT(elapsed >= 50, "Timeout occurred after reasonable time");
    
    // Verify handler is still usable after timeout
    TEST_ASSERT(handler.isConnected(), "Handler still connected after timeout");
    
    handler.close();
    close(sockets.first);
}

// ============================================================================
// Test 5: Connection Status Detection
// ============================================================================

void testConnectionStatus() {
    TEST_SECTION("Connection Status");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler(sockets.first);
    
    // Initially connected
    TEST_ASSERT(handler.isConnected(), "Handler reports connected initially");
    TEST_ASSERT(handler.getSocketFd() >= 0, "Socket FD is valid");
    
    // Close the other end
    close(sockets.second);
    this_thread::sleep_for(chrono::milliseconds(50));
    
    // Try to read (should detect disconnection)
    handler.setReadTimeout(0, 100000);
    string received = handler.readMessage(1);
    
    TEST_ASSERT(!handler.isConnected(), "Handler detects disconnection after read attempt");
    
    handler.close();
    
    // After close, socket FD should be invalid
    TEST_ASSERT(!handler.isConnected(), "Handler reports disconnected after close");
}

// ============================================================================
// Test 6: Write After Disconnect
// ============================================================================

void testWriteAfterDisconnect() {
    TEST_SECTION("Write After Disconnect");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler(sockets.first);
    
    // Close the connection
    handler.close();
    
    // Try to write after close
    bool write_result = handler.writeMessage("{\"test\":true}");
    TEST_ASSERT_FALSE(write_result, "Write fails after close");
}

// ============================================================================
// Test 7: JSON Utility Functions
// ============================================================================

void testJsonUtils() {
    TEST_SECTION("JSON Utility Functions");
    
    // Test request type extraction
    string json1 = "{\"requestType\":\"LOGIN\",\"data\":{}}";
    TEST_ASSERT_EQ(StreamUtils::extractRequestType(json1), "LOGIN", "Extract LOGIN request type");
    
    string json2 = "{\"requestType\":\"REGISTER\",\"data\":{\"username\":\"test\"}}";
    TEST_ASSERT_EQ(StreamUtils::extractRequestType(json2), "REGISTER", "Extract REGISTER request type");
    
    // Missing request type
    string json3 = "{\"data\":{}}";
    TEST_ASSERT_EQ(StreamUtils::extractRequestType(json3), "", "Missing request type returns empty");
    
    // Test response code extraction
    string response1 = "{\"responseCode\":200,\"data\":{}}";
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(response1), 200, "Extract 200 response code");
    
    string response2 = "{\"responseCode\":401,\"message\":\"Unauthorized\"}";
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(response2), 401, "Extract 401 response code");
    
    string response3 = "{\"responseCode\":500,\"error\":\"Internal error\"}";
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(response3), 500, "Extract 500 response code");
    
    // Missing response code
    string response4 = "{\"data\":{}}";
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(response4), -1, "Missing response code returns -1");
    
    // Test error response creation
    string error_resp = StreamUtils::createErrorResponse(401, "Login failed");
    TEST_ASSERT(!error_resp.empty(), "Error response created");
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(error_resp), 401, "Error response has correct code");
    
    // Test success response creation
    string success_resp = StreamUtils::createSuccessResponse(200, "{\"userid\":1}");
    TEST_ASSERT(!success_resp.empty(), "Success response created");
    TEST_ASSERT_EQ(StreamUtils::extractResponseCode(success_resp), 200, "Success response has correct code");
    
    // Test request creation
    string request = StreamUtils::createRequest("LOGIN", "{\"username\":\"test\"}");
    TEST_ASSERT(!request.empty(), "Request created");
    TEST_ASSERT_EQ(StreamUtils::extractRequestType(request), "LOGIN", "Created request has correct type");
    
    // Test notification creation
    string notification = StreamUtils::createNotification("GAME_START", "{\"gameId\":123}");
    TEST_ASSERT(!notification.empty(), "Notification created");
    TEST_ASSERT(notification.find("\"type\":\"GAME_START\"") != string::npos, "Notification has type field");
    TEST_ASSERT(notification.find("\"data\":{\"gameId\":123}") != string::npos, "Notification has data field");
    TEST_ASSERT(notification.find("responseCode") == string::npos, "Notification has no responseCode");
}

// ============================================================================
// Test 8: JSON Validation
// ============================================================================

void testJsonValidation() {
    TEST_SECTION("JSON Validation");
    
    // Valid JSON
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"key\":\"value\"}"), "Simple object is valid");
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"a\":1,\"b\":2}"), "Multiple keys is valid");
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"nested\":{\"inner\":true}}"), "Nested object is valid");
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"array\":[1,2,3]}"), "Array in object is valid");
    TEST_ASSERT(StreamUtils::validateJsonFormat("[1,2,3]"), "Simple array is valid");
    TEST_ASSERT(StreamUtils::validateJsonFormat("{\"escaped\":\"quote\\\"here\"}"), "Escaped quotes handled");
    
    // Invalid JSON
    TEST_ASSERT_FALSE(StreamUtils::validateJsonFormat("{\"key\":\"value\""), "Missing closing brace is invalid");
    TEST_ASSERT_FALSE(StreamUtils::validateJsonFormat("{\"key\":\"value\"}}}"), "Extra closing braces is invalid");
    TEST_ASSERT_FALSE(StreamUtils::validateJsonFormat("[1,2,3"), "Missing closing bracket is invalid");
    TEST_ASSERT_FALSE(StreamUtils::validateJsonFormat(""), "Empty string is invalid");
    TEST_ASSERT_FALSE(StreamUtils::validateJsonFormat("{\"unclosed\":\"string}"), "Unclosed string is invalid");
}

// ============================================================================
// Test 9: Large Messages
// ============================================================================

void testLargeMessages() {
    TEST_SECTION("Large Messages");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Create progressively larger messages (reduced max size to avoid slow tests)
    vector<size_t> sizes = {100, 500, 1000, 5000};
    
    for (size_t target_size : sizes) {
        // Create large data content
        string large_data = "";
        while (large_data.length() < target_size) {
            large_data += "ABCDEFGHIJ";
        }
        large_data = large_data.substr(0, target_size);
        
        string large_message = "{\"requestType\":\"LARGE\",\"data\":{\"content\":\"" + large_data + "\"}}";
        
        bool write_result = handler1.writeMessage(large_message);
        TEST_ASSERT(write_result, "Write " + to_string(target_size) + " byte message");
        
        string received = handler2.readMessage(5);
        TEST_ASSERT_EQ(received, large_message, "Receive " + to_string(target_size) + " byte message correctly");
    }
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 10: Buffer Clear
// ============================================================================

void testBufferClear() {
    TEST_SECTION("Buffer Clear");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler2(sockets.second);
    
    // Send partial message (no newline)
    send(sockets.first, "{\"partial\":\"data\"", 17, 0);
    this_thread::sleep_for(chrono::milliseconds(50));
    
    // Clear buffer
    handler2.clearBuffer();
    
    // Send complete new message
    string new_msg = "{\"requestType\":\"NEW\"}";
    send(sockets.first, (new_msg + "\n").c_str(), new_msg.length() + 1, 0);
    
    // Should receive only the new message
    string received = handler2.readMessage(1);
    TEST_ASSERT_EQ(received, new_msg, "Buffer cleared correctly, new message received");
    
    close(sockets.first);
    handler2.close();
}

// ============================================================================
// Test 11: Rapid Message Exchange
// ============================================================================

void testRapidMessageExchange() {
    TEST_SECTION("Rapid Message Exchange");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    const int message_count = 100;
    int received_count = 0;
    
    // Send many messages rapidly
    for (int i = 0; i < message_count; i++) {
        string msg = "{\"requestType\":\"RAPID\",\"seq\":" + to_string(i) + "}";
        handler1.writeMessage(msg);
    }
    
    // Receive all messages
    for (int i = 0; i < message_count; i++) {
        string received = handler2.readMessage(2);
        if (!received.empty()) {
            received_count++;
        }
    }
    
    TEST_ASSERT_EQ(received_count, message_count, "All rapid messages received");
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 12: Bidirectional Communication
// ============================================================================

void testBidirectionalCommunication() {
    TEST_SECTION("Bidirectional Communication");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Simulate request-response pattern
    string request = "{\"requestType\":\"PING\",\"data\":{}}";
    string response = "{\"responseCode\":200,\"data\":{\"message\":\"PONG\"}}";
    
    // Handler1 sends request
    handler1.writeMessage(request);
    
    // Handler2 receives and sends response
    string received_request = handler2.readMessage(1);
    TEST_ASSERT_EQ(received_request, request, "Request received");
    
    handler2.writeMessage(response);
    
    // Handler1 receives response
    string received_response = handler1.readMessage(1);
    TEST_ASSERT_EQ(received_response, response, "Response received");
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 13: Special Characters in JSON
// ============================================================================

void testSpecialCharacters() {
    TEST_SECTION("Special Characters in JSON");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Test various special characters
    vector<string> test_messages = {
        "{\"requestType\":\"TEST\",\"data\":{\"msg\":\"Hello World\"}}",
        "{\"requestType\":\"TEST\",\"data\":{\"unicode\":\"Xin chào\"}}",
        "{\"requestType\":\"TEST\",\"data\":{\"quotes\":\"He said \\\"hello\\\"\"}}",
        "{\"requestType\":\"TEST\",\"data\":{\"backslash\":\"path\\\\to\\\\file\"}}",
        "{\"requestType\":\"TEST\",\"data\":{\"numbers\":12345}}"
    };
    
    for (const auto& msg : test_messages) {
        handler1.writeMessage(msg);
        string received = handler2.readMessage(1);
        TEST_ASSERT_EQ(received, msg, "Special chars message: " + msg.substr(0, 40) + "...");
    }
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 14: Socket Timeout Configuration
// ============================================================================

void testTimeoutConfiguration() {
    TEST_SECTION("Timeout Configuration");
    
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created");
    if (sockets.first < 0) return;
    
    StreamHandler handler(sockets.first);
    
    // Test read timeout setting
    TEST_ASSERT(handler.setReadTimeout(1, 0), "Set 1 second read timeout");
    TEST_ASSERT(handler.setReadTimeout(0, 500000), "Set 500ms read timeout");
    TEST_ASSERT(handler.setReadTimeout(5, 500000), "Set 5.5 second read timeout");
    
    // Test write timeout setting
    TEST_ASSERT(handler.setWriteTimeout(1, 0), "Set 1 second write timeout");
    TEST_ASSERT(handler.setWriteTimeout(0, 500000), "Set 500ms write timeout");
    
    handler.close();
    close(sockets.second);
    
    // Test on closed socket (should fail)
    TEST_ASSERT_FALSE(handler.setReadTimeout(1, 0), "Set timeout on closed socket fails");
}

// ============================================================================
// Test 15: Notification Creation and Sending
// ============================================================================

void testNotificationCreation() {
    TEST_SECTION("Notification Creation and Sending");
    
    // Test various notification types
    string game_start = StreamUtils::createNotification("GAME_START", "{\"gameId\":123,\"timestamp\":1705320000}");
    TEST_ASSERT(!game_start.empty(), "GAME_START notification created");
    TEST_ASSERT(game_start.find("\"type\":\"GAME_START\"") != string::npos, "GAME_START has correct type");
    
    string game_end = StreamUtils::createNotification("GAME_END", "{\"status\":\"won\",\"finalPrize\":1000000}");
    TEST_ASSERT(!game_end.empty(), "GAME_END notification created");
    TEST_ASSERT(game_end.find("\"type\":\"GAME_END\"") != string::npos, "GAME_END has correct type");
    
    string question_info = StreamUtils::createNotification("QUESTION_INFO", 
        "{\"questionNumber\":1,\"question\":\"What is 2+2?\",\"options\":[\"3\",\"4\",\"5\",\"6\"]}");
    TEST_ASSERT(!question_info.empty(), "QUESTION_INFO notification created");
    TEST_ASSERT(question_info.find("\"type\":\"QUESTION_INFO\"") != string::npos, "QUESTION_INFO has correct type");
    
    string connection = StreamUtils::createNotification("CONNECTION", "{\"serverName\":\"Test Server\"}");
    TEST_ASSERT(!connection.empty(), "CONNECTION notification created");
    TEST_ASSERT(connection.find("\"type\":\"CONNECTION\"") != string::npos, "CONNECTION has correct type");
    
    // Verify notification does NOT have responseCode
    TEST_ASSERT(game_start.find("responseCode") == string::npos, "GAME_START has no responseCode");
    TEST_ASSERT(game_end.find("responseCode") == string::npos, "GAME_END has no responseCode");
    TEST_ASSERT(question_info.find("responseCode") == string::npos, "QUESTION_INFO has no responseCode");
    TEST_ASSERT(connection.find("responseCode") == string::npos, "CONNECTION has no responseCode");
    
    // Test notification over socket
    auto sockets = createSocketPair();
    TEST_ASSERT(sockets.first >= 0, "Socket pair created for notification test");
    if (sockets.first < 0) return;
    
    StreamHandler handler1(sockets.first);
    StreamHandler handler2(sockets.second);
    
    // Send notification
    bool sent = handler1.writeMessage(game_start);
    TEST_ASSERT(sent, "Notification sent successfully");
    
    string received = handler2.readMessage(1);
    TEST_ASSERT_EQ(received, game_start, "Notification received correctly");
    
    handler1.close();
    handler2.close();
}

// ============================================================================
// Test 16: Response vs Notification Distinction
// ============================================================================

void testResponseNotificationDistinction() {
    TEST_SECTION("Response vs Notification Distinction");
    
    // Create response
    string response = StreamUtils::createSuccessResponse(200, "{\"userId\":1}");
    
    // Create notification
    string notification = StreamUtils::createNotification("GAME_START", "{\"gameId\":1}");
    
    // Response should have responseCode, no type
    TEST_ASSERT(response.find("responseCode") != string::npos, "Response has responseCode");
    TEST_ASSERT(response.find("\"type\":") == string::npos, "Response has no type field");
    
    // Notification should have type, no responseCode
    TEST_ASSERT(notification.find("\"type\":") != string::npos, "Notification has type");
    TEST_ASSERT(notification.find("responseCode") == string::npos, "Notification has no responseCode");
    
    // Both should be valid JSON
    TEST_ASSERT(StreamUtils::validateJsonFormat(response), "Response is valid JSON");
    TEST_ASSERT(StreamUtils::validateJsonFormat(notification), "Notification is valid JSON");
    
    // Test client-side distinction logic
    bool is_response = response.find("responseCode") != string::npos;
    bool is_notification = notification.find("\"type\":") != string::npos && 
                           notification.find("responseCode") == string::npos;
    
    TEST_ASSERT(is_response, "Response correctly identified");
    TEST_ASSERT(is_notification, "Notification correctly identified");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║          StreamHandler Unit Tests                          ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;
    
    // Initialize logger (suppress output during tests)
    Logger::getInstance().initialize("", LogLevel::ERROR);
    
    // Reset test statistics
    getTestStats().reset();
    
    // Run all tests
    RUN_TEST(testBasicReadWrite);
    RUN_TEST(testMultipleMessages);
    RUN_TEST(testPartialMessages);
    RUN_TEST(testTimeout);
    RUN_TEST(testConnectionStatus);
    RUN_TEST(testWriteAfterDisconnect);
    RUN_TEST(testJsonUtils);
    RUN_TEST(testJsonValidation);
    RUN_TEST(testLargeMessages);
    RUN_TEST(testBufferClear);
    RUN_TEST(testRapidMessageExchange);
    RUN_TEST(testBidirectionalCommunication);
    RUN_TEST(testSpecialCharacters);
    RUN_TEST(testTimeoutConfiguration);
    RUN_TEST(testNotificationCreation);
    RUN_TEST(testResponseNotificationDistinction);
    
    // Print summary
    getTestStats().printSummary();
    
    return getTestStats().exitCode();
}
