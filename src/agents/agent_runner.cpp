#include "teamforge/agents/agent_runner.hpp"

#include "teamforge/telemetry/logger.hpp"

#include <fstream>

namespace teamforge::agents {

namespace {

domain::GateType parse_gate(const std::string& g) {
    if (g == "Review") return domain::GateType::Review;
    if (g == "Test") return domain::GateType::Test;
    if (g == "Security") return domain::GateType::Security;
    return domain::GateType::None;
}

}  // namespace

domain::ToolCapabilitySet default_tools_for_role(domain::AgentRole role) {
    domain::ToolCapabilitySet caps;
    switch (role) {
        case domain::AgentRole::Orchestrator:
            caps.tools = {"json_validate"};
            break;
        case domain::AgentRole::Architect:
            caps.tools = {"file_read", "file_list", "code_search", "json_validate"};
            break;
        case domain::AgentRole::Backend:
            caps.tools = {"file_read", "file_write", "file_list", "code_search", "shell_execute",
                          "json_validate"};
            break;
        case domain::AgentRole::TestEngineer:
            caps.tools = {"file_read", "file_write", "test_execute", "code_search", "json_validate"};
            break;
        case domain::AgentRole::CodeReviewer:
            caps.tools = {"file_read", "code_search", "git_diff", "json_validate"};
            break;
        default:
            caps.tools = {"file_read", "json_validate"};
            break;
    }
    return caps;
}

AgentRunner::AgentRunner(llm::LLMProvider& llm, tools::ToolRegistry& tools,
                         memory::TaskContext& context, std::filesystem::path agents_dir,
                         std::filesystem::path workspace)
    : llm_(llm),
      tools_(tools),
      context_(context),
      agents_dir_(std::move(agents_dir)),
      workspace_(std::move(workspace)) {}

domain::AgentDescriptor AgentRunner::load_descriptor(domain::AgentRole role) const {
    const auto filename = domain::to_string(role) + ".json";
    const auto path = agents_dir_ / filename;

    domain::AgentDescriptor desc;
    desc.role = role;
    desc.tools = default_tools_for_role(role);

    if (std::filesystem::exists(path)) {
        std::ifstream file(path);
        const auto j = nlohmann::json::parse(file);
        desc.system_prompt_template = j.value("system_prompt", "");
        desc.output_schema = j.value("output_schema", nlohmann::json::object());
        desc.validator = j.value("validator", domain::to_string(role));
        if (j.contains("tools")) {
            desc.tools.tools.clear();
            for (const auto& t : j.at("tools")) desc.tools.tools.insert(t.get<std::string>());
        }
    } else {
        desc.system_prompt_template = "You are the " + domain::to_string(role) +
                                      " agent. Respond with valid JSON only.";
        desc.validator = domain::to_string(role);
    }
    return desc;
}

std::string AgentRunner::build_system_prompt(const domain::AgentDescriptor& desc,
                                             const domain::AgentInput& input) const {
    return desc.system_prompt_template + "\n\nObjective: " + input.objective +
           "\n\nContext: " + input.context_refs.dump() +
           "\n\nTaskContext: " + context_.snapshot().dump();
}

Result<domain::AgentOutput> AgentRunner::parse_output(domain::AgentRole role,
                                                      const std::string& content) const {
    domain::AgentOutput output;
    try {
        output.structured_payload = nlohmann::json::parse(content);
    } catch (const nlohmann::json::exception&) {
        const auto start = content.find('{');
        const auto end = content.rfind('}');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return Result<domain::AgentOutput>::err("Failed to parse agent JSON output");
        }
        try {
            output.structured_payload = nlohmann::json::parse(content.substr(start, end - start + 1));
        } catch (const nlohmann::json::exception&) {
            return Result<domain::AgentOutput>::err("Failed to parse agent JSON output");
        }
    }

