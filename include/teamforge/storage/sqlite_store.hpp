#pragma once

#include "teamforge/domain/event.hpp"
#include "teamforge/domain/task.hpp"
#include "teamforge/domain/subtask.hpp"
#include "teamforge/domain/artifact.hpp"
#include "teamforge/core/result.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace teamforge::storage {

class SqliteStore {
public:
    explicit SqliteStore(std::string db_path);
    ~SqliteStore();

    SqliteStore(const SqliteStore&) = delete;
    SqliteStore& operator=(const SqliteStore&) = delete;

    Result<void> initialize();
    Result<void> save_task(const domain::Task& task);
    Result<std::optional<domain::Task>> load_task(const domain::TaskId& id) const;
    Result<void> save_subtask(const domain::Subtask& subtask);
    Result<std::vector<domain::Subtask>> load_subtasks(const domain::TaskId& task_id) const;
    Result<void> append_event(const domain::Event& event);
    Result<std::vector<domain::Event>> load_events(const domain::TaskId& task_id) const;
    Result<void> save_artifact(const domain::Artifact& artifact);
    Result<std::vector<domain::Artifact>> load_artifacts(const domain::TaskId& task_id) const;

private:
    std::string db_path_;
    void* db_{nullptr};  // sqlite3*
    mutable std::mutex mutex_;

    Result<void> exec_sql(const std::string& sql);
};

}  // namespace teamforge::storage
