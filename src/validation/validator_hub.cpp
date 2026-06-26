#include "teamforge/validation/validator_hub.hpp"

#include <fstream>

namespace teamforge::validation {

ValidatorHub::ValidatorHub(std::filesystem::path schemas_dir)
    : schemas_dir_(std::move(schemas_dir)) {}

bool ValidatorHub::is_blocking_gate(domain::GateType gate) const {
    return gate == domain::GateType::Review || gate == domain::GateType::Test ||
           gate == domain::GateType::Security;
}

domain::ValidationResult ValidatorHub::validate_schema(const std::string& schema_name,
                                                       const nlohmann::json& data) const {
    if (schema_name == "design_doc") return validate_design_doc(data);
    if (schema_name == "review_report") return validate_review_report(data);
    if (schema_name == "test_report") return validate_test_report(data);
    if (schema_name == "implementation_bundle") return validate_implementation(data);
    if (schema_name == "task_plan") return validate_task_plan(data);

    domain::ValidationResult r;
    r.validator = schema_name;
    r.passed = true;
    r.score = 1.0;
    r.verdict = domain::Verdict::Approve;
    return r;
}

domain::ValidationResult ValidatorHub::validate_design_doc(const nlohmann::json& data) const {
    domain::ValidationResult r;
    r.validator = "design_doc";
    std::vector<std::string> required = {"title", "modules", "endpoints", "data_models"};
    for (const auto& key : required) {
        if (!data.contains(key)) {
            r.findings.push_back({"Error", "Missing required field: " + key, "design_doc"});
        }
    }
    r.passed = r.findings.empty();
    r.score = r.passed ? 0.9 : 0.3;
    r.verdict = r.passed ? domain::Verdict::Approve : domain::Verdict::RequestChanges;
    return r;
}

domain::ValidationResult ValidatorHub::validate_review_report(const nlohmann::json& data) const {
    domain::ValidationResult r;
    r.validator = "review_report";
    if (!data.contains("verdict")) {
        r.findings.push_back({"Error", "Missing verdict", "review_report"});
    }
    const auto verdict = data.value("verdict", "Block");
    r.score = data.value("score", 0.0);

    for (const auto& f : data.value("findings", nlohmann::json::array())) {
        if (f.value("severity", "") == "Critical") {
            r.findings.push_back({"Critical", f.value("message", ""), f.value("location", "")});
        }
    }

    if (!r.findings.empty()) {
        r.passed = false;
        r.verdict = domain::Verdict::Block;
    } else if (verdict == "Approve") {
        r.passed = true;
        r.verdict = domain::Verdict::Approve;
    } else {
        r.passed = false;
        r.verdict = domain::Verdict::RequestChanges;
    }
    return r;
}

domain::ValidationResult ValidatorHub::validate_test_report(const nlohmann::json& data) const {
    domain::ValidationResult r;
    r.validator = "test_report";
    const bool passed = data.value("passed", false);
    r.passed = passed;
    r.score = passed ? 0.95 : 0.1;
    r.verdict = passed ? domain::Verdict::Approve : domain::Verdict::Block;
    if (!passed) {
        r.findings.push_back({"Error", "Tests did not pass", "test_report"});
    }
    return r;
}

domain::ValidationResult ValidatorHub::validate_implementation(const nlohmann::json& data) const {
    domain::ValidationResult r;
    r.validator = "implementation_bundle";
    if (!data.contains("files") || !data.at("files").is_array() || data.at("files").empty()) {
        r.findings.push_back({"Error", "No files in implementation bundle", "implementation"});
        r.passed = false;
        r.verdict = domain::Verdict::RequestChanges;
        r.score = 0.2;
        return r;
    }
    r.passed = true;
    r.score = 0.85;
    r.verdict = domain::Verdict::Approve;
    return r;
}

domain::ValidationResult ValidatorHub::validate_task_plan(const nlohmann::json& data) const {
    domain::ValidationResult r;
    r.validator = "task_plan";
    if (!data.contains("subtasks") || !data.at("subtasks").is_array()) {
        r.findings.push_back({"Error", "Task plan missing subtasks array", "task_plan"});
        r.passed = false;
        r.verdict = domain::Verdict::Block;
        return r;
    }
    r.passed = true;
    r.score = 1.0;
    r.verdict = domain::Verdict::Approve;
    return r;
}

domain::ValidationResult ValidatorHub::validate_agent_output(domain::AgentRole role,
                                                           const domain::AgentOutput& output) const {
    switch (role) {
        case domain::AgentRole::Orchestrator:
            return validate_task_plan(output.structured_payload);
        case domain::AgentRole::Architect:
            return validate_design_doc(output.structured_payload);
        case domain::AgentRole::Backend:
            return validate_implementation(output.structured_payload);
        case domain::AgentRole::TestEngineer:
            return validate_test_report(output.structured_payload);
        case domain::AgentRole::CodeReviewer:
            return validate_review_report(output.structured_payload);
        default:
            domain::ValidationResult r;
            r.validator = domain::to_string(role);
            r.passed = output.status == domain::AgentStatus::Success;
            r.score = r.passed ? 1.0 : 0.0;
            r.verdict = r.passed ? domain::Verdict::Approve : domain::Verdict::Block;
            return r;
    }
}

}  // namespace teamforge::validation
