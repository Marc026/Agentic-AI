#pragma once

#include "teamforge/domain/enums.hpp"
#include "teamforge/domain/types.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace teamforge::domain {

struct AgentMessage {
    MessageId id;
    TaskId task_id;
    AgentRole from{AgentRole::Orchestrator};
    AgentRole to{AgentRole::Orchestrator};
    MessageKind kind{MessageKind::Event};
    nlohmann::json payload;
    Timestamp ts{std::chrono::system_clock::now()};

    nlohmann::json to_json() const;
    static AgentMessage from_json(const nlohmann::json& j);
};

}  // namespace teamforge::domain
