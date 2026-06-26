# Agent Contracts

Each agent returns structured JSON validated by `ValidatorHub`.

| Agent | Schema | Required Fields |
|-------|--------|-----------------|
| Orchestrator | task_plan | subtasks[] |
| Architect | design_doc | title, modules, endpoints, data_models |
| Backend | implementation_bundle | files[] |
| TestEngineer | test_report | test_files[], passed |
| CodeReviewer | review_report | verdict, score, findings[] |

## Verdicts

- **Approve**: output propagates
- **RequestChanges**: orchestrator may request revision (up to budget)
- **Block**: gate failure; downstream work blocked

## Tool ACLs

See `agents/*.json` and `default_tools_for_role()` in `agent_runner.cpp`.
