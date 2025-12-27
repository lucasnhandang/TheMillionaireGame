#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>

using namespace std;

namespace MillionaireGame {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const string& log_file, LogLevel min_level) {
    lock_guard<mutex> lock(log_mutex_);
    
    min_level_ = min_level;
    
    if (!log_file.empty()) {
        log_file_ = make_unique<ofstream>(log_file, ios::app);
        if (!log_file_->is_open()) {
            cerr << "Warning: Failed to open log file: " << log_file << endl;
            log_file_.reset();
        }
    }
    
    initialized_ = true;
}

void Logger::log(LogLevel level, const string& message, 
                 const string& file, int line) {
    if (!initialized_ || level < min_level_) {
        return;
    }
    
    lock_guard<mutex> lock(log_mutex_);
    
    ostringstream log_entry;
    log_entry << "[" << getTimestamp() << "] "
              << "[" << levelToString(level) << "] ";
    
    if (!file.empty() && line > 0) {
        // Extract filename from path
        size_t last_slash = file.find_last_of("/\\");
        string filename = (last_slash != string::npos) 
                               ? file.substr(last_slash + 1) 
                               : file;
        log_entry << "[" << filename << ":" << line << "] ";
    }
    
    log_entry << message;
    
    string log_str = log_entry.str();
    
    // Output to stdout
    if (level >= LogLevel::WARNING) {
        cerr << log_str << endl;
    } else {
        cout << log_str << endl;
    }
    
    // Output to file if available
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << log_str << endl;
        log_file_->flush();
    }
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::setMinLevel(LogLevel level) {
    lock_guard<mutex> lock(log_mutex_);
    min_level_ = level;
}

void Logger::close() {
    lock_guard<mutex> lock(log_mutex_);
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
    log_file_.reset();
}

Logger::~Logger() {
    close();
}

string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

string Logger::getTimestamp() {
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    
    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace MillionaireGame

