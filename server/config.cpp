#include "config.h"
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

namespace MillionaireGame {

string ConfigLoader::trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

string ConfigLoader::extractValue(const string& json, const string& key) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        size_t end = pos;
        while (end < json.length() && 
               json[end] != ',' && 
               json[end] != '}' && 
               json[end] != ' ' && 
               json[end] != '\n' &&
               json[end] != '\t') {
            end++;
        }
        return json.substr(pos, end - pos);
    }
}

int ConfigLoader::extractIntValue(const string& json, const string& key, int default_value) {
    string value_str = extractValue(json, key);
    if (value_str.empty()) return default_value;
    
    try {
        return stoi(value_str);
    } catch (...) {
        return default_value;
    }
}

string ConfigLoader::extractStringValue(const string& json, const string& key, const string& default_value) {
    string value = extractValue(json, key);
    if (value.empty()) return default_value;
    return value;
}

bool ConfigLoader::loadFromFile(const string& config_path, ServerConfig& config) {
    ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return loadFromString(buffer.str(), config);
}

bool ConfigLoader::loadFromString(const string& json_content, ServerConfig& config) {
    string json = json_content;
    
    config.port = extractIntValue(json, "port", 8080);
    config.log_file = extractStringValue(json, "log_file", "");
    config.log_level = extractStringValue(json, "log_level", "INFO");
    config.max_clients = extractIntValue(json, "max_clients", 100);
    config.ping_timeout_seconds = extractIntValue(json, "ping_timeout_seconds", 60);
    config.connection_timeout_seconds = extractIntValue(json, "connection_timeout_seconds", 300);
    
    // Database configuration
    config.db_host = extractStringValue(json, "db_host", "localhost");
    config.db_port = extractIntValue(json, "db_port", 5432);
    config.db_name = extractStringValue(json, "db_name", "millionaire_game");
    config.db_user = extractStringValue(json, "db_user", "postgres");
    config.db_password = extractStringValue(json, "db_password", "");
    
    return true;
}

} // namespace MillionaireGame

