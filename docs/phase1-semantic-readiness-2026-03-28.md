# Phase-1 Semantic Readiness

Date: March 28, 2026

## Goal

This document records the phase-1 acceptance baseline for semantic backend readiness.

The target for this phase is not "full industrial-grade semantics". The target is:

- semantic mode can compile
- semantic mode can probe backend availability
- semantic mode can report a specific blocked reason
- UI and detailed reports keep that blocked reason visible

## Accepted blocked states

- `missing_compile_commands`
- `backend_unavailable`
- `llvm_not_found`
- `llvm_headers_missing`
- `libclang_not_found`
- `compilation_database_load_failed`
- `compilation_database_empty`
- `system_headers_unresolved`
- `translation_unit_parse_failed`

## Verified on March 28, 2026

### Default macOS build

Command sequence:

```bash
export SAVT_QT_ROOT="/Users/adg/Qt/6.9.2/macos"
cmake --preset macos-qt-debug
cmake --build --preset macos-qt-debug --parallel
ctest --preset macos-qt-debug -R savt_backend_tests
ctest --preset macos-qt-debug -R savt_ai_tests
ctest --preset macos-qt-debug -R savt_snapshot_tests
```

Recorded result:

- configure: passed
- build: passed
- `savt_backend_tests`: passed
- `savt_ai_tests`: passed
- `savt_snapshot_tests`: passed

### macOS semantic probe build

Command sequence:

```bash
export SAVT_QT_ROOT="/Users/adg/Qt/6.9.2/macos"
cmake -S . -B build/macos-qt-semantic-probe -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$SAVT_QT_ROOT" \
  -DSAVT_BUILD_TESTS=ON \
  -DSAVT_ENABLE_CLANG_TOOLING=ON
cmake --build build/macos-qt-semantic-probe --target savt_backend_tests --parallel
ctest --test-dir build/macos-qt-semantic-probe --output-on-failure -R savt_backend_tests
```

Recorded result in this workspace:

- configure: passed with a clear CMake warning
- blocked reason: `llvm_not_found`
- `savt_backend_tests`: passed

This confirms that a build with `SAVT_ENABLE_CLANG_TOOLING=ON` but without a usable LLVM package now reports a specific blocked reason instead of collapsing into a generic fallback.

## Windows status

The repository now includes the standard Windows phase-1 entry point:

- `scripts\dev\verify-windows-msvc-semantic-probe.bat`

It was not revalidated in this macOS workspace on March 28, 2026, but it is the intended Windows acceptance path for this phase.
