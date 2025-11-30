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
    
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-c" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (string(argv[i]) == "-p" && i + 1 < argc) {
            config.port = stoi(argv[++i]);
        } else if (string(argv[i]) == "-l" && i + 1 < argc) {
            config.log_file = argv[++i];
        } else if (string(argv[i]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (config.port == 0) {
        if (!ConfigLoader::loadFromFile(config_file, config)) {
            cerr << "Warning: Failed to load config file: " << config_file << endl;
            cerr << "Using default configuration" << endl;
        }
    }

    ServerCore server(config);

    if (!server.start()) {
        cerr << "Failed to start server" << endl;
        return 1;
    }

    server.run();

    return 0;
}

