#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>

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
     * Extract boolean value from JSON
     * Looks for "key":true/false pattern
     */
    bool extractBool(const std::string& json, const std::string& key, bool default_value = false);
}

} // namespace MillionaireGame

#endif // JSON_UTILS_H

