#include "teamforge/storage/sqlite_store.hpp"

#include <sqlite3.h>

#include <filesystem>

namespace teamforge::storage {

namespace {

constexpr const char* kSchema = R"(
CREATE TABLE IF NOT EXISTS tasks (
    id TEXT PRIMARY KEY,
    user_request TEXT NOT NULL,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    budget_json TEXT NOT NULL,
    criteria_json TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS subtasks (
    id TEXT PRIMARY KEY,
    parent TEXT NOT NULL,
    assigned_role TEXT NOT NULL,
    objective TEXT NOT NULL,
    status TEXT NOT NULL,
    depends_on_json TEXT NOT NULL,
    gate TEXT NOT NULL,
    revision_count INTEGER NOT NULL DEFAULT 0,
    retry_count INTEGER NOT NULL DEFAULT 0,
    last_output_json TEXT,
    FOREIGN KEY(parent) REFERENCES tasks(id)
);

CREATE TABLE IF NOT EXISTS events (
    id TEXT PRIMARY KEY,
    task_id TEXT NOT NULL,
    type INTEGER NOT NULL,
    details_json TEXT NOT NULL,
    ts TEXT NOT NULL,
    FOREIGN KEY(task_id) REFERENCES tasks(id)
);

CREATE TABLE IF NOT EXISTS artifacts (
    id TEXT PRIMARY KEY,
    task_id TEXT NOT NULL,
    kind INTEGER NOT NULL,
    path TEXT NOT NULL,
    checksum TEXT NOT NULL,
    metadata_json TEXT NOT NULL,
    FOREIGN KEY(task_id) REFERENCES tasks(id)
);
)";

}  // namespace

SqliteStore::SqliteStore(std::string db_path) : db_path_(std::move(db_path)) {}

SqliteStore::~SqliteStore() {
    if (db_) {
        sqlite3_close(static_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
}

Result<void> SqliteStore::exec_sql(const std::string& sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(static_cast<sqlite3*>(db_), sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "sqlite error";
        sqlite3_free(err);
        return Result<void>::err(msg);
    }
    return Result<void>::ok();
}

Result<void> SqliteStore::initialize() {
    std::lock_guard lock(mutex_);
    std::filesystem::create_directories(std::filesystem::path(db_path_).parent_path());

    sqlite3* db = nullptr;
    if (sqlite3_open(db_path_.c_str(), &db) != SQLITE_OK) {
        return Result<void>::err("Failed to open database");
    }
    db_ = db;
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    return exec_sql(kSchema);
}

Result<void> SqliteStore::save_task(const domain::Task& task) {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql =
        "INSERT OR REPLACE INTO tasks (id, user_request, status, created_at, budget_json, "
        "criteria_json) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<void>::err(sqlite3_errmsg(db));
    }

    const auto j = task.to_json();
    const std::string id = task.id;
    const std::string req = task.user_request;
    const std::string status = domain::to_string(task.status);
    const std::string created = domain::timestamp_to_iso(task.created_at);
    const std::string budget = j.at("budget").dump();
    const std::string criteria = j.at("criteria").dump();

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, req.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, created.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, budget.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, criteria.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return Result<void>::err(sqlite3_errmsg(db));
    return Result<void>::ok();
}

Result<std::optional<domain::Task>> SqliteStore::load_task(const domain::TaskId& id) const {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql = "SELECT user_request, status, created_at, budget_json, criteria_json FROM "
                      "tasks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::optional<domain::Task>>::err(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    domain::Task task;
    task.id = id;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        task.user_request = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const std::string status_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (status_str == "Running") task.status = domain::TaskStatus::Running;
        else if (status_str == "Completed") task.status = domain::TaskStatus::Completed;
        else if (status_str == "Failed") task.status = domain::TaskStatus::Failed;
        else if (status_str == "Blocked") task.status = domain::TaskStatus::Blocked;
        else if (status_str == "Cancelled") task.status = domain::TaskStatus::Cancelled;
        sqlite3_finalize(stmt);
        return Result<std::optional<domain::Task>>::ok(task);
    }
    sqlite3_finalize(stmt);
    return Result<std::optional<domain::Task>>::ok(std::nullopt);
}

Result<void> SqliteStore::save_subtask(const domain::Subtask& subtask) {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql =
        "INSERT OR REPLACE INTO subtasks (id, parent, assigned_role, objective, status, "
        "depends_on_json, gate, revision_count, retry_count, last_output_json) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<void>::err(sqlite3_errmsg(db));
    }

