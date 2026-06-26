#include "teamforge/domain/message.hpp"
#include "teamforge/domain/artifact.hpp"
#include "teamforge/domain/event.hpp"
#include "teamforge/domain/agent.hpp"

namespace teamforge::domain {

nlohmann::json AgentMessage::to_json() const {
    return {{"id", id},
            {"task_id", task_id},
            {"from", to_string(from)},
            {"to", to_string(to)},
            {"kind", static_cast<int>(kind)},
            {"payload", payload},
            {"ts", timestamp_to_iso(ts)}};
}

AgentMessage AgentMessage::from_json(const nlohmann::json& j) {
    AgentMessage m;
    m.id = j.at("id").get<std::string>();
    m.task_id = j.at("task_id").get<std::string>();
    m.from = agent_role_from_string(j.at("from").get<std::string>());
    m.to = agent_role_from_string(j.at("to").get<std::string>());
    m.payload = j.value("payload", nlohmann::json::object());
    return m;
}

nlohmann::json Artifact::to_json() const {
    return {{"id", id},
            {"task_id", task_id},
            {"kind", static_cast<int>(kind)},
            {"path", path},
            {"checksum", checksum},
            {"metadata", metadata}};
}

Artifact Artifact::from_json(const nlohmann::json& j) {
    Artifact a;
    a.id = j.at("id").get<std::string>();
    a.task_id = j.at("task_id").get<std::string>();
    a.path = j.at("path").get<std::string>();
    a.checksum = j.value("checksum", "");
    a.metadata = j.value("metadata", nlohmann::json::object());
    return a;
}

nlohmann::json Event::to_json() const {
    return {{"id", id},
            {"task_id", task_id},
            {"type", static_cast<int>(type)},
            {"details", details},
            {"ts", timestamp_to_iso(ts)}};
}

Event Event::from_json(const nlohmann::json& j) {
    Event e;
    e.id = j.at("id").get<std::string>();
    e.task_id = j.at("task_id").get<std::string>();
    e.type = static_cast<EventType>(j.at("type").get<int>());
    e.details = j.value("details", nlohmann::json::object());
    return e;
}

nlohmann::json ValidationResult::to_json() const {
    nlohmann::json findings_json = nlohmann::json::array();
    for (const auto& f : findings) {
        findings_json.push_back(
            {{"severity", f.severity}, {"message", f.message}, {"location", f.location}});
    }
    return {{"validator", validator},
            {"passed", passed},
            {"score", score},
            {"findings", findings_json},
            {"verdict", static_cast<int>(verdict)}};
}

}  // namespace teamforge::domain

