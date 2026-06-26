#include "teamforge/domain/subtask.hpp"

namespace teamforge::domain {

nlohmann::json Subtask::to_json() const {
    return {{"id", id},
            {"parent", parent},
            {"assigned_role", to_string(assigned_role)},
            {"objective", objective},
            {"status", to_string(status)},
            {"depends_on", depends_on},
            {"gate", to_string(gate)},
            {"revision_count", revision_count},
            {"retry_count", retry_count}};
}

Subtask Subtask::from_json(const nlohmann::json& j) {
    Subtask s;
    s.id = j.at("id").get<std::string>();
    s.parent = j.at("parent").get<std::string>();
    s.assigned_role = agent_role_from_string(j.at("assigned_role").get<std::string>());
    s.objective = j.at("objective").get<std::string>();
    if (j.contains("depends_on")) {
        s.depends_on = j.at("depends_on").get<std::vector<SubtaskId>>();
    }
    if (j.contains("gate")) {
        const auto g = j.at("gate").get<std::string>();
        if (g == "Review") s.gate = GateType::Review;
        else if (g == "Test") s.gate = GateType::Test;
        else if (g == "Security") s.gate = GateType::Security;
    }
    return s;
}

nlohmann::json TaskPlan::to_json() const {
    nlohmann::json subs = nlohmann::json::array();
    for (const auto& s : subtasks) subs.push_back(s.to_json());
    return {{"subtasks", subs}, {"metadata", metadata}};
}

TaskPlan TaskPlan::from_json(const nlohmann::json& j) {
    TaskPlan plan;
    for (const auto& s : j.at("subtasks")) {
        plan.subtasks.push_back(Subtask::from_json(s));
    }
    plan.metadata = j.value("metadata", nlohmann::json::object());
    return plan;
}

}  // namespace teamforge::domain
