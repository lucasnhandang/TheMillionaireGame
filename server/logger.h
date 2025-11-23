#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace MillionaireGame {

/**
 * Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

/**
 * Logger class for server logging and debugging
 */
class Logger {
public:
    /**
     * Get singleton instance
     */
    static Logger& getInstance();
    
    /**
     * Initialize logger
     * @param log_file Path to log file (empty = stdout only)
     * @param min_level Minimum log level to output
     */
    void initialize(const std::string& log_file = "", LogLevel min_level = LogLevel::INFO);
    
    /**
     * Log a message
     * @param level Log level
     * @param message Message to log
     * @param file Source file (optional)
     * @param line Source line (optional)
     */
    void log(LogLevel level, const std::string& message, 
             const std::string& file = "", int line = 0);
    
    /**
     * Convenience methods
     */
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    
    /**
     * Set minimum log level
     */
    void setMinLevel(LogLevel level);
    
    /**
     * Close logger
     */
    void close();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::mutex log_mutex_;
    std::unique_ptr<std::ofstream> log_file_;
    LogLevel min_level_;
    bool initialized_;
    
    std::string levelToString(LogLevel level);
    std::string getTimestamp();
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::getInstance().error(msg, __FILE__, __LINE__)

} // namespace MillionaireGame

#endif // LOGGER_H

