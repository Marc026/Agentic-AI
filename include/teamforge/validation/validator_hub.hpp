#pragma once

#include "teamforge/domain/agent.hpp"
#include "teamforge/core/result.hpp"

#include <filesystem>
#include <string>

namespace teamforge::validation {

class ValidatorHub {
public:
    explicit ValidatorHub(std::filesystem::path schemas_dir);

    domain::ValidationResult validate_agent_output(domain::AgentRole role,
                                                   const domain::AgentOutput& output) const;
    domain::ValidationResult validate_schema(const std::string& schema_name,
                                             const nlohmann::json& data) const;
    bool is_blocking_gate(domain::GateType gate) const;

private:
    std::filesystem::path schemas_dir_;

    domain::ValidationResult validate_design_doc(const nlohmann::json& data) const;
    domain::ValidationResult validate_review_report(const nlohmann::json& data) const;
    domain::ValidationResult validate_test_report(const nlohmann::json& data) const;
    domain::ValidationResult validate_implementation(const nlohmann::json& data) const;
    domain::ValidationResult validate_task_plan(const nlohmann::json& data) const;
};

}  // namespace teamforge::validation
