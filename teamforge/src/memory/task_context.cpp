#include "teamforge/memory/task_context.hpp"

namespace teamforge::memory {

void TaskContext::add_artifact(const domain::Artifact& artifact) {
    std::lock_guard lock(mutex_);
    artifacts_.push_back(artifact);
}

void TaskContext::add_decision(const std::string& key, const nlohmann::json& value) {
    std::lock_guard lock(mutex_);
    decisions_[key] = value;
}

void TaskContext::add_open_question(const std::string& question) {
    std::lock_guard lock(mutex_);
    open_questions_.push_back(question);
}

nlohmann::json TaskContext::snapshot() const {
    std::lock_guard lock(mutex_);
    nlohmann::json arts = nlohmann::json::array();
    for (const auto& a : artifacts_) arts.push_back(a.to_json());
    return {{"task_id", task_id_},
            {"artifacts", arts},
            {"decisions", decisions_},
            {"open_questions", open_questions_}};
}

std::vector<domain::Artifact> TaskContext::artifacts() const {
    std::lock_guard lock(mutex_);
    return artifacts_;
}

void MemoryManager::write_working(const std::string& key, nlohmann::json value) {
    std::lock_guard lock(mutex_);
    working_[key] = std::move(value);
}

nlohmann::json MemoryManager::read_working(const std::string& key) const {
    std::lock_guard lock(mutex_);
    return working_.value(key, nlohmann::json{});
}

void MemoryManager::clear_working() {
    std::lock_guard lock(mutex_);
    working_ = nlohmann::json::object();
}

void MemoryManager::write_task(const domain::TaskId& task_id, const std::string& key,
                               nlohmann::json value) {
    std::lock_guard lock(mutex_);
    task_memory_[task_id + ":" + key] = std::move(value);
}

nlohmann::json MemoryManager::read_task(const domain::TaskId& task_id,
                                        const std::string& key) const {
    std::lock_guard lock(mutex_);
    return task_memory_.count(task_id + ":" + key) ? task_memory_.at(task_id + ":" + key)
                                                   : nlohmann::json{};
}

}  // namespace teamforge::memory
