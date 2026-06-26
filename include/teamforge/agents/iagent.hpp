#pragma once

#include "teamforge/domain/agent.hpp"
#include "teamforge/core/result.hpp"

namespace teamforge::agents {

class IAgent {
public:
    virtual ~IAgent() = default;
    virtual domain::AgentRole role() const = 0;
    virtual domain::AgentDescriptor descriptor() const = 0;
};

}  // namespace teamforge::agents
