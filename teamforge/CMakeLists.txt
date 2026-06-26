cmake_minimum_required(VERSION 3.20)
project(teamforge VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    sqlite3
    GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json spdlog cli11 catch2)

FetchContent_GetProperties(httplib)
if(NOT httplib_POPULATED)
    FetchContent_Populate(httplib)
    add_library(cpp-httplib INTERFACE)
    target_include_directories(cpp-httplib INTERFACE ${httplib_SOURCE_DIR})
endif()

FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
    set(BUILD_SHELL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${sqlite3_SOURCE_DIR} ${sqlite3_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(teamforge_lib
    src/domain/task.cpp
    src/domain/subtask.cpp
    src/domain/serde.cpp
    src/telemetry/logger.cpp
    src/storage/sqlite_store.cpp
    src/messaging/event_log.cpp
    src/memory/task_context.cpp
    src/orchestration/workflow_engine.cpp
    src/orchestration/orchestrator.cpp
    src/tools/tool_registry.cpp
    src/tools/builtin_tools.cpp
    src/llm/llm_providers.cpp
    src/validation/validator_hub.cpp
    src/agents/agent_runner.cpp
)

target_include_directories(teamforge_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(teamforge_lib PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite3
    cpp-httplib
)

target_include_directories(teamforge_lib PRIVATE ${sqlite3_SOURCE_DIR})

if(WIN32)
    target_link_libraries(teamforge_lib PUBLIC ws2_32)
endif()

add_executable(teamforge src/main.cpp)
target_link_libraries(teamforge PRIVATE teamforge_lib CLI11::CLI11)

enable_testing()

add_executable(teamforge_tests
    tests/unit/test_workflow_engine.cpp
    tests/unit/test_validator_hub.cpp
    tests/unit/test_tools.cpp
    tests/integration/test_orchestrator_mock.cpp
)

target_link_libraries(teamforge_tests PRIVATE teamforge_lib Catch2::Catch2WithMain)
include(Catch)
catch_discover_tests(teamforge_tests)
