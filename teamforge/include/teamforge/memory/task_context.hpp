#pragma once

#include "teamforge/domain/agent.hpp"
#include "teamforge/domain/artifact.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace teamforge::memory {

class TaskContext {
public:
    void set_task_id(domain::TaskId id) { task_id_ = std::move(id); }
    const domain::TaskId& task_id() const { return task_id_; }

    void add_artifact(const domain::Artifact& artifact);
    void add_decision(const std::string& key, const nlohmann::json& value);
    void add_open_question(const std::string& question);

    nlohmann::json snapshot() const;
    std::vector<domain::Artifact> artifacts() const;

private:
    domain::TaskId task_id_;
    mutable std::mutex mutex_;
    std::vector<domain::Artifact> artifacts_;
    nlohmann::json decisions_{nlohmann::json::object()};
    std::vector<std::string> open_questions_;
};

class MemoryManager {
public:
    void write_working(const std::string& key, nlohmann::json value);
    nlohmann::json read_working(const std::string& key) const;
    void clear_working();

    void write_task(const domain::TaskId& task_id, const std::string& key, nlohmann::json value);
    nlohmann::json read_task(const domain::TaskId& task_id, const std::string& key) const;

private:
    mutable std::mutex mutex_;
    nlohmann::json working_{nlohmann::json::object()};
    std::unordered_map<std::string, nlohmann::json> task_memory_;
};

}  // namespace teamforge::memory
