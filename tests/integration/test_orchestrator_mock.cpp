#include <catch2/catch_test_macros.hpp>

#include "teamforge/orchestration/orchestrator.hpp"

#include <filesystem>

using namespace teamforge;
using namespace teamforge::orchestration;

TEST_CASE("Orchestrator completes URL shortener demo with mock LLM", "[integration]") {
    const auto root = std::filesystem::current_path();

    OrchestratorConfig config;
    config.workspace = root / "data" / "test_workspace";
    config.data_dir = root / "data" / "test";
    config.agents_dir = root / "agents";
    config.schemas_dir = root / "schemas";
    config.use_mock_llm = true;

    Orchestrator orchestrator(config);
    auto result = orchestrator.run("Build a URL shortener REST API");
    REQUIRE(result.is_ok());

    const auto& r = result.value();
    REQUIRE(r.success);
    REQUIRE(r.task.status == domain::TaskStatus::Completed);
    REQUIRE(r.events.size() > 5);
    REQUIRE(r.artifacts.size() >= 1);
}
