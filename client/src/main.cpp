#include "client_core.h"
#include "protocol_handler.h"
#include "gui/login_window.h"
#include "gui/main_window.h"
#include <iostream>
#include <string>

using namespace MillionaireGame;
using namespace std;

int main(int argc, char* argv[]) {
    // Default server configuration
    string host = "localhost";
    int port = 8080;
    
    // Parse command line arguments
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = stoi(argv[2]);
    }
    
    cout << "Dang ket noi den server " << host << ":" << port << "..." << endl;
    
    // Create client core and connect
    ClientCore client;
    if (!client.connect(host, port)) {
        cerr << "Khong the ket noi den server!" << endl;
        cerr << "Kiem tra xem server da chay chua." << endl;
        return 1;
    }
    
    cout << "Ket noi thanh cong!" << endl;
    
    // Create protocol handler
    ProtocolHandler protocol(&client);
    
    // Wait for CONNECTION notification from server
    cout << "Dang cho thong bao ket noi tu server..." << endl;
    
    // Try multiple times to get CONNECTION notification
    string connectionMsg;
    for (int i = 0; i < 5; i++) {
        connectionMsg = client.receiveMessage(2);
        if (!connectionMsg.empty()) {
            cerr << "[DEBUG] Received initial message (attempt " << (i+1) << "): " << connectionMsg.substr(0, 100) << "..." << endl;
            if (protocol.isNotification(connectionMsg)) {
                string type = protocol.getNotificationType(connectionMsg);
                cerr << "[DEBUG] Notification type: " << type << endl;
                if (type == "CONNECTION") {
                    cout << "Da ket noi voi server thanh cong!" << endl;
                    break;
                }
            } else {
                cerr << "[DEBUG] Initial message is not a notification, waiting for CONNECTION..." << endl;
                // Not CONNECTION notification, continue waiting
                continue;
            }
        } else {
            cerr << "[DEBUG] No message received (attempt " << (i+1) << "), retrying..." << endl;
        }
    }
    
    if (connectionMsg.empty() || !protocol.isNotification(connectionMsg) || 
        protocol.getNotificationType(connectionMsg) != "CONNECTION") {
        cerr << "[DEBUG] Warning: Did not receive CONNECTION notification, but continuing anyway..." << endl;
        cout << "Khong nhan duoc thong bao ket noi, nhung van tiep tuc..." << endl;
    }
    
    // Start notification listener for future notifications
    client.setNotificationCallback([&protocol](const string& message) {
        // Handle notifications (can be extended)
        if (protocol.isNotification(message)) {
            string type = protocol.getNotificationType(message);
            // Process notification based on type
        }
    });
    client.startListening();
    
    // Main application loop
    while (true) {
        // Show login window
        LoginWindow loginWindow(&protocol);
        if (!loginWindow.show()) {
            // User chose to exit
            break;
        }
        
        // User logged in successfully
        string authToken = loginWindow.getAuthToken();
        string username = loginWindow.getUsername();
        string role = loginWindow.getRole();
        
        // Show main window
        MainWindow mainWindow(&protocol, authToken, username, role);
        if (!mainWindow.show()) {
            // User logged out, return to login
            protocol.logout(authToken);
            continue;
        }
    }
    
    // Disconnect
    client.disconnect();
    cout << "Cam on ban da choi!" << endl;
    
    return 0;
}

