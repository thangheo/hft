#include "common/logger.hpp"
// #include <chrono>
// #include <iomanip>
// #include <iostream>
// #include <mutex>
// #include <sstream>

// namespace common {

// // Initialize static members
// std::mutex Logger::s_mutex;
// Logger::Level Logger::s_level = Logger::Level::INFO;
// std::ofstream Logger::s_file;
// bool Logger::s_console_output = true;

// Logger::Logger(const std::string& prefix) : m_prefix(prefix) {}

// Logger::~Logger() = default;

// void Logger::init(const std::string& log_file, Level level) {
//     std::lock_guard<std::mutex> lock(s_mutex);
//     s_level = level;
    
//     if (!log_file.empty() && !s_file.is_open()) {
//         s_file.open(log_file, std::ios::app);
//         if (!s_file) {
//             std::cerr << "Failed to open log file: " << log_file << std::endl;
//         }
//     }
// }

// void Logger::set_level(Level level) {
//     std::lock_guard<std::mutex> lock(s_mutex);
//     s_level = level;
// }

// std::string Logger::level_to_string(Level level) {
//     switch (level) {
//         case Level::DEBUG:   return "DEBUG";
//         case Level::INFO:    return "INFO";
//         case Level::WARNING: return "WARNING";
//         case Level::ERROR:   return "ERROR";
//         default:             return "UNKNOWN";
//     }
// }

// void Logger::log(Level level, const std::string& message) {
//     if (level < s_level) {
//         return;
//     }

//     auto now = std::chrono::system_clock::now();
//     auto time_t = std::chrono::system_clock::to_time_t(now);
//     auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//         now.time_since_epoch()) % 1000;

//     std::stringstream ss;
//     ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
//        << "." << std::setfill('0') << std::setw(3) << ms.count()
//        << " [" << level_to_string(level) << "]";
    
//     if (!m_prefix.empty()) {
//         ss << " [" << m_prefix << "]";
//     }
    
//     ss << " " << message;
//     std::string formatted_msg = ss.str();

//     std::lock_guard<std::mutex> lock(s_mutex);
    
//     if (s_console_output) {
//         std::cout << formatted_msg << std::endl;
//     }
    
//     if (s_file.is_open()) {
//         s_file << formatted_msg << std::endl;
//         s_file.flush();
//     }
// }

// void Logger::debug(const std::string& message) {
//     log(Level::DEBUG, message);
// }

// void Logger::info(const std::string& message) {
//     log(Level::INFO, message);
// }

// void Logger::warning(const std::string& message) {
//     log(Level::WARNING, message);
// }

// void Logger::error(const std::string& message) {
//     log(Level::ERROR, message);
// }

// void Logger::close() {
//     std::lock_guard<std::mutex> lock(s_mutex);
//     if (s_file.is_open()) {
//         s_file.close();
//     }
// }

// } // namespace common

// #include "common/logger.hpp"
// #include <iostream>
// #include <fstream>
// #include <mutex>
// #include <string>
// #include <ctime>

// namespace Logger {

//     namespace {
//         std::ofstream logFile;
//         std::mutex logMutex;

//         std::string getCurrentTime() {
//             std::time_t now = std::time(nullptr);
//             char buf[80];
//             std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
//             return std::string(buf);
//         }
//     }

//     void init(const std::string& fileName) {
//         std::lock_guard<std::mutex> lock(logMutex);
//         logFile.open(fileName, std::ios::app);
//         if (!logFile.is_open()) {
//             throw std::runtime_error("Failed to open log file: " + fileName);
//         }
//     }

//     void log(const std::string& message) {
//         std::lock_guard<std::mutex> lock(logMutex);
//         if (logFile.is_open()) {
//             logFile << "[" << getCurrentTime() << "] " << message << std::endl;
//         } else {
//             std::cerr << "Logger not initialized. Message: " << message << std::endl;
//         }
//     }

//     void close() {
//         std::lock_guard<std::mutex> lock(logMutex);
//         if (logFile.is_open()) {
//             logFile.close();
//         }
//     }

// } // namespace Logger

#include "common/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>

namespace trading {
namespace common {

// Static instance initialization
std::shared_ptr<Logger> Logger::instance_;

// Concrete logger implementation
class ConsoleLogger : public Logger {
public:
    explicit ConsoleLogger(const std::string& app_name, LogLevel level)
        : app_name_(app_name), level_(level) {}
    
    void setLevel(LogLevel level) override {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }
    
    bool shouldLog(LogLevel level) const override {
        return level >= level_;
    }
    
protected:
    void logMessage(LogLevel level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
            
        std::stringstream ss;
        ss << "[" << app_name_ << "] "
           << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
           << "[" << levelToString(level) << "] "
           << message;
           
        std::cout << ss.str() << std::endl;
    }
    
private:
    std::string app_name_;
    LogLevel level_;
    std::mutex mutex_;
    
    std::string levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            case LogLevel::OFF: return "OFF";
            default: return "UNKNOWN";
        }
    }
};

void Logger::initialize(const std::string& app_name, LogLevel level) {
    if (!instance_) {
        instance_ = std::make_shared<ConsoleLogger>(app_name, level);
    }
}

std::shared_ptr<Logger> Logger::getLogger() {
    if (!instance_) {
        // Auto-initialize with default settings if not already initialized
        initialize("Application", LogLevel::INFO);
    }
    return instance_;
}

} // namespace common
} // namespace trading