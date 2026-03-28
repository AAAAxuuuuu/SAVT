# Phase-3 Precision Baseline

Date: March 28, 2026

## Goal

This document records the current phase-3 baseline for precision fixtures, golden outputs, and regression visibility.

The target for this phase is not "finished benchmark science". The target is:

- keep a representative precision fixture set in-repo
- keep snapshot and golden outputs synchronized
- make fixture/golden drift fail immediately
- expand coverage beyond the original minimal syntax/semantic smoke cases

## Repository state after this refresh

- Precision fixture directories: 11
- Default macOS snapshot cases exercised in this workspace: 7
- Snapshot artifact types per case: 9

Representative fixture coverage now includes:

- C++ syntax-only layered application structure
- C++ application plus vendored dependency boundary
- Java backend package structure
- JS backend package structure
- QML mixed UI/backend/data structure
- Python tooling + data workflow
- Semantic success fixture for cross-translation-unit C++
- Semantic blocked/fallback fixtures for missing compilation database, unresolved system headers, backend unavailable, and LLVM requested but not found

## Changes accepted in this phase-3 pass

### 1. Restored the Python tooling/data fixture baseline

`python_tooling_data_project` now includes its tooling entrypoint again via `tools/build_graph.py`, so the fixture once more exercises:

- a tooling entry module
- analytics/storage/support modules
- data inventory alongside code modules

### 2. Added a vendored dependency fixture

`cpp_vendored_dependency_app` was added to keep a golden baseline for:

- primary app entry remaining visible
- vendored dependency classification
- dependency flow from app module to third-party module

### 3. Added inventory validation to `savt_snapshot_tests`

The snapshot test now fails early when:

- a fixture directory exists but is not registered
- a golden directory exists but is not registered
- a registered fixture has no source files
- a registered case is missing its golden artifacts

This closes the gap where snapshot drift could remain unnoticed until a human happened to inspect the artifacts manually.

## Verified on March 28, 2026

Command sequence:

```bash
export SAVT_QT_ROOT="/Users/adg/Qt/6.9.2/macos"
cmake --build --preset macos-qt-debug --parallel --target savt_snapshot_tests
./build/macos-qt-debug/tests/savt_snapshot_tests --update
ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_snapshot_tests
ctest --preset macos-qt-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_ai_tests'
```

Recorded result:

- snapshot golden update: passed
- `savt_snapshot_tests`: passed
- `savt_backend_tests`: passed
- `savt_ai_tests`: passed

## Remaining gap versus the full phase-3 target

This repository revision materially improves the baseline, but phase 3 is not fully finished yet.

Remaining work still includes:

- growing the representative sample set from 11 toward 20+ projects/cases
- adding more manually reviewed goldens for difficult semantic scenarios
- introducing explicit precision scoring or audited expected facts for selected fixtures
