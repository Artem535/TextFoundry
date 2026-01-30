//
// Logger implementation for TextFoundry
//

#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace tf {

std::shared_ptr<spdlog::logger> Logger::instance_ = nullptr;
bool Logger::initialized_ = false;

static spdlog::level::level_enum toSpdlogLevel(const LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warn:     return spdlog::level::warn;
        case LogLevel::Error:    return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
        default:                 return spdlog::level::info;
    }
}

void Logger::init(LogLevel level) {
    if (initialized_) {
        return;
    }

    instance_ = spdlog::stdout_color_mt("textfoundry");
    instance_->set_level(toSpdlogLevel(level));
    instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    initialized_ = true;
}

void Logger::init(const std::string& logFile, LogLevel level) {
    if (initialized_) {
        return;
    }

    instance_ = spdlog::basic_logger_mt("textfoundry", logFile);
    instance_->set_level(toSpdlogLevel(level));
    instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    initialized_ = true;
}

void Logger::shutdown() {
    instance_.reset();
    initialized_ = false;
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!initialized_) {
        init(LogLevel::Info);
    }
    return instance_;
}

void Logger::setLevel(LogLevel level) {
    if (instance_) {
        instance_->set_level(toSpdlogLevel(level));
    }
}

} // namespace tf
