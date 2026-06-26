#include "teamforge/tools/builtin_tools.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <regex>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef Message
#else
#include <unistd.h>
#endif

namespace teamforge::tools {

namespace {

ToolDescriptor make_desc(const char* name, const char* desc) {
    return {name, desc, {{"type", "object"}}};
}

bool is_command_allowed(const std::string& cmd) {
    static const std::array<const char*, 12> allowed = {
        "cmake", "--build", "ctest", "git", "dir", "ls", "echo", "type", "cat", "findstr", "rg",
        "where"};
    for (const auto* prefix : allowed) {
        if (cmd.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

std::string run_command(const std::string& cmd, int timeout_seconds) {
    (void)timeout_seconds;
    std::array<char, 4096> buffer{};
    std::string output;
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return "Failed to run command";
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

}  // namespace

ToolDescriptor FileReadTool::descriptor() const {
    return make_desc("file_read", "Read a file from the workspace");
}

Result<ToolOutput> FileReadTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto path = input.arguments.value("path", "");
    auto resolved = detail::resolve_path(ctx, path);
    if (resolved.is_err()) return Result<ToolOutput>::err(resolved.error());

    std::ifstream file(resolved.value());
    if (!file) return Result<ToolOutput>::err("Cannot read file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    ToolOutput out;
    out.success = true;
    out.result = {{"path", path}, {"content", ss.str()}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor FileWriteTool::descriptor() const {
    return make_desc("file_write", "Write content to a file in the workspace");
}

Result<ToolOutput> FileWriteTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto path = input.arguments.value("path", "");
    const auto content = input.arguments.value("content", "");
    auto resolved = detail::resolve_path(ctx, path);
    if (resolved.is_err()) return Result<ToolOutput>::err(resolved.error());

    std::filesystem::create_directories(resolved.value().parent_path());
    std::ofstream file(resolved.value());
    if (!file) return Result<ToolOutput>::err("Cannot write file: " + path);
    file << content;

    ToolOutput out;
    out.success = true;
    out.result = {{"path", path}, {"bytes_written", content.size()}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor FileListTool::descriptor() const {
    return make_desc("file_list", "List files in a workspace directory");
}

Result<ToolOutput> FileListTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto path = input.arguments.value("path", ".");
    auto resolved = detail::resolve_path(ctx, path);
    if (resolved.is_err()) return Result<ToolOutput>::err(resolved.error());

    nlohmann::json files = nlohmann::json::array();
    if (std::filesystem::exists(resolved.value())) {
        for (const auto& entry : std::filesystem::directory_iterator(resolved.value())) {
            files.push_back(entry.path().filename().string());
        }
    }

    ToolOutput out;
    out.success = true;
    out.result = {{"path", path}, {"files", files}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor CodeSearchTool::descriptor() const {
    return make_desc("code_search", "Search code in workspace using pattern");
}

Result<ToolOutput> CodeSearchTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto pattern = input.arguments.value("pattern", "");
    const auto subpath = input.arguments.value("path", ".");
    auto resolved = detail::resolve_path(ctx, subpath);
    if (resolved.is_err()) return Result<ToolOutput>::err(resolved.error());

    nlohmann::json matches = nlohmann::json::array();
    const std::regex re(pattern, std::regex::icase);

    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(resolved.value())) {
        if (!entry.is_regular_file()) continue;
        std::ifstream file(entry.path());
        std::string line;
        int line_no = 0;
        while (std::getline(file, line)) {
            ++line_no;
            if (std::regex_search(line, re)) {
                matches.push_back({{"file", entry.path().string()},
                                   {"line", line_no},
                                   {"text", line.substr(0, 200)}});
            }
        }
    }

    ToolOutput out;
    out.success = true;
    out.result = {{"pattern", pattern}, {"matches", matches}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor ShellExecuteTool::descriptor() const {
    return make_desc("shell_execute", "Run an allowlisted shell command in workspace");
}

Result<ToolOutput> ShellExecuteTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto command = input.arguments.value("command", "");
    if (!is_command_allowed(command)) {
        return Result<ToolOutput>::err("Command not in allowlist: " + command);
    }

    const auto cwd = ctx.workspace_root.string();
#ifdef _WIN32
    const std::string full = "cd /d \"" + cwd + "\" && " + command;
#else
    const std::string full = "cd \"" + cwd + "\" && " + command;
#endif

    ToolOutput out;
    out.success = true;
    out.result = {{"command", command}, {"stdout", run_command(full, ctx.timeout_seconds)}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor GitDiffTool::descriptor() const {
    return make_desc("git_diff", "Get git diff for workspace");
}

Result<ToolOutput> GitDiffTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    (void)input;
    const auto cwd = ctx.workspace_root.string();
#ifdef _WIN32
    const std::string cmd = "cd /d \"" + cwd + "\" && git diff";
#else
    const std::string cmd = "cd \"" + cwd + "\" && git diff";
#endif

    ToolOutput out;
    out.success = true;
    out.result = {{"diff", run_command(cmd, ctx.timeout_seconds)}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor TestExecuteTool::descriptor() const {
    return make_desc("test_execute", "Run tests via ctest or custom command");
}

Result<ToolOutput> TestExecuteTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    const auto command = input.arguments.value("command", "ctest --output-on-failure");
    if (!is_command_allowed(command)) {
        return Result<ToolOutput>::err("Test command not allowed: " + command);
    }

    const auto cwd = ctx.workspace_root.string();
#ifdef _WIN32
    const std::string full = "cd /d \"" + cwd + "\" && " + command;
#else
    const std::string full = "cd \"" + cwd + "\" && " + command;
#endif

    const auto output = run_command(full, ctx.timeout_seconds);
    ToolOutput out;
    out.success = output.find("FAILED") == std::string::npos &&
                  output.find("Failed") == std::string::npos;
    out.result = {{"command", command}, {"output", output}, {"passed", out.success}};
    return Result<ToolOutput>::ok(std::move(out));
}

ToolDescriptor JsonValidateTool::descriptor() const {
    return make_desc("json_validate", "Validate JSON against required keys");
}

Result<ToolOutput> JsonValidateTool::execute(const ToolInput& input, ExecutionContext& ctx) {
    (void)ctx;
    const auto data = input.arguments.at("data");
    const auto required = input.arguments.value("required_keys", std::vector<std::string>{});

    nlohmann::json missing = nlohmann::json::array();
    for (const auto& key : required) {
        if (!data.contains(key)) missing.push_back(key);
    }

    ToolOutput out;
    out.success = missing.empty();
    out.result = {{"valid", out.success}, {"missing_keys", missing}};
    return Result<ToolOutput>::ok(std::move(out));
}

void install_default_tools(ToolRegistry& tool_registry) {
    tool_registry.register_tool(std::make_shared<FileReadTool>());
    tool_registry.register_tool(std::make_shared<FileWriteTool>());
    tool_registry.register_tool(std::make_shared<FileListTool>());
    tool_registry.register_tool(std::make_shared<CodeSearchTool>());
    tool_registry.register_tool(std::make_shared<ShellExecuteTool>());
    tool_registry.register_tool(std::make_shared<GitDiffTool>());
    tool_registry.register_tool(std::make_shared<TestExecuteTool>());
    tool_registry.register_tool(std::make_shared<JsonValidateTool>());
}

}  // namespace teamforge::tools
