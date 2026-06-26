#pragma once

#include "teamforge/agents/iagent.hpp"
#include "teamforge/llm/llm_provider.hpp"
#include "teamforge/memory/task_context.hpp"
#include "teamforge/tools/tool_registry.hpp"

#include <filesystem>
#include <memory>

namespace teamforge::agents {

class AgentRunner {
public:
    AgentRunner(llm::LLMProvider& llm, tools::ToolRegistry& tools, memory::TaskContext& context,
                std::filesystem::path agents_dir, std::filesystem::path workspace);

    Result<domain::AgentOutput> run(const domain::AgentInput& input);

private:
    llm::LLMProvider& llm_;
    tools::ToolRegistry& tools_;
    memory::TaskContext& context_;
    std::filesystem::path agents_dir_;
    std::filesystem::path workspace_;

    domain::AgentDescriptor load_descriptor(domain::AgentRole role) const;
    std::string build_system_prompt(const domain::AgentDescriptor& desc,
                                    const domain::AgentInput& input) const;
    Result<domain::AgentOutput> parse_output(domain::AgentRole role,
                                             const std::string& content) const;
    Result<void> apply_side_effects(domain::AgentRole role, domain::AgentOutput& output);
};

domain::ToolCapabilitySet default_tools_for_role(domain::AgentRole role);

}  // namespace teamforge::agents
