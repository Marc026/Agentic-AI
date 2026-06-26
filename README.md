# TeamForge

Multi-Agent Software Engineering Orchestration Platform (C++20).

TeamForge coordinates specialized AI agents through explicit contracts, a typed task graph, durable SQLite state, review gates, and a plugin tool framework.

## Features

- Orchestrator with DAG-based workflow engine
- Agents: Orchestrator, Architect, Backend, Test Engineer, Code Reviewer
- Blocking review and test gates
- Plugin tools: file I/O, code search, sandboxed shell, git diff, test execution
- SQLite persistence with append-only event log
- Mock LLM mode for CI and demos
- HTTP LLM provider (OpenAI-compatible / Ollama)

## Build

Requires CMake 3.20+ and a C++20 compiler (Visual Studio 2022 or GCC 11+).

Dependencies are fetched automatically via CMake FetchContent (no vcpkg required).

```powershell
cd teamforge
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
& $cmake --build build --config Release
cd build
.\Release\teamforge_tests.exe
```

## Usage

```powershell
# Run URL shortener demo (mock LLM)
./build/teamforge run "Build a URL shortener REST API"

# Check task status
./build/teamforge status task-...

# Replay event log
./build/teamforge replay task-...

# Use local Ollama
./build/teamforge run "Build a URL shortener REST API" --http-llm --llm-url http://localhost:11434
```

## Architecture

See [docs/architecture.md](docs/architecture.md).

## License

MIT
