#pragma once

#include "teamforge/tools/itool.hpp"
#include "teamforge/tools/tool_registry.hpp"

#include <fstream>
#include <sstream>

namespace teamforge::tools {

namespace detail {

inline Result<std::filesystem::path> resolve_path(const ExecutionContext& ctx,
                                                  const std::string& relative) {
    const auto resolved = (ctx.workspace_root / relative).lexically_normal();
    const auto root = ctx.workspace_root.lexically_normal();
    const auto root_str = root.string();
    const auto resolved_str = resolved.string();
    if (resolved_str.rfind(root_str, 0) != 0) {
        return Result<std::filesystem::path>::err("Path traversal rejected: " + relative);
    }
    return Result<std::filesystem::path>::ok(resolved);
}

}  // namespace detail

class FileReadTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class FileWriteTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class FileListTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class CodeSearchTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class ShellExecuteTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class GitDiffTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class TestExecuteTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

class JsonValidateTool : public ITool {
public:
    ToolDescriptor descriptor() const override;
    Result<ToolOutput> execute(const ToolInput& input, ExecutionContext& ctx) override;
};

void install_default_tools(ToolRegistry& tool_registry);

}  // namespace teamforge::tools
