#include "json_utils.h"
#include <cctype>
#include <sstream>
#include <map>

using namespace std;

namespace MillionaireGame {

namespace JsonUtils {

string extractString(const string& json, const string& key) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length() || json[pos] != '"') return "";
    pos++;
    
    size_t end = json.find('"', pos);
    if (end == string::npos) return "";
    
    return json.substr(pos, end - pos);
}

int extractInt(const string& json, const string& key, int default_value) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return default_value;
    
    pos = json.find(':', pos);
    if (pos == string::npos) return default_value;
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) return default_value;
    
    size_t end = pos;
    while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
        end++;
    }
    
    string value_str = json.substr(pos, end - pos);
    try {
        return stoi(value_str);
    } catch (...) {
        return default_value;
    }
}

bool extractBool(const string& json, const string& key, bool default_value) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return default_value;
    
    pos = json.find(':', pos);
    if (pos == string::npos) return default_value;
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) return default_value;
    
    if (json.substr(pos, 4) == "true") return true;
    if (json.substr(pos, 5) == "false") return false;
    
    return default_value;
}

string buildJson(const map<string, string>& stringFields,
                const map<string, int>& intFields,
                const map<string, bool>& boolFields) {
    ostringstream json;
    json << "{";
    
    bool first = true;
    
    // Add string fields
    for (const auto& pair : stringFields) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\":\"" << pair.second << "\"";
        first = false;
    }
    
    // Add int fields
    for (const auto& pair : intFields) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\":" << pair.second;
        first = false;
    }
    
    // Add bool fields
    for (const auto& pair : boolFields) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\":" << (pair.second ? "true" : "false");
        first = false;
    }
    
    json << "}";
    return json.str();
}

} // namespace JsonUtils

} // namespace MillionaireGame

