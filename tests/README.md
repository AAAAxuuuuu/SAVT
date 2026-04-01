# Tests

This directory contains the automated backend and adapter checks used by the shared phase-0 and phase-1 baselines.

Current test targets:

- `savt_backend_tests`
- `savt_snapshot_tests`
- `savt_ai_tests`
- `savt_ui_tests`
- `savt_perf_smoke`
- `savt_utf8_check`

Recommended baseline entry points:

```bash
./scripts/dev/verify-macos-debug.sh
```

```bat
scripts\dev\verify-windows-msvc-debug.bat
```

```bash
./scripts/dev/verify-macos-semantic-probe.sh
```

```bat
scripts\dev\verify-windows-msvc-semantic-probe.bat
```

```bat
scripts\dev\verify-mingw-debug.bat
```

These scripts run the smallest repeatable configure/build/test paths for this repository:

1. configure one shared debug preset with `SAVT_BUILD_TESTS=ON`
2. build `savt_backend_tests`, `savt_snapshot_tests`, `savt_ai_tests`, `savt_ui_tests`, and `savt_perf_baseline`
3. run `ctest` against the matching build directory

Use this baseline before and after incremental refactor batches so regressions are visible immediately.

Additional coverage introduced in phases 9 and 10:

- `savt_ui_tests` covers `AstPreviewService`, `ReportService`, and `AnalysisController` startup/default-state behavior.
- `savt_ui_tests` now also covers the incremental analysis pipeline, including stable-layer cache hits and source-change invalidation.
- `savt_perf_smoke` runs the lightweight performance baseline tool against representative fixtures plus a synthetic workspace sample.
- `./build/.../tests/savt_perf_baseline --full` now records both cold and hot incremental-analysis timings, plus per-layer cache-hit flags for a 10k+ file synthetic sample.

Snapshot coverage lives under `tests/fixtures/precision/` and `tests/golden/precision/`.

- `savt_snapshot_tests` checks normalized golden outputs for `AnalysisReport` JSON/DOT, `ArchitectureOverview`, `CapabilityGraph`, module layout text, capability scene layout text, Markdown reports, and `precision_summary.txt`.
- The precision fixture inventory currently contains 21 named cases. See `tests/fixtures/precision/CATALOG.md` for the maintained catalog.
- The default macOS snapshot baseline exercised in this workspace covers 17 active cases; semantic-capable builds activate the semantic-success and blocked-semantic cases instead of the fallback-only case mix.
- `savt_snapshot_tests` now includes audited substring expectations for representative fixtures, so high-value facts stay guarded even when a full golden diff would be too coarse to explain intent.
- `savt_snapshot_tests` now validates fixture/golden inventory consistency before comparing outputs, so orphaned fixture directories, missing golden artifacts, and stale registrations fail immediately.
- Syntax and heuristic fixtures run in all test builds.
- Semantic-capable builds additionally cover semantic success and blocked-semantic fixtures such as missing `compile_commands.json` and unresolved system headers.
- Builds without the semantic backend automatically switch to the fallback fixture set, including `semantic_preferred_backend_unavailable`.
- Builds where `SAVT_ENABLE_CLANG_TOOLING=ON` is requested but LLVM is still unavailable use the dedicated fallback fixture `semantic_preferred_llvm_not_found`.
