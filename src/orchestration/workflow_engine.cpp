#include "teamforge/orchestration/workflow_engine.hpp"

#include <algorithm>
#include <functional>
#include <set>

namespace teamforge::orchestration {

WorkflowEngine::WorkflowEngine(WorkflowConfig config) : config_(config) {}

Result<void> WorkflowEngine::load_plan(const domain::TaskPlan& plan) {
    subtasks_.clear();
    for (const auto& s : plan.subtasks) {
        subtasks_[s.id] = s;
    }
    return validate_dag();
}

Result<void> WorkflowEngine::validate_dag() const {
    std::set<domain::SubtaskId> visiting;
    std::set<domain::SubtaskId> visited;

    std::function<bool(const domain::SubtaskId&)> dfs = [&](const domain::SubtaskId& id) -> bool {
        if (visited.count(id)) return true;
        if (visiting.count(id)) return false;
        visiting.insert(id);
        const auto it = subtasks_.find(id);
        if (it == subtasks_.end()) return false;
        for (const auto& dep : it->second.depends_on) {
            if (!dfs(dep)) return false;
        }
        visiting.erase(id);
        visited.insert(id);
        return true;
    };

    for (const auto& [id, _] : subtasks_) {
        if (!dfs(id)) {
            return Result<void>::err("Cycle detected in task graph at subtask: " + id);
        }
    }
    return Result<void>::ok();
}

void WorkflowEngine::refresh_ready_states() {
    for (auto& [_, subtask] : subtasks_) {
        update_ready_status(subtask);
    }
}

bool WorkflowEngine::dependencies_met(const domain::Subtask& subtask) const {
    for (const auto& dep : subtask.depends_on) {
        const auto it = subtasks_.find(dep);
        if (it == subtasks_.end()) return false;
        if (it->second.status != domain::SubtaskStatus::Completed) return false;
    }
    return true;
}

void WorkflowEngine::update_ready_status(domain::Subtask& subtask) {
    if (subtask.status == domain::SubtaskStatus::Completed ||
        subtask.status == domain::SubtaskStatus::Failed) {
        return;
    }
    if (dependencies_met(subtask)) {
        if (subtask.status == domain::SubtaskStatus::Pending) {
            subtask.status = domain::SubtaskStatus::Ready;
        }
    }
}

std::vector<domain::Subtask> WorkflowEngine::ready_subtasks() const {
    std::vector<domain::Subtask> ready;
    for (auto& [id, subtask] : subtasks_) {
        auto copy = subtask;
        if ((copy.status == domain::SubtaskStatus::Ready ||
             copy.status == domain::SubtaskStatus::Pending) &&
            dependencies_met(copy)) {
            if (copy.status == domain::SubtaskStatus::Pending) {
                copy.status = domain::SubtaskStatus::Ready;
            }
            ready.push_back(copy);
        }
    }
    return ready;
}

std::vector<domain::Subtask> WorkflowEngine::all_subtasks() const {
    std::vector<domain::Subtask> all;
    for (const auto& [_, s] : subtasks_) all.push_back(s);
    return all;
}

Result<void> WorkflowEngine::mark_running(const domain::SubtaskId& id) {
    auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return Result<void>::err("Subtask not found: " + id);
    it->second.status = domain::SubtaskStatus::Running;
    return Result<void>::ok();
}

Result<void> WorkflowEngine::mark_completed(const domain::SubtaskId& id,
                                            const domain::AgentOutput& output) {
    auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return Result<void>::err("Subtask not found: " + id);

    if (it->second.gate != domain::GateType::None &&
        output.status != domain::AgentStatus::Success) {
        it->second.status = domain::SubtaskStatus::Blocked;
        it->second.last_output = output;
        return Result<void>::ok();
    }

    it->second.status = domain::SubtaskStatus::Completed;
    it->second.last_output = output;

    for (auto& [_, subtask] : subtasks_) {
        update_ready_status(subtask);
    }
    return Result<void>::ok();
}

Result<void> WorkflowEngine::mark_failed(const domain::SubtaskId& id, const std::string& reason) {
    auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return Result<void>::err("Subtask not found: " + id);
    it->second.retry_count++;
    if (can_retry(id)) {
        it->second.status = domain::SubtaskStatus::Ready;
    } else {
        it->second.status = domain::SubtaskStatus::Failed;
    }
    return Result<void>::ok();
}

Result<void> WorkflowEngine::request_revision(const domain::SubtaskId& id) {
    auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return Result<void>::err("Subtask not found: " + id);
    if (it->second.revision_count >= config_.max_revisions) {
        it->second.status = domain::SubtaskStatus::Failed;
        return Result<void>::err("Max revisions exceeded for subtask: " + id);
    }
    it->second.revision_count++;
    it->second.status = domain::SubtaskStatus::Ready;
    return Result<void>::ok();
}

bool WorkflowEngine::is_complete() const {
    if (subtasks_.empty()) return false;
    return std::all_of(subtasks_.begin(), subtasks_.end(), [](const auto& pair) {
        return pair.second.status == domain::SubtaskStatus::Completed;
    });
}

bool WorkflowEngine::has_failed() const {
    return std::any_of(subtasks_.begin(), subtasks_.end(), [](const auto& pair) {
        return pair.second.status == domain::SubtaskStatus::Failed;
    });
}

bool WorkflowEngine::can_retry(const domain::SubtaskId& id) const {
    const auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return false;
    return it->second.retry_count < config_.max_retries;
}

const domain::Subtask* WorkflowEngine::get_subtask(const domain::SubtaskId& id) const {
    const auto it = subtasks_.find(id);
    if (it == subtasks_.end()) return nullptr;
    return &it->second;
}

RevisionPolicy::RevisionPolicy(int max_revisions) : max_revisions_(max_revisions) {}

bool RevisionPolicy::can_revise(const domain::Subtask& subtask) const {
    return subtask.revision_count < max_revisions_;
}

bool RevisionPolicy::should_escalate(const domain::Subtask& subtask) const {
    return subtask.revision_count >= max_revisions_;
}

}  // namespace teamforge::orchestration
