#include "teamforge/tools/tool_registry.hpp"
#include "teamforge/tools/builtin_tools.hpp"

#include "teamforge/telemetry/logger.hpp"

namespace teamforge::tools {

void ToolRegistry::register_tool(std::shared_ptr<ITool> tool) {
    tools_[tool->descriptor().name] = std::move(tool);
}

Result<ToolOutput> ToolRegistry::execute(const ToolInput& input, ExecutionContext& ctx) const {
    if (!ctx.allowed_tools.allows(input.tool_name)) {
        return Result<ToolOutput>::err("Tool not allowed for agent: " + input.tool_name);
    }
    const auto it = tools_.find(input.tool_name);
    if (it == tools_.end()) {
        return Result<ToolOutput>::err("Unknown tool: " + input.tool_name);
    }
    telemetry::Logger::debug("Executing tool: " + input.tool_name);
    return it->second->execute(input, ctx);
}

std::vector<ToolDescriptor> ToolRegistry::list_tools() const {
    std::vector<ToolDescriptor> result;
    for (const auto& [_, tool] : tools_) {
        result.push_back(tool->descriptor());
    }
    return result;
}

bool ToolRegistry::has_tool(const std::string& name) const {
    return tools_.count(name) > 0;
}

}  // namespace teamforge::tools
