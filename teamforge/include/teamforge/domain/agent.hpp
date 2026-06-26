#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/subtask.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace teamforge::domain {

struct ToolCapabilitySet {
    std::set<std::string> tools;

    bool allows(const std::string& tool_name) const {
        return tools.find(tool_name) != tools.end();
    }
};

struct AgentConstraints {
    int max_tokens{4096};
    int timeout_seconds{120};
    int max_tool_calls{10};
};

struct MemoryScope {
    MemoryTier tier{MemoryTier::Task};
    TaskId task_id;
};

struct AgentInput {
    TaskId task_id;
    SubtaskId subtask_id;
    AgentRole assigned_role{AgentRole::Orchestrator};
    std::string objective;
    nlohmann::json context_refs;
    ToolCapabilitySet allowed_tools;
    MemoryScope memory_scope;
    AgentConstraints constraints;
};

struct AgentDescriptor {
    AgentRole role{AgentRole::Orchestrator};
    std::string system_prompt_template;
    nlohmann::json output_schema;
    ToolCapabilitySet tools;
    ValidatorId validator;
};

struct Finding {
    std::string severity;
    std::string message;
    std::string location;
};

struct ValidationResult {
    ValidatorId validator;
    bool passed{false};
    double score{0.0};
    std::vector<Finding> findings;
    Verdict verdict{Verdict::Block};

    nlohmann::json to_json() const;
};

struct MemoryEntry {
    MemoryId id;
    MemoryTier tier{MemoryTier::Task};
    std::string key;
    nlohmann::json value;
    std::optional<TaskId> scope;
    Timestamp ttl{std::chrono::system_clock::now()};
};

}  // namespace teamforge::domain