    const auto deps = nlohmann::json(subtask.depends_on).dump();
    sqlite3_bind_text(stmt, 1, subtask.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, subtask.parent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, domain::to_string(subtask.assigned_role).c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, subtask.objective.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, domain::to_string(subtask.status).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, deps.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, domain::to_string(subtask.gate).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, subtask.revision_count);
    sqlite3_bind_int(stmt, 9, subtask.retry_count);
    sqlite3_bind_null(stmt, 10);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return Result<void>::err(sqlite3_errmsg(db));
    return Result<void>::ok();
}

Result<std::vector<domain::Subtask>> SqliteStore::load_subtasks(const domain::TaskId& task_id) const {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql =
        "SELECT id, assigned_role, objective, status, depends_on_json, gate, revision_count, "
        "retry_count FROM subtasks WHERE parent = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<domain::Subtask>>::err(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, task_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<domain::Subtask> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Subtask s;
        s.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        s.parent = task_id;
        s.assigned_role =
            domain::agent_role_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        s.objective = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.revision_count = sqlite3_column_int(stmt, 6);
        s.retry_count = sqlite3_column_int(stmt, 7);
        const auto deps =
            nlohmann::json::parse(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        s.depends_on = deps.get<std::vector<domain::SubtaskId>>();
        result.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<domain::Subtask>>::ok(std::move(result));
}

Result<void> SqliteStore::append_event(const domain::Event& event) {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql =
        "INSERT INTO events (id, task_id, type, details_json, ts) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<void>::err(sqlite3_errmsg(db));
    }

    const std::string details = event.details.dump();
    const std::string ts = domain::timestamp_to_iso(event.ts);
    sqlite3_bind_text(stmt, 1, event.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, event.task_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(event.type));
    sqlite3_bind_text(stmt, 4, details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ts.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return Result<void>::err(sqlite3_errmsg(db));
    return Result<void>::ok();
}

Result<std::vector<domain::Event>> SqliteStore::load_events(const domain::TaskId& task_id) const {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql = "SELECT id, type, details_json, ts FROM events WHERE task_id = ? ORDER BY ts;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<domain::Event>>::err(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, task_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<domain::Event> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Event e;
        e.task_id = task_id;
        e.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        e.type = static_cast<domain::EventType>(sqlite3_column_int(stmt, 1));
        e.details = nlohmann::json::parse(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<domain::Event>>::ok(std::move(result));
}

Result<void> SqliteStore::save_artifact(const domain::Artifact& artifact) {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql =
        "INSERT OR REPLACE INTO artifacts (id, task_id, kind, path, checksum, metadata_json) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<void>::err(sqlite3_errmsg(db));
    }

    const std::string meta = artifact.metadata.dump();
    sqlite3_bind_text(stmt, 1, artifact.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artifact.task_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(artifact.kind));
    sqlite3_bind_text(stmt, 4, artifact.path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, artifact.checksum.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, meta.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return Result<void>::err(sqlite3_errmsg(db));
    return Result<void>::ok();
}

Result<std::vector<domain::Artifact>> SqliteStore::load_artifacts(const domain::TaskId& task_id) const {
    std::lock_guard lock(mutex_);
    sqlite3* db = static_cast<sqlite3*>(db_);
    const char* sql = "SELECT id, kind, path, checksum, metadata_json FROM artifacts WHERE task_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<domain::Artifact>>::err(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, task_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<domain::Artifact> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Artifact a;
        a.task_id = task_id;
        a.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        a.kind = static_cast<domain::ArtifactKind>(sqlite3_column_int(stmt, 1));
        a.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        a.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        a.metadata = nlohmann::json::parse(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        result.push_back(std::move(a));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<domain::Artifact>>::ok(std::move(result));
}

}  // namespace teamforge::storage
