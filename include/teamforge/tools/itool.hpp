#pragma once

#include "teamforge/core/result.hpp"
#include "teamforge/domain/agent.hpp"

#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <string>

namespace teamforge::tools {

struct ToolDescriptor {
    std::string name;
    std::string description;
    nlohmann::json input_schema;
};

struct ToolInput {
    std::string tool_name;
    nlohmann::json arguments;
};

struct ToolOutput {
    bool success{false};
    nlohmann::json result;
    std::string stderr_output;
};

struct ExecutionContext {
    std::filesystem::path workspace_root;
    int timeout_seconds{30};
    domain::ToolCapabilitySet allowed_tools;
};

class ITool {
public:
    virtual ~ITool() = default;
    virtual ToolDescriptor descriptor() const = 0;
    virtual Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) = 0;
};

}  // namespace teamforge::tools
