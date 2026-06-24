#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <iostream>

namespace rapiddesk::core {

    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    class Logger {
    public:
        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        void set_level(LogLevel level) {
            std::lock_guard<std::mutex> lock(mutex_);
            level_ = level;
        }

        void log(LogLevel level, const std::string& message) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (level < level_) return;

            const char* prefix = "";
            switch (level) {
            case LogLevel::Debug:   prefix = "[DEBUG] "; break;
            case LogLevel::Info:    prefix = "[INFO]  "; break;
            case LogLevel::Warning: prefix = "[WARN]  "; break;
            case LogLevel::Error:   prefix = "[ERROR] "; break;
            }
            std::cout << prefix << message << std::endl;
        }

        template<typename... Args>
        void debug(Args&&... args) { log(LogLevel::Debug, format(std::forward<Args>(args)...)); }

        template<typename... Args>
        void info(Args&&... args) { log(LogLevel::Info, format(std::forward<Args>(args)...)); }

        template<typename... Args>
        void warn(Args&&... args) { log(LogLevel::Warning, format(std::forward<Args>(args)...)); }

        template<typename... Args>
        void error(Args&&... args) { log(LogLevel::Error, format(std::forward<Args>(args)...)); }

    private:
        Logger() = default;
        ~Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        LogLevel level_ = LogLevel::Debug;
        std::mutex mutex_;

        template<typename... Args>
        static std::string format(Args&&... args) {
            // Simplificado - concatena tudo em string
            std::string result;
            ((result += to_string(std::forward<Args>(args))), ...);
            return result;
        }

        template<typename T>
        static std::string to_string(T&& value) {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                return std::forward<T>(value);
            }
            else {
                return std::to_string(std::forward<T>(value));
            }
        }

        static std::string to_string(const char* value) { return value; }
    };

    // Macros de conveniência
#define LOG_DEBUG(...) rapiddesk::core::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)  rapiddesk::core::Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)  rapiddesk::core::Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) rapiddesk::core::Logger::instance().error(__VA_ARGS__)

} // namespace rapiddesk::core