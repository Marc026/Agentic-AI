#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace teamforge::domain {

struct TaskBudget {
    int max_subtasks{20};
    int max_revisions{5};
    int max_wall_time_seconds{3600};
};

struct AcceptanceCriteria {
    std::vector<std::string> required_artifact_kinds;
    std::vector<std::string> success_conditions;
};

struct Task {
    TaskId id;
    std::string user_request;
    TaskStatus status{TaskStatus::Created};
    Timestamp created_at{std::chrono::system_clock::now()};
    TaskBudget budget;
    AcceptanceCriteria criteria;

    nlohmann::json to_json() const;
    static Task from_json(const nlohmann::json& j);
};

}  // namespace teamforge::domain
