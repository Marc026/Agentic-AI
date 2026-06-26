# TeamForge Architecture

TeamForge uses clean architecture with an orchestrator-centric hub pattern.

## Layers

- **Presentation**: CLI (`teamforge run`, `status`, `replay`)
- **Application**: Orchestrator, WorkflowEngine, AgentRunner, ValidatorHub
- **Domain**: Task, Subtask, Message, Artifact, Event models
- **Infrastructure**: SQLite, HTTP LLM, tools, telemetry

## Control Flow

1. User submits a request via CLI
2. Orchestrator creates a Task and TaskPlan (DAG of subtasks)
3. WorkflowEngine schedules ready subtasks by dependency order
4. AgentRunner invokes role-specific agents via LLM with bounded tools
5. ValidatorHub validates structured JSON outputs
6. Gate nodes (Review, Test) block downstream work on failure
7. Events and artifacts persist to SQLite and workspace

## Concurrency

- Orchestrator owns workflow state (single writer)
- Agent LLM calls are synchronous in MVP (thread pool ready for v1)
- SQLite WAL mode with mutex-protected store
