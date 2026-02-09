//
// Logger module for TextFoundry
//

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace tf {

/**
 * Logger levels
 */
enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };

/**
 * Logger initialization and global accessor
 */
class Logger {
 public:
  /**
   * Initialize default console logger
   * Call once at application startup
   */
  static void init(LogLevel level = LogLevel::Info);

  /**
   * Initialize logger with custom sink
   */
  static void init(const std::string& logFile, LogLevel level = LogLevel::Info);

  /**
   * Shutdown logger and release resources
   */
  static void shutdown();

  /**
   * Get the global logger instance
   */
  [[nodiscard]] static std::shared_ptr<spdlog::logger> get();

  /**
   * Set global log level
   */
  static void SetLevel(LogLevel level);

 private:
  static std::shared_ptr<spdlog::logger> instance_;
  static bool initialized_;
};

/**
 * Helper macros for logging with source location
 */
#define TF_LOG_TRACE(...) ::tf::Logger::get()->trace(__VA_ARGS__)
#define TF_LOG_DEBUG(...) ::tf::Logger::get()->debug(__VA_ARGS__)
#define TF_LOG_INFO(...) ::tf::Logger::get()->info(__VA_ARGS__)
#define TF_LOG_WARN(...) ::tf::Logger::get()->warn(__VA_ARGS__)
#define TF_LOG_ERROR(...) ::tf::Logger::get()->error(__VA_ARGS__)
#define TF_LOG_CRITICAL(...) ::tf::Logger::get()->critical(__VA_ARGS__)

}  // namespace tf
