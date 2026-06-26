#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

namespace teamforge::domain {

using TaskId = std::string;
using SubtaskId = std::string;
using MessageId = std::string;
using ArtifactId = std::string;
using EventId = std::string;
using MemoryId = std::string;
using ValidatorId = std::string;

using Timestamp = std::chrono::system_clock::time_point;

inline std::string generate_id(const std::string& prefix) {
    static std::uint64_t counter = 0;
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return prefix + "-" + std::to_string(now) + "-" + std::to_string(++counter);
}

inline std::string timestamp_to_iso(Timestamp ts) {
    const auto tt = std::chrono::system_clock::to_time_t(ts);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&tt));
    return buf;
}

}  // namespace teamforge::domain
