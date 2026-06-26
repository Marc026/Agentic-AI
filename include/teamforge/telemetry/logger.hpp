#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace teamforge::telemetry {

class Logger {
public:
    static void init(const std::string& level = "info");

    template <typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::debug(fmt, std::forward<Args>(args)...);
    }
};

struct Metrics {
    int tasks_completed{0};
    int tasks_failed{0};
    int total_revisions{0};
    int gate_pass_first_try{0};
    int gate_total{0};
    double total_wall_time_seconds{0.0};

    void record_gate(bool first_pass) {
        ++gate_total;
        if (first_pass) ++gate_pass_first_try;
    }

    double gate_pass_rate() const {
        if (gate_total == 0) return 0.0;
        return static_cast<double>(gate_pass_first_try) / gate_total;
    }
};

}  // namespace teamforge::telemetry