    output.status = domain::AgentStatus::Success;
    if (role == domain::AgentRole::CodeReviewer) {
        const auto verdict = output.structured_payload.value("verdict", "Block");
        if (verdict != "Approve") output.status = domain::AgentStatus::NeedsRevision;
    }
    if (role == domain::AgentRole::TestEngineer) {
        if (!output.structured_payload.value("passed", false)) {
            output.status = domain::AgentStatus::Blocked;
        }
    }
    return Result<domain::AgentOutput>::ok(std::move(output));
}

Result<void> AgentRunner::apply_side_effects(domain::AgentRole role, domain::AgentOutput& output) {
    if (role == domain::AgentRole::Backend && output.structured_payload.contains("files")) {
        tools::ExecutionContext ctx{workspace_, 30, default_tools_for_role(role)};
        for (const auto& f : output.structured_payload.at("files")) {
            tools::ToolInput tin;
            tin.tool_name = "file_write";
            tin.arguments = {{"path", f.at("path")}, {"content", f.at("content")}};
            auto res = tools_.execute(tin, ctx);
            if (res.is_err()) return Result<void>::err(res.error());

            domain::Artifact artifact;
            artifact.id = domain::generate_id("art");
            artifact.task_id = context_.task_id();
            artifact.kind = domain::ArtifactKind::SourceFile;
            artifact.path = f.at("path").get<std::string>();
            artifact.checksum = std::to_string(f.at("content").get<std::string>().size());
            context_.add_artifact(artifact);
            output.artifacts.push_back({artifact.id, artifact.kind, artifact.path});
        }
    }

    if (role == domain::AgentRole::TestEngineer) {
        if (output.structured_payload.contains("test_files")) {
            tools::ExecutionContext ctx{workspace_, 30, default_tools_for_role(role)};
            for (const auto& f : output.structured_payload.at("test_files")) {
                tools::ToolInput tin;
                tin.tool_name = "file_write";
                tin.arguments = {{"path", f.at("path")}, {"content", f.at("content")}};
                auto res = tools_.execute(tin, ctx);
                if (res.is_err()) return Result<void>::err(res.error());
            }
        }
        if (output.structured_payload.contains("test_command")) {
            tools::ExecutionContext ctx{workspace_, 60, default_tools_for_role(role)};
            tools::ToolInput tin;
            tin.tool_name = "test_execute";
            tin.arguments = {{"command", output.structured_payload.at("test_command")}};
            auto res = tools_.execute(tin, ctx);
            if (res.is_ok()) {
                output.structured_payload["passed"] = res.value().success;
            }
        }
    }

    if (role == domain::AgentRole::Architect) {
        domain::Artifact artifact;
        artifact.id = domain::generate_id("art");
        artifact.task_id = context_.task_id();
        artifact.kind = domain::ArtifactKind::DesignDoc;
        artifact.path = "design_doc.json";
        artifact.checksum = output.structured_payload.dump();
        context_.add_artifact(artifact);
        output.artifacts.push_back({artifact.id, artifact.kind, artifact.path});
    }

    return Result<void>::ok();
}

Result<domain::AgentOutput> AgentRunner::run(const domain::AgentInput& input) {
    const auto desc = load_descriptor(input.assigned_role);
    llm::CompletionRequest req;
    req.messages = {{"system", build_system_prompt(desc, input)}, {"user", input.objective}};
    req.max_tokens = input.constraints.max_tokens;

    telemetry::Logger::info("Running agent " + domain::to_string(desc.role) + " for subtask " +
                            input.subtask_id);

    auto llm_result = llm_.complete(req);
    if (llm_result.is_err()) return Result<domain::AgentOutput>::err(llm_result.error());

    auto parsed = parse_output(desc.role, llm_result.value().content);
    if (parsed.is_err()) return parsed;

    auto output = parsed.value();
    auto effects = apply_side_effects(desc.role, output);
    if (effects.is_err()) return Result<domain::AgentOutput>::err(effects.error());

    return Result<domain::AgentOutput>::ok(std::move(output));
}

}  // namespace teamforge::agents
