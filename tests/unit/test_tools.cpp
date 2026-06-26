#include <catch2/catch_test_macros.hpp>

#include "teamforge/tools/builtin_tools.hpp"
#include "teamforge/tools/tool_registry.hpp"

#include <filesystem>

using namespace teamforge;
using namespace teamforge::tools;

TEST_CASE("File tools reject path traversal", "[tools]") {
    ToolRegistry registry;
    install_default_tools(registry);

    const auto tmp = std::filesystem::temp_directory_path() / "teamforge_test";
    std::filesystem::create_directories(tmp);

    ExecutionContext ctx{tmp, 10, domain::ToolCapabilitySet{{"file_read", "file_write"}}};

    ToolInput read;
    read.tool_name = "file_read";
    read.arguments = {{"path", "../outside.txt"}};
    const auto result = registry.execute(read, ctx);
    REQUIRE(result.is_err());
}

TEST_CASE("Shell tool rejects disallowed commands", "[tools]") {
    ToolRegistry registry;
    install_default_tools(registry);

    const auto tmp = std::filesystem::temp_directory_path() / "teamforge_test";
    ExecutionContext ctx{tmp, 10, domain::ToolCapabilitySet{{"shell_execute"}}};

    ToolInput cmd;
    cmd.tool_name = "shell_execute";
    cmd.arguments = {{"command", "rm -rf /"}};
    const auto result = registry.execute(cmd, ctx);
    REQUIRE(result.is_err());
}

TEST_CASE("File write and read round trip", "[tools]") {
    ToolRegistry registry;
    install_default_tools(registry);

    const auto tmp = std::filesystem::temp_directory_path() / "teamforge_test";
    std::filesystem::create_directories(tmp);

    ExecutionContext ctx{tmp, 10, domain::ToolCapabilitySet{{"file_read", "file_write"}}};

    ToolInput write;
    write.tool_name = "file_write";
    write.arguments = {{"path", "hello.txt"}, {"content", "teamforge"}};
    REQUIRE(registry.execute(write, ctx).is_ok());

    ToolInput read;
    read.tool_name = "file_read";
    read.arguments = {{"path", "hello.txt"}};
    const auto result = registry.execute(read, ctx);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().result.at("content") == "teamforge");
}
