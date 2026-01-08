#include <QApplication>
#include "mainwindow.h"
#include "socket_client.h"
#include "protocol_handler.h"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    
    // Parse arguments
    std::string host = "localhost";
    int port = 8080;
    bool demoMode = false;
    
    if (argc > 1) {
        if (std::string(argv[1]) == "--demo") {
            demoMode = true;
            std::cout << "Running in DEMO MODE (no server connection)" << std::endl;
        } else {
            host = argv[1];
        }
    }
    if (argc > 2 && !demoMode) {
        port = std::stoi(argv[2]);
    }
    
    // Create main window
    MainWindow window;
    window.setDemoMode(demoMode);
    
    // Connect to server (skip in demo mode)
    SocketClient* client = nullptr;
    ProtocolHandler* protocol = nullptr;
    
    if (!demoMode) {
        client = new SocketClient(host, port);
        if (!client->connect()) {
            std::cerr << "Failed to connect to server at " << host << ":" << port << std::endl;
            std::cerr << "Tip: Make sure server is running, or use --demo flag for demo mode" << std::endl;
            return 1;
        }
        protocol = new ProtocolHandler(client);
        window.setProtocolHandler(protocol);
    }
    
    window.show();
    
    int result = app.exec();
    
    // Cleanup
    if (!demoMode) {
        if (protocol) {
            protocol->logout();
            delete protocol;
        }
        if (client) {
            client->disconnect();
            delete client;
        }
    }
    
    return result;
}
