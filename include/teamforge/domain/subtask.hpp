#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace teamforge::domain {

struct ArtifactRef {
    ArtifactId id;
    ArtifactKind kind{ArtifactKind::Other};
    std::string path;
};

struct ValidationHints {
    bool requires_schema_validation{true};
    std::string schema_name;
};

struct AgentOutput {
    AgentStatus status{AgentStatus::Failed};
    nlohmann::json structured_payload;
    std::vector<ArtifactRef> artifacts;
    ValidationHints validation_hints;
};

struct Subtask {
    SubtaskId id;
    TaskId parent;
    AgentRole assigned_role{AgentRole::Orchestrator};
    std::string objective;
    SubtaskStatus status{SubtaskStatus::Pending};
    std::vector<SubtaskId> depends_on;
    GateType gate{GateType::None};
    int revision_count{0};
    int retry_count{0};
    std::optional<AgentOutput> last_output;

    nlohmann::json to_json() const;
    static Subtask from_json(const nlohmann::json& j);
};

struct TaskPlan {
    std::vector<Subtask> subtasks;
    nlohmann::json metadata;

    nlohmann::json to_json() const;
    static TaskPlan from_json(const nlohmann::json& j);
};

}  // namespace teamforge::domain
