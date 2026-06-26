#pragma once

#include <string>
#include <string_view>

namespace teamforge::domain {

enum class TaskStatus {
    Created,
    Running,
    Blocked,
    Completed,
    Failed,
    Cancelled
};

enum class SubtaskStatus {
    Pending,
    Ready,
    Assigned,
    Running,
    Review,
    Blocked,
    Completed,
    Failed,
    Partial
};

enum class AgentRole {
    Orchestrator,
    Architect,
    Backend,
    Frontend,
    TestEngineer,
    CodeReviewer,
    Debugger,
    Documentation,
    Security
};

enum class AgentStatus {
    Success,
    NeedsRevision,
    Blocked,
    Failed
};

enum class GateType { None, Review, Test, Security };

enum class MessageKind { Request, Response, Event, Revision, Block };

enum class ArtifactKind {
    DesignDoc,
    SourceFile,
    TestReport,
    ReviewReport,
    ImplementationBundle,
    TaskPlan,
    Documentation,
    SecurityReport,
    Other
};

enum class MemoryTier { Working, Task, LongTerm, Artifact };

enum class Verdict { Approve, RequestChanges, Block };

enum class EventType {
    TaskCreated,
    TaskCompleted,
    TaskFailed,
    SubtaskStarted,
    SubtaskCompleted,
    SubtaskFailed,
    ToolInvoked,
    ValidationPassed,
    ValidationFailed,
    RevisionRequested,
    AgentOutputReceived,
    PlanCreated
};

inline std::string to_string(TaskStatus s) {
    switch (s) {
        case TaskStatus::Created: return "Created";
        case TaskStatus::Running: return "Running";
        case TaskStatus::Blocked: return "Blocked";
        case TaskStatus::Completed: return "Completed";
        case TaskStatus::Failed: return "Failed";
        case TaskStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

inline std::string to_string(AgentRole r) {
    switch (r) {
        case AgentRole::Orchestrator: return "Orchestrator";
        case AgentRole::Architect: return "Architect";
        case AgentRole::Backend: return "Backend";
        case AgentRole::Frontend: return "Frontend";
        case AgentRole::TestEngineer: return "TestEngineer";
        case AgentRole::CodeReviewer: return "CodeReviewer";
        case AgentRole::Debugger: return "Debugger";
        case AgentRole::Documentation: return "Documentation";
        case AgentRole::Security: return "Security";
    }
    return "Unknown";
}

inline std::string to_string(GateType g) {
    switch (g) {
        case GateType::None: return "None";
        case GateType::Review: return "Review";
        case GateType::Test: return "Test";
        case GateType::Security: return "Security";
    }
    return "Unknown";
}

inline std::string to_string(SubtaskStatus s) {
    switch (s) {
        case SubtaskStatus::Pending: return "Pending";
        case SubtaskStatus::Ready: return "Ready";
        case SubtaskStatus::Assigned: return "Assigned";
        case SubtaskStatus::Running: return "Running";
        case SubtaskStatus::Review: return "Review";
        case SubtaskStatus::Blocked: return "Blocked";
        case SubtaskStatus::Completed: return "Completed";
        case SubtaskStatus::Failed: return "Failed";
        case SubtaskStatus::Partial: return "Partial";
    }
    return "Unknown";
}

inline AgentRole agent_role_from_string(std::string_view s) {
    if (s == "Orchestrator") return AgentRole::Orchestrator;
    if (s == "Architect") return AgentRole::Architect;
    if (s == "Backend") return AgentRole::Backend;
    if (s == "Frontend") return AgentRole::Frontend;
    if (s == "TestEngineer") return AgentRole::TestEngineer;
    if (s == "CodeReviewer") return AgentRole::CodeReviewer;
    if (s == "Debugger") return AgentRole::Debugger;
    if (s == "Documentation") return AgentRole::Documentation;
    if (s == "Security") return AgentRole::Security;
    return AgentRole::Orchestrator;
}

}  // namespace teamforge::domain
