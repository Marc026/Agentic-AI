#include "teamforge/orchestration/orchestrator.hpp"

#include "teamforge/llm/mock_llm_provider.hpp"
#include "teamforge/tools/builtin_tools.hpp"

namespace teamforge::orchestration {

namespace {

domain::GateType parse_gate(const std::string& g) {
    if (g == "Review") return domain::GateType::Review;
    if (g == "Test") return domain::GateType::Test;
    if (g == "Security") return domain::GateType::Security;
    return domain::GateType::None;
}

}  // namespace

Orchestrator::Orchestrator(OrchestratorConfig config)
    : config_(std::move(config)),
      store_((config_.data_dir / "teamforge.db").string()),
      event_log_(store_),
      validators_(config_.schemas_dir),
      workflow_(WorkflowConfig{}),
      revision_policy_(5) {
    const auto init = store_.initialize();
    if (init.is_err()) {
        telemetry::Logger::error("Failed to initialize store: {}", init.error());
    }

    install_default_tools(tools_);

    if (config_.use_mock_llm) {
        llm_ = std::make_unique<llm::MockLLMProvider>();
    } else {
        llm_ = std::make_unique<llm::HttpLLMProvider>(config_.llm_base_url, config_.llm_api_key,
                                                      config_.llm_model);
    }

    agent_runner_ = std::make_unique<agents::AgentRunner>(*llm_, tools_, context_, config_.agents_dir,
                                                          config_.workspace);
}

domain::TaskPlan Orchestrator::build_url_shortener_plan(const domain::TaskId& task_id) const {
    domain::TaskPlan plan;
    auto make = [&](const char* id, domain::AgentRole role, const char* obj,
                    std::initializer_list<const char*> deps, domain::GateType gate) {
        domain::Subtask s;
        s.id = id;
        s.parent = task_id;
        s.assigned_role = role;
        s.objective = obj;
        s.gate = gate;
        for (const auto* d : deps) s.depends_on.push_back(d);
        plan.subtasks.push_back(std::move(s));
    };

    make("st-arch", domain::AgentRole::Architect, "Design URL shortener REST API", {},
         domain::GateType::None);
    make("st-rev-design", domain::AgentRole::CodeReviewer, "Review design document", {"st-arch"},
         domain::GateType::Review);
    make("st-backend", domain::AgentRole::Backend, "Implement URL shortener service", {"st-rev-design"},
         domain::GateType::None);
    make("st-test", domain::AgentRole::TestEngineer, "Generate and run tests for URL shortener",
         {"st-backend"}, domain::GateType::Test);
    make("st-rev-code", domain::AgentRole::CodeReviewer, "Review implementation code", {"st-backend"},
         domain::GateType::Review);
    return plan;
}

Result<domain::TaskPlan> Orchestrator::create_plan(domain::Task& task) {
    if (task.user_request.find("URL shortener") != std::string::npos ||
        task.user_request.find("url shortener") != std::string::npos) {
        return Result<domain::TaskPlan>::ok(build_url_shortener_plan(task.id));
    }

    domain::AgentInput input;
    input.task_id = task.id;
    input.subtask_id = "plan";
    input.assigned_role = domain::AgentRole::Orchestrator;
    input.objective = "Decompose this request into a task graph: " + task.user_request;
    input.allowed_tools = agents::default_tools_for_role(domain::AgentRole::Orchestrator);
    input.memory_scope = {domain::MemoryTier::Task, task.id};

    auto output = agent_runner_->run(input);
    if (output.is_err()) return Result<domain::TaskPlan>::err(output.error());

    domain::TaskPlan plan;
    for (const auto& s : output.value().structured_payload.at("subtasks")) {
        domain::Subtask sub;
        sub.id = s.at("id").get<std::string>();
        sub.parent = task.id;
        sub.assigned_role = domain::agent_role_from_string(s.at("assigned_role").get<std::string>());
        sub.objective = s.at("objective").get<std::string>();
        if (s.contains("depends_on")) {
            sub.depends_on = s.at("depends_on").get<std::vector<domain::SubtaskId>>();
        }
        if (s.contains("gate")) {
            sub.gate = parse_gate(s.at("gate").get<std::string>());
        }
        plan.subtasks.push_back(std::move(sub));
    }
    return Result<domain::TaskPlan>::ok(std::move(plan));
}

Result<void> Orchestrator::process_subtask(domain::Task& task, const domain::Subtask& subtask) {
    workflow_.mark_running(subtask.id);
    event_log_.append(task.id, domain::EventType::SubtaskStarted,
                      {{"subtask_id", subtask.id}, {"role", domain::to_string(subtask.assigned_role)}});

    domain::AgentInput input;
    input.task_id = task.id;
    input.subtask_id = subtask.id;
    input.objective = subtask.objective;
    input.assigned_role = subtask.assigned_role;
    input.allowed_tools = agents::default_tools_for_role(subtask.assigned_role);
    input.memory_scope = {domain::MemoryTier::Task, task.id};
    input.context_refs = context_.snapshot();

    auto output_result = agent_runner_->run(input);
    if (output_result.is_err()) {
        workflow_.mark_failed(subtask.id, output_result.error());
        event_log_.append(task.id, domain::EventType::SubtaskFailed,
                          {{"subtask_id", subtask.id}, {"error", output_result.error()}});
        return Result<void>::err(output_result.error());
    }

    auto output = output_result.value();
    event_log_.append(task.id, domain::EventType::AgentOutputReceived,
                      {{"subtask_id", subtask.id}, {"payload", output.structured_payload}});

    const auto validation = validators_.validate_agent_output(subtask.assigned_role, output);
    event_log_.append(task.id,
                      validation.passed ? domain::EventType::ValidationPassed
                                        : domain::EventType::ValidationFailed,
                      validation.to_json());

    if (!validation.passed) {
        metrics_.record_gate(false);
        if (revision_policy_.can_revise(subtask)) {
            workflow_.request_revision(subtask.id);
            event_log_.append(task.id, domain::EventType::RevisionRequested,
                              {{"subtask_id", subtask.id}, {"findings", validation.findings.size()}});
            return Result<void>::ok();
        }
        workflow_.mark_failed(subtask.id, "Validation failed");
        return Result<void>::err("Validation failed for subtask: " + subtask.id);
    }

    if (subtask.gate != domain::GateType::None) {
        metrics_.record_gate(subtask.revision_count == 0);
    }

    for (const auto& ref : output.artifacts) {
        domain::Artifact artifact;
        artifact.id = ref.id;
        artifact.task_id = task.id;
        artifact.kind = ref.kind;
        artifact.path = ref.path;
        artifact.metadata = output.structured_payload;
        persist_artifact(artifact);
    }

    workflow_.mark_completed(subtask.id, output);
    if (const auto* updated = workflow_.get_subtask(subtask.id)) {
        store_.save_subtask(*updated);
    }
    event_log_.append(task.id, domain::EventType::SubtaskCompleted, {{"subtask_id", subtask.id}});
    return Result<void>::ok();
}

Result<void> Orchestrator::execute_workflow(domain::Task& task) {
    const auto start = std::chrono::steady_clock::now();
    int iterations = 0;
    const int max_iterations = 100;

    while (!workflow_.is_complete() && !workflow_.has_failed()) {
        if (++iterations > max_iterations) {
            return Result<void>::err("Runaway loop detected");
        }

        workflow_.refresh_ready_states();
        auto ready = workflow_.ready_subtasks();
        if (ready.empty()) {
            if (workflow_.has_failed()) break;
            return Result<void>::err("Workflow deadlocked with no ready subtasks");
        }

        for (const auto& subtask : ready) {
            auto result = process_subtask(task, subtask);
            if (result.is_err() && workflow_.has_failed()) {
                return result;
            }
        }
    }

    metrics_.total_wall_time_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return Result<void>::ok();
}

void Orchestrator::persist_artifact(const domain::Artifact& artifact) {
    store_.save_artifact(artifact);
    context_.add_artifact(artifact);
}

Result<RunResult> Orchestrator::run(const std::string& user_request) {
    domain::Task task;
    task.id = domain::generate_id("task");
    task.user_request = user_request;
    task.status = domain::TaskStatus::Running;
    task.criteria.required_artifact_kinds = {"DesignDoc", "SourceFile", "TestReport"};

    context_.set_task_id(task.id);
    std::filesystem::create_directories(config_.workspace);
    std::filesystem::create_directories(config_.data_dir / "artifacts" / task.id);

    store_.save_task(task);
    event_log_.append(task.id, domain::EventType::TaskCreated, {{"request", user_request}});

    auto plan_result = create_plan(task);
    if (plan_result.is_err()) {
        task.status = domain::TaskStatus::Failed;
        store_.save_task(task);
        return Result<RunResult>::err(plan_result.error());
    }

    const auto plan = plan_result.value();
    event_log_.append(task.id, domain::EventType::PlanCreated, plan.to_json());

    auto load_result = workflow_.load_plan(plan);
    if (load_result.is_err()) {
        task.status = domain::TaskStatus::Failed;
        store_.save_task(task);
        return Result<RunResult>::err(load_result.error());
    }

    for (const auto& s : plan.subtasks) store_.save_subtask(s);

    auto exec = execute_workflow(task);
    RunResult result;
    result.task = task;
    result.events = event_log_.load(task.id).value_or(std::vector<domain::Event>{});
    result.artifacts = store_.load_artifacts(task.id).value_or(std::vector<domain::Artifact>{});
    result.metrics = metrics_;

    if (workflow_.is_complete() && exec.is_ok()) {
        task.status = domain::TaskStatus::Completed;
        metrics_.tasks_completed++;
        result.success = true;
        result.message = "Task completed successfully";
        event_log_.append(task.id, domain::EventType::TaskCompleted, {{"artifacts", result.artifacts.size()}});
    } else {
        task.status = domain::TaskStatus::Failed;
        metrics_.tasks_failed++;
        result.success = false;
        result.message = exec.is_err() ? exec.error() : "Workflow failed";
        event_log_.append(task.id, domain::EventType::TaskFailed, {{"reason", result.message}});
    }

    store_.save_task(task);
    result.task = task;
    return Result<RunResult>::ok(std::move(result));
}

Result<RunResult> Orchestrator::replay(const domain::TaskId& task_id) {
    auto events = event_log_.load(task_id);
    if (events.is_err()) return Result<RunResult>::err(events.error());

    RunResult result;
    result.events = events.value();
    result.task.id = task_id;
    result.artifacts = store_.load_artifacts(task_id).value_or(std::vector<domain::Artifact>{});
    result.success = true;
    result.message = "Replay loaded " + std::to_string(result.events.size()) + " events";
    return Result<RunResult>::ok(std::move(result));
}

Result<domain::Task> Orchestrator::status(const domain::TaskId& task_id) const {
    auto task = store_.load_task(task_id);
    if (task.is_err()) return Result<domain::Task>::err(task.error());
    if (!task.value().has_value()) return Result<domain::Task>::err("Task not found");
    return Result<domain::Task>::ok(task.value().value());
}

}  // namespace teamforge::orchestration
