#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>

namespace MillionaireGame {

struct ServerConfig {
    int port;
    std::string log_file;
    std::string log_level;
    int max_clients;
    int ping_timeout_seconds;
    int connection_timeout_seconds;
    
    // Database configuration
    std::string db_host;
    int db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
    
    ServerConfig() 
        : port(8080), log_file(""), log_level("INFO"), 
          max_clients(100), ping_timeout_seconds(60), 
          connection_timeout_seconds(300),
          db_host("localhost"), db_port(5432), db_name("millionaire_game"),
          db_user("postgres"), db_password("") {}
};

class ConfigLoader {
public:
    static bool loadFromFile(const std::string& config_path, ServerConfig& config);
    static bool loadFromString(const std::string& json_content, ServerConfig& config);
    
private:
    static std::string trim(const std::string& str);
    static std::string extractValue(const std::string& json, const std::string& key);
    static int extractIntValue(const std::string& json, const std::string& key, int default_value);
    static std::string extractStringValue(const std::string& json, const std::string& key, const std::string& default_value);
};

} // namespace MillionaireGame

#endif // CONFIG_H

