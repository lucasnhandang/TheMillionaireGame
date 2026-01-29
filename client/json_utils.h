#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <map>

namespace MillionaireGame {

/**
 * JSON parsing utilities
 * Simple JSON extraction functions for request parsing
 */
namespace JsonUtils {
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
     * Extract long long value from JSON
     * Looks for "key":value pattern
     */
    long long extractLongLong(const std::string& json, const std::string& key, long long default_value = 0);
    
    /**
     * Extract boolean value from JSON
     * Looks for "key":true/false pattern
     */
    bool extractBool(const std::string& json, const std::string& key, bool default_value = false);
    
    /**
     * Extract JSON value (array or object) from JSON
     * Returns the raw JSON value as string (e.g., "[1,3]" or "{\"A\":10,\"B\":65}")
     */
    std::string extractJsonValue(const std::string& json, const std::string& key);
    
    /**
     * Build JSON object from key-value pairs
     */
    std::string buildJson(const std::map<std::string, std::string>& stringFields,
                         const std::map<std::string, int>& intFields,
                         const std::map<std::string, bool>& boolFields);
}

} // namespace MillionaireGame

#endif // JSON_UTILS_H

