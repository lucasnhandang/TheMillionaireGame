#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>

namespace MillionaireGame {

/**
 * Simple JSON parser for client-side JSON handling
 * Extracts values from JSON strings
 */
namespace JsonParser {
    /**
     * Extract string value from JSON
     * Looks for "key":"value" pattern
     */
    std::string extractString(const std::string& json, const std::string& key);
    
    /**
     * Extract integer value from JSON
     * Looks for "key":value pattern
     */
    int extractInt(const std::string& json, const std::string& key, int default_value = 0);
    
    /**
     * Extract boolean value from JSON
     * Looks for "key":true/false pattern
     */
    bool extractBool(const std::string& json, const std::string& key, bool default_value = false);
    
    /**
     * Build JSON object from key-value pairs
     */
    std::string buildObject(const std::vector<std::pair<std::string, std::string>>& stringFields,
                           const std::vector<std::pair<std::string, int>>& intFields = {},
                           const std::vector<std::pair<std::string, bool>>& boolFields = {});
    
    /**
     * Build JSON array from string values
     */
    std::string buildArray(const std::vector<std::string>& values);
    
    /**
     * Escape string for JSON
     */
    std::string escapeString(const std::string& str);
}

} // namespace MillionaireGame

#endif // JSON_PARSER_H

