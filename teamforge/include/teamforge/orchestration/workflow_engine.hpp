#pragma once

#include "teamforge/domain/subtask.hpp"
#include "teamforge/domain/task.hpp"
#include "teamforge/core/result.hpp"

#include <unordered_map>
#include <vector>

namespace teamforge::orchestration {

struct WorkflowConfig {
    int max_retries{3};
    int max_revisions{5};
};

class WorkflowEngine {
public:
    explicit WorkflowEngine(WorkflowConfig config = {});

    Result<void> load_plan(const domain::TaskPlan& plan);
    Result<void> validate_dag() const;
    void refresh_ready_states();

    std::vector<domain::Subtask> ready_subtasks() const;
    std::vector<domain::Subtask> all_subtasks() const;

    Result<void> mark_running(const domain::SubtaskId& id);
    Result<void> mark_completed(const domain::SubtaskId& id, const domain::AgentOutput& output);
    Result<void> mark_failed(const domain::SubtaskId& id, const std::string& reason);
    Result<void> request_revision(const domain::SubtaskId& id);

    bool is_complete() const;
    bool has_failed() const;
    bool can_retry(const domain::SubtaskId& id) const;

    const domain::Subtask* get_subtask(const domain::SubtaskId& id) const;

private:
    WorkflowConfig config_;
    std::unordered_map<domain::SubtaskId, domain::Subtask> subtasks_;

    bool dependencies_met(const domain::Subtask& subtask) const;
    void update_ready_status(domain::Subtask& subtask);
};

class RevisionPolicy {
public:
    explicit RevisionPolicy(int max_revisions = 5);

    bool can_revise(const domain::Subtask& subtask) const;
    bool should_escalate(const domain::Subtask& subtask) const;

private:
    int max_revisions_;
};

}  // namespace teamforge::orchestration
