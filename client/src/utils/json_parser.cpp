#include "json_parser.h"
#include <cctype>
#include <sstream>

using namespace std;

namespace MillionaireGame {

namespace JsonParser {

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
    
    size_t end = pos;
    while (end < json.length() && json[end] != '"' && json[end] != '\\') {
        end++;
    }
    
    // Handle escaped characters
    while (end < json.length() && json[end] == '\\') {
        end += 2; // Skip escape sequence
        while (end < json.length() && json[end] != '"' && json[end] != '\\') {
            end++;
        }
    }
    
    if (end >= json.length() || json[end] != '"') return "";
    
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
    while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ' && json[end] != '\n' && json[end] != '\r') {
        end++;
    }
    
    if (end == pos) return default_value;
    
    try {
        return stoi(json.substr(pos, end - pos));
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
    
    if (json.substr(pos, 4) == "true") {
        return true;
    } else if (json.substr(pos, 5) == "false") {
        return false;
    }
    
    return default_value;
}

string escapeString(const string& str) {
    stringstream ss;
    for (char c : str) {
        if (c == '"') {
            ss << "\\\"";
        } else if (c == '\\') {
            ss << "\\\\";
        } else if (c == '\n') {
            ss << "\\n";
        } else if (c == '\r') {
            ss << "\\r";
        } else if (c == '\t') {
            ss << "\\t";
        } else {
            ss << c;
        }
    }
    return ss.str();
}

string buildObject(const vector<pair<string, string>>& stringFields,
                   const vector<pair<string, int>>& intFields,
                   const vector<pair<string, bool>>& boolFields) {
    stringstream ss;
    ss << "{";
    
    bool first = true;
    
    for (const auto& field : stringFields) {
        if (!first) ss << ",";
        ss << "\"" << field.first << "\":\"" << escapeString(field.second) << "\"";
        first = false;
    }
    
    for (const auto& field : intFields) {
        if (!first) ss << ",";
        ss << "\"" << field.first << "\":" << field.second;
        first = false;
    }
    
    for (const auto& field : boolFields) {
        if (!first) ss << ",";
        ss << "\"" << field.first << "\":" << (field.second ? "true" : "false");
        first = false;
    }
    
    ss << "}";
    return ss.str();
}

string buildArray(const vector<string>& values) {
    stringstream ss;
    ss << "[";
    
    for (size_t i = 0; i < values.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << escapeString(values[i]) << "\"";
    }
    
    ss << "]";
    return ss.str();
}

} // namespace JsonParser

} // namespace MillionaireGame

