# Tests

This directory contains the current automated backend and adapter checks used by the phase-0 baseline.

Current test targets:

- `savt_backend_tests`
- `savt_snapshot_tests`
- `savt_ai_tests`
- `savt_utf8_check`

Recommended Windows entry point:

```powershell
.\scripts\dev\verify-mingw-debug.bat
```

That script runs the smallest repeatable validation path for this repository:

1. configure `qt-mingw-debug` with `SAVT_BUILD_TESTS=ON`
2. build `savt_backend_tests`, `savt_snapshot_tests`, and `savt_ai_tests`
3. run `ctest` against `build/qtcreator-qt6.9.3-mingw-debug`

Use this baseline before and after incremental refactor batches so regressions are visible immediately.

Snapshot coverage lives under `tests/fixtures/precision/` and `tests/golden/precision/`.

- `savt_snapshot_tests` checks normalized golden outputs for `AnalysisReport` JSON/DOT, `ArchitectureOverview`, `CapabilityGraph`, module layout text, capability scene layout text, Markdown reports, and `precision_summary.txt`.
- Syntax and heuristic fixtures run in all test builds.
- Semantic-capable builds additionally cover semantic success and blocked-semantic fixtures such as missing `compile_commands.json` and unresolved system headers.
- Builds without the semantic backend automatically switch to the fallback fixture set, including `semantic_preferred_backend_unavailable`.
