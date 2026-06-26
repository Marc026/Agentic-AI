#include <catch2/catch_test_macros.hpp>

#include "teamforge/validation/validator_hub.hpp"

using namespace teamforge;
using namespace teamforge::domain;
using namespace teamforge::validation;

TEST_CASE("ValidatorHub approves valid design doc", "[validation]") {
    ValidatorHub hub("schemas");
    nlohmann::json doc = {{"title", "API"},
                          {"modules", {"a"}},
                          {"endpoints", nlohmann::json::array({{{"method", "GET"}, {"path", "/"}}})},
                          {"data_models", nlohmann::json::array()}};

    AgentOutput output;
    output.status = AgentStatus::Success;
    output.structured_payload = doc;

    const auto result = hub.validate_agent_output(AgentRole::Architect, output);
    REQUIRE(result.passed);
    REQUIRE(result.verdict == Verdict::Approve);
}

TEST_CASE("ValidatorHub blocks failed tests", "[validation]") {
    ValidatorHub hub("schemas");
    nlohmann::json report = {{"passed", false}, {"coverage_estimate", 0.1}};

    AgentOutput output;
    output.status = AgentStatus::Blocked;
    output.structured_payload = report;

    const auto result = hub.validate_agent_output(AgentRole::TestEngineer, output);
    REQUIRE_FALSE(result.passed);
    REQUIRE(result.verdict == Verdict::Block);
}

TEST_CASE("Reviewer critical findings block approval", "[validation]") {
    ValidatorHub hub("schemas");
    nlohmann::json report = {
        {"verdict", "Approve"},
        {"score", 0.9},
        {"findings", {{{"severity", "Critical"}, {"message", "SQL injection"}, {"location", "api"}}}}};

    AgentOutput output;
    output.status = AgentStatus::Success;
    output.structured_payload = report;

    const auto result = hub.validate_agent_output(AgentRole::CodeReviewer, output);
    REQUIRE_FALSE(result.passed);
    REQUIRE(result.verdict == Verdict::Block);
}
