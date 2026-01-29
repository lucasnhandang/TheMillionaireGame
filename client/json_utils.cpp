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

long long extractLongLong(const string& json, const string& key, long long default_value) {
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
        return stoll(value_str);
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

string extractJsonValue(const string& json, const string& key) {
    string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    if (pos >= json.length()) return "";
    
    size_t start = pos;
    
    // Determine the type and extract accordingly
    if (json[pos] == '"') {
        // String value - extract until closing quote
        pos++;
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        return json.substr(start, end - start + 1);
    } else if (json[pos] == '[') {
        // Array value - extract until matching closing bracket
        int bracket_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '[') bracket_count++;
            if (json[end] == ']') {
                bracket_count--;
                if (bracket_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else if (json[pos] == '{') {
        // Object value - extract until matching closing brace
        int brace_count = 0;
        size_t end = pos;
        while (end < json.length()) {
            if (json[end] == '{') brace_count++;
            if (json[end] == '}') {
                brace_count--;
                if (brace_count == 0) {
                    return json.substr(start, end - start + 1);
                }
            }
            end++;
        }
        return "";
    } else {
        // Number or boolean - extract until comma, }, or ]
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
            end++;
        }
        return json.substr(start, end - start);
    }
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

