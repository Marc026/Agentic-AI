#include "teamforge/orchestration/orchestrator.hpp"
#include "teamforge/telemetry/logger.hpp"

#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace {

std::filesystem::path find_project_root() {
    auto path = std::filesystem::current_path();
    for (int i = 0; i < 5; ++i) {
        if (std::filesystem::exists(path / "agents") && std::filesystem::exists(path / "schemas")) {
            return path;
        }
        if (!path.has_parent_path()) break;
        path = path.parent_path();
    }
    return std::filesystem::current_path();
}

}  // namespace

int main(int argc, char** argv) {
    teamforge::telemetry::Logger::init("info");

    const auto root = find_project_root();

    teamforge::orchestration::OrchestratorConfig config;
    config.workspace = root / "data" / "workspace";
    config.data_dir = root / "data";
    config.agents_dir = root / "agents";
    config.schemas_dir = root / "schemas";
    config.use_mock_llm = true;

    CLI::App app{"TeamForge - Multi-Agent Software Engineering Platform"};
    app.require_subcommand(1);

    std::string request;
    bool use_http_llm = false;
    std::string llm_url = "http://localhost:11434";
    std::string llm_model = "llama3";

    auto* run_cmd = app.add_subcommand("run", "Run a new engineering task");
    run_cmd->add_option("request", request, "High-level task description")->required();
    run_cmd->add_flag("--http-llm", use_http_llm, "Use HTTP LLM instead of mock");
    run_cmd->add_option("--llm-url", llm_url, "LLM base URL");
    run_cmd->add_option("--llm-model", llm_model, "LLM model name");

    std::string task_id;
    auto* status_cmd = app.add_subcommand("status", "Show task status");
    status_cmd->add_option("task_id", task_id, "Task ID")->required();

    auto* replay_cmd = app.add_subcommand("replay", "Replay task event log");
    replay_cmd->add_option("task_id", task_id, "Task ID")->required();

    CLI11_PARSE(app, argc, argv);

    config.use_mock_llm = !use_http_llm;
    config.llm_base_url = llm_url;
    config.llm_model = llm_model;

    teamforge::orchestration::Orchestrator orchestrator(config);

    if (run_cmd->parsed()) {
        teamforge::telemetry::Logger::info("Starting task: " + request);
        auto result = orchestrator.run(request);
        if (result.is_err()) {
            std::cerr << "Error: " << result.error() << "\n";
            return 1;
        }
        const auto& r = result.value();
        std::cout << "Task ID: " << r.task.id << "\n";
        std::cout << "Status: " << teamforge::domain::to_string(r.task.status) << "\n";
        std::cout << "Message: " << r.message << "\n";
        std::cout << "Artifacts: " << r.artifacts.size() << "\n";
        std::cout << "Events: " << r.events.size() << "\n";
        std::cout << "Gate pass rate: " << r.metrics.gate_pass_rate() << "\n";
        return r.success ? 0 : 1;
    }

    if (status_cmd->parsed()) {
        auto result = orchestrator.status(task_id);
        if (result.is_err()) {
            std::cerr << "Error: " << result.error() << "\n";
            return 1;
        }
        const auto& t = result.value();
        std::cout << "Task ID: " << t.id << "\n";
        std::cout << "Request: " << t.user_request << "\n";
        std::cout << "Status: " << teamforge::domain::to_string(t.status) << "\n";
        return 0;
    }

    if (replay_cmd->parsed()) {
        auto result = orchestrator.replay(task_id);
        if (result.is_err()) {
            std::cerr << "Error: " << result.error() << "\n";
            return 1;
        }
        const auto& r = result.value();
        std::cout << r.message << "\n";
        for (const auto& e : r.events) {
            std::cout << "- [" << teamforge::domain::timestamp_to_iso(e.ts) << "] event "
                      << static_cast<int>(e.type) << "\n";
        }
        return 0;
    }

    return 0;
}
