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
    
    // Start notification listener
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

