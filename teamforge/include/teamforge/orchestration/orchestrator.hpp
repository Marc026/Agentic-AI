#pragma once

#include "teamforge/agents/agent_runner.hpp"
#include "teamforge/domain/task.hpp"
#include "teamforge/messaging/event_log.hpp"
#include "teamforge/memory/task_context.hpp"
#include "teamforge/orchestration/workflow_engine.hpp"
#include "teamforge/storage/sqlite_store.hpp"
#include "teamforge/telemetry/logger.hpp"
#include "teamforge/validation/validator_hub.hpp"

#include <chrono>
#include <filesystem>
#include <memory>

namespace teamforge::orchestration {

struct OrchestratorConfig {
    std::filesystem::path workspace;
    std::filesystem::path data_dir;
    std::filesystem::path agents_dir;
    std::filesystem::path schemas_dir;
    bool use_mock_llm{true};
    std::string llm_base_url{"http://localhost:11434"};
    std::string llm_model{"llama3"};
    std::string llm_api_key;
};

struct RunResult {
    domain::Task task;
    std::vector<domain::Artifact> artifacts;
    std::vector<domain::Event> events;
    telemetry::Metrics metrics;
    bool success{false};
    std::string message;
};

class Orchestrator {
public:
    explicit Orchestrator(OrchestratorConfig config);

    Result<RunResult> run(const std::string& user_request);
    Result<RunResult> replay(const domain::TaskId& task_id);
    Result<domain::Task> status(const domain::TaskId& task_id) const;

private:
    OrchestratorConfig config_;
    storage::SqliteStore store_;
    messaging::EventLog event_log_;
    memory::TaskContext context_;
    tools::ToolRegistry tools_;
    validation::ValidatorHub validators_;
    WorkflowEngine workflow_;
    RevisionPolicy revision_policy_;
    std::unique_ptr<llm::LLMProvider> llm_;
    std::unique_ptr<agents::AgentRunner> agent_runner_;
    telemetry::Metrics metrics_;

    Result<domain::TaskPlan> create_plan(domain::Task& task);
    domain::TaskPlan build_url_shortener_plan(const domain::TaskId& task_id) const;
    Result<void> execute_workflow(domain::Task& task);
    Result<void> process_subtask(domain::Task& task, const domain::Subtask& subtask);
    void persist_artifact(const domain::Artifact& artifact);
};

}  // namespace teamforge::orchestration
