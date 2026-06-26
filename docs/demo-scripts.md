# Demo Scripts

## URL Shortener (Mock LLM)

```powershell
cd teamforge
./build/Release/teamforge.exe run "Build a URL shortener REST API"
```

Expected output:
- Task status: Completed
- Artifacts: design doc + source files
- Events: 20+ audit log entries

## Replay

```powershell
./build/Release/teamforge.exe replay task-<id>
```

## Interview Flow

1. Run mock demo (deterministic, no network)
2. Show `data/workspace/src/url_shortener.cpp` generated artifacts
3. Show SQLite event log via replay
4. Mention gate pass rate metric in CLI output
