# Tests

This directory contains the automated backend and adapter checks used by the shared phase-0 baseline.

Current test targets:

- `savt_backend_tests`
- `savt_snapshot_tests`
- `savt_ai_tests`
- `savt_utf8_check`

Recommended baseline entry points:

```bash
./scripts/dev/verify-macos-debug.sh
```

```bat
scripts\dev\verify-windows-msvc-debug.bat
```

```bat
scripts\dev\verify-mingw-debug.bat
```

These scripts run the smallest repeatable configure/build/test paths for this repository:

1. configure one shared debug preset with `SAVT_BUILD_TESTS=ON`
2. build `savt_backend_tests`, `savt_snapshot_tests`, and `savt_ai_tests`
3. run `ctest` against the matching build directory

Use this baseline before and after incremental refactor batches so regressions are visible immediately.

Snapshot coverage lives under `tests/fixtures/precision/` and `tests/golden/precision/`.

- `savt_snapshot_tests` checks normalized golden outputs for `AnalysisReport` JSON/DOT, `ArchitectureOverview`, `CapabilityGraph`, module layout text, capability scene layout text, Markdown reports, and `precision_summary.txt`.
- Syntax and heuristic fixtures run in all test builds.
- Semantic-capable builds additionally cover semantic success and blocked-semantic fixtures such as missing `compile_commands.json` and unresolved system headers.
- Builds without the semantic backend automatically switch to the fallback fixture set, including `semantic_preferred_backend_unavailable`.
