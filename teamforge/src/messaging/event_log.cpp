#include "teamforge/messaging/event_log.hpp"

namespace teamforge::messaging {

EventLog::EventLog(storage::SqliteStore& store) : store_(store) {}

Result<domain::Event> EventLog::append(domain::TaskId task_id, domain::EventType type,
                                       nlohmann::json details) {
    domain::Event event;
    event.id = domain::generate_id("evt");
    event.task_id = std::move(task_id);
    event.type = type;
    event.details = std::move(details);
    event.ts = std::chrono::system_clock::now();

    auto result = store_.append_event(event);
    if (result.is_err()) return Result<domain::Event>::err(result.error());

    for (const auto& sub : subscribers_) sub(event);
    return Result<domain::Event>::ok(event);
}

Result<std::vector<domain::Event>> EventLog::load(const domain::TaskId& task_id) const {
    return store_.load_events(task_id);
}

void EventLog::subscribe(std::function<void(const domain::Event&)> handler) {
    subscribers_.push_back(std::move(handler));
}

void MessageBus::publish(const domain::AgentMessage& message) {
    std::lock_guard lock(mutex_);
    messages_.push_back(message);
}

std::vector<domain::AgentMessage> MessageBus::drain(const domain::TaskId& task_id) {
    std::lock_guard lock(mutex_);
    std::vector<domain::AgentMessage> out;
    std::vector<domain::AgentMessage> remaining;
    for (auto& m : messages_) {
        if (m.task_id == task_id) out.push_back(std::move(m));
        else remaining.push_back(std::move(m));
    }
    messages_ = std::move(remaining);
    return out;
}

}  // namespace teamforge::messaging
