#pragma once

#include "teamforge/domain/event.hpp"
#include "teamforge/domain/message.hpp"
#include "teamforge/storage/sqlite_store.hpp"
#include "teamforge/core/result.hpp"

#include <functional>
#include <mutex>
#include <vector>

namespace teamforge::messaging {

class EventLog {
public:
    explicit EventLog(storage::SqliteStore& store);

    Result<domain::Event> append(domain::TaskId task_id, domain::EventType type,
                                 nlohmann::json details);
    Result<std::vector<domain::Event>> load(const domain::TaskId& task_id) const;
    void subscribe(std::function<void(const domain::Event&)> handler);

private:
    storage::SqliteStore& store_;
    std::vector<std::function<void(const domain::Event&)>> subscribers_;
};

class MessageBus {
public:
    void publish(const domain::AgentMessage& message);
    std::vector<domain::AgentMessage> drain(const domain::TaskId& task_id);

private:
    std::mutex mutex_;
    std::vector<domain::AgentMessage> messages_;
};

}  // namespace teamforge::messaging
