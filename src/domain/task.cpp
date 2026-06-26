#include "teamforge/domain/task.hpp"

namespace teamforge::domain {

nlohmann::json Task::to_json() const {
    return {
        {"id", id},
        {"user_request", user_request},
        {"status", to_string(status)},
        {"created_at", timestamp_to_iso(created_at)},
        {"budget",
         {{"max_subtasks", budget.max_subtasks},
          {"max_revisions", budget.max_revisions},
          {"max_wall_time_seconds", budget.max_wall_time_seconds}}},
        {"criteria",
         {{"required_artifact_kinds", criteria.required_artifact_kinds},
          {"success_conditions", criteria.success_conditions}}}};
}

Task Task::from_json(const nlohmann::json& j) {
    Task t;
    t.id = j.at("id").get<std::string>();
    t.user_request = j.at("user_request").get<std::string>();
    t.budget.max_subtasks = j.at("budget").value("max_subtasks", 20);
    t.budget.max_revisions = j.at("budget").value("max_revisions", 5);
    t.budget.max_wall_time_seconds = j.at("budget").value("max_wall_time_seconds", 3600);
    if (j.contains("criteria")) {
        t.criteria.required_artifact_kinds =
            j.at("criteria").value("required_artifact_kinds", std::vector<std::string>{});
        t.criteria.success_conditions =
            j.at("criteria").value("success_conditions", std::vector<std::string>{});
    }
    return t;
}

}  // namespace teamforge::domain
