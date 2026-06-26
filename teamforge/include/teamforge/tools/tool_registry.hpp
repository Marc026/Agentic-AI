#pragma once

#include "teamforge/tools/itool.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace teamforge::tools {

class ToolRegistry {
public:
    void register_tool(std::shared_ptr<ITool> tool);
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) const;
    std::vector<ToolDescriptor> list_tools() const;
    bool has_tool(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<ITool>> tools_;
};

}  // namespace teamforge::tools
