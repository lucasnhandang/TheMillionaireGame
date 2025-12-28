#include "server_core.h"
#include "config.h"
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace MillionaireGame;

void printUsage(const char* program_name) {
    cout << "Usage: " << program_name << " [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -c <config_file>  Configuration file path (default: config.json)" << endl;
    cout << "  -p <port>         Server port (overrides config file)" << endl;
    cout << "  -l <log_file>     Log file path (overrides config file)" << endl;
    cout << "  -h                Show this help message" << endl;
}

int main(int argc, char* argv[]) {
    string config_file = "config.json";
    ServerConfig config;
    int port_override = 0;
    string log_file_override;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-c" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (string(argv[i]) == "-p" && i + 1 < argc) {
            port_override = stoi(argv[++i]);
        } else if (string(argv[i]) == "-l" && i + 1 < argc) {
            log_file_override = argv[++i];
        } else if (string(argv[i]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    // Load config file
    if (!ConfigLoader::loadFromFile(config_file, config)) {
        cerr << "Warning: Failed to load config file: " << config_file << endl;
        cerr << "Using default configuration" << endl;
    }
    
    // Apply command line overrides (they take precedence)
    if (port_override > 0) {
        config.port = port_override;
    }
    if (!log_file_override.empty()) {
        config.log_file = log_file_override;
    }

    ServerCore server(config);

    if (!server.start()) {
        cerr << "Failed to start server" << endl;
        return 1;
    }

    server.run();

    return 0;
}

