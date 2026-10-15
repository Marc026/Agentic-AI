#include "teamforge/telemetry/logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace teamforge::telemetry {

namespace {

void log_message(spdlog::level::level_enum level, const std::string& message) {
    auto* logger = spdlog::default_logger_raw();
    if (logger == nullptr) return;
    logger->log(spdlog::source_loc{}, level, spdlog::string_view_t{message});
}

}  // namespace

void Logger::init(const std::string& level) {
    auto console = spdlog::stdout_color_mt("teamforge");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    if (level == "debug") spdlog::set_level(spdlog::level::debug);
    else if (level == "warn") spdlog::set_level(spdlog::level::warn);
    else if (level == "error") spdlog::set_level(spdlog::level::err);
    else spdlog::set_level(spdlog::level::info);
}

void Logger::info(const std::string& message) { log_message(spdlog::level::info, message); }

void Logger::warn(const std::string& message) { log_message(spdlog::level::warn, message); }

void Logger::error(const std::string& message) { log_message(spdlog::level::err, message); }

void Logger::debug(const std::string& message) { log_message(spdlog::level::debug, message); }

}  // namespace teamforge::telemetry
