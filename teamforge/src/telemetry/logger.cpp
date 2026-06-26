#include "teamforge/telemetry/logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace teamforge::telemetry {

void Logger::init(const std::string& level) {
    auto console = spdlog::stdout_color_mt("teamforge");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    if (level == "debug") spdlog::set_level(spdlog::level::debug);
    else if (level == "warn") spdlog::set_level(spdlog::level::warn);
    else if (level == "error") spdlog::set_level(spdlog::level::err);
    else spdlog::set_level(spdlog::level::info);
}

}  // namespace teamforge::telemetry
