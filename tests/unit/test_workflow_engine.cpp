#include <catch2/catch_test_macros.hpp>

#include "teamforge/orchestration/workflow_engine.hpp"

using namespace teamforge;
using namespace teamforge::domain;
using namespace teamforge::orchestration;

TEST_CASE("WorkflowEngine schedules dependencies correctly", "[workflow]") {
    WorkflowEngine engine;

    TaskPlan plan;
    Subtask a;
    a.id = "a";
    a.parent = "task-1";
    a.assigned_role = AgentRole::Architect;
    a.objective = "Design";

    Subtask b;
    b.id = "b";
    b.parent = "task-1";
    b.assigned_role = AgentRole::Backend;
    b.objective = "Implement";
    b.depends_on = {"a"};

    plan.subtasks = {a, b};
    REQUIRE(engine.load_plan(plan).is_ok());

    engine.refresh_ready_states();
    auto ready = engine.ready_subtasks();
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].id == "a");

    AgentOutput output;
    output.status = AgentStatus::Success;
    REQUIRE(engine.mark_completed("a", output).is_ok());

    engine.refresh_ready_states();
    ready = engine.ready_subtasks();
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].id == "b");
}

TEST_CASE("WorkflowEngine detects cycles", "[workflow]") {
    WorkflowEngine engine;
    TaskPlan plan;

    Subtask a;
    a.id = "a";
    a.parent = "task-1";
    a.depends_on = {"b"};

    Subtask b;
    b.id = "b";
    b.parent = "task-1";
    b.depends_on = {"a"};

    plan.subtasks = {a, b};
    REQUIRE(engine.load_plan(plan).is_err());
}

TEST_CASE("WorkflowEngine enforces revision cap", "[workflow]") {
    WorkflowEngine engine({.max_retries = 3, .max_revisions = 2});

    TaskPlan plan;
    Subtask a;
    a.id = "a";
    a.parent = "task-1";
    plan.subtasks = {a};
    REQUIRE(engine.load_plan(plan).is_ok());

    REQUIRE(engine.request_revision("a").is_ok());
    REQUIRE(engine.request_revision("a").is_ok());
    REQUIRE(engine.request_revision("a").is_err());
}

TEST_CASE("Gate nodes block on non-success output", "[workflow]") {
    WorkflowEngine engine;
    TaskPlan plan;

    Subtask gate;
    gate.id = "gate";
    gate.parent = "task-1";
    gate.gate = GateType::Review;

    plan.subtasks = {gate};
    REQUIRE(engine.load_plan(plan).is_ok());
    engine.refresh_ready_states();

    AgentOutput blocked;
    blocked.status = AgentStatus::NeedsRevision;
    REQUIRE(engine.mark_completed("gate", blocked).is_ok());
    REQUIRE_FALSE(engine.is_complete());
}
