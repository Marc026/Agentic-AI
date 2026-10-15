#pragma once

#include <string>

namespace teamforge::telemetry {

class Logger {
public:
    static void init(const std::string& level = "info");
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void debug(const std::string& message);
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
