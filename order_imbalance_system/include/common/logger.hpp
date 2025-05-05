// include/common/logger.h
#pragma once

#include <string>
#include <memory>
#include <sstream>

namespace trading {
namespace common {
// Add these to logger.hpp
#define LOG_TRACE(...) trading::common::Logger::getLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) trading::common::Logger::getLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) trading::common::Logger::getLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) trading::common::Logger::getLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) trading::common::Logger::getLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) trading::common::Logger::getLogger()->critical(__VA_ARGS__)
// Log levels
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
};

// Logger interface
class Logger {
public:
    static void initialize(const std::string& app_name, LogLevel level);
    
    static std::shared_ptr<Logger> getLogger();
    
    virtual ~Logger() = default;
    
    // Log methods
    template<typename... Args>
    void trace(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::TRACE)) {
            log(LogLevel::TRACE, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::DEBUG)) {
            log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::INFO)) {
            log(LogLevel::INFO, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void warn(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::WARN)) {
            log(LogLevel::WARN, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::ERROR)) {
            log(LogLevel::ERROR, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void critical(const std::string& format, Args&&... args) {
        if (shouldLog(LogLevel::CRITICAL)) {
            log(LogLevel::CRITICAL, format, std::forward<Args>(args)...);
        }
    }
    
    // Set log level
    virtual void setLevel(LogLevel level) = 0;
    
    // Check if should log at level
    virtual bool shouldLog(LogLevel level) const = 0;
    
protected:
    Logger() = default;
    
    // Format and log message
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        std::string message = formatString(format, std::forward<Args>(args)...);
        logMessage(level, message);
    }
    
    // Format string with arguments
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        // Simple format implementation - in production you would use a proper formatting library
        // like fmt or boost::format
        std::ostringstream ss;
        
        size_t pos = 0;
        size_t next_pos = format.find('{}', pos);
        
        if constexpr (sizeof...(args) == 0) {
            return format;
        } else {
            formatStringHelper(ss, format, pos, next_pos, std::forward<Args>(args)...);
        }
        
        return ss.str();
    }
    
    // Helper for formatting strings
    template<typename T, typename... Args>
    void formatStringHelper(std::ostringstream& ss, const std::string& format, 
                          size_t& pos, size_t& next_pos, T&& arg, Args&&... args) {
        if (next_pos == std::string::npos) {
            ss << format.substr(pos);
            return;
        }
        
        ss << format.substr(pos, next_pos - pos);
        ss << arg;
        
        pos = next_pos + 2;  // Skip {}
        next_pos = format.find('{}', pos);
        
        if constexpr (sizeof...(args) > 0) {
            formatStringHelper(ss, format, pos, next_pos, std::forward<Args>(args)...);
        } else {
            ss << format.substr(pos);
        }
    }
    
    // Log formatted message
    virtual void logMessage(LogLevel level, const std::string& message) = 0;
    
private:
    static std::shared_ptr<Logger> instance_;
};

} // namespace common
} // namespace trading
