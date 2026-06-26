#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace teamforge::domain {

struct Event {
    EventId id;
    TaskId task_id;
    EventType type{EventType::TaskCreated};
    nlohmann::json details;
    Timestamp ts{std::chrono::system_clock::now()};

    nlohmann::json to_json() const;
    static Event from_json(const nlohmann::json& j);
};

}  // namespace teamforge::domain
