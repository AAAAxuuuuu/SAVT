# Phase-0 Baseline

Date: March 28, 2026

## Scope

This document records the current phase-0 baseline after standardizing the repository around shared CMake presets and local machine overrides.

## Shared build entry points

- macOS: `scripts/dev/verify-macos-debug.sh`
- Windows MSVC: `scripts/dev/verify-windows-msvc-debug.bat`
- Windows MinGW compatibility path: `scripts/dev/verify-mingw-debug.bat`

## Current repository expectations

- `CMakePresets.json` stays shared and machine-agnostic.
- `CMakeUserPresets.json` stays local-only.
- Local secrets stay out of version control.
- Generated Qt files stay out of version control.
- Missing `third_party/` parser sources can be fetched automatically during configure.
- A clean checkout with an empty `third_party/` directory must still configure and build.

## Verified result on March 28, 2026

Validated locally on macOS from a clean `third_party/` state with:

- Qt root: `/Users/adg/Qt/6.9.2/macos`
- Compiler: AppleClang 21.0.0
- Generator: Ninja
- Preset: `macos-qt-debug`

Command sequence:

```bash
export SAVT_QT_ROOT="/Users/adg/Qt/6.9.2/macos"
cmake --preset macos-qt-debug
cmake --build --preset macos-qt-debug --parallel
ctest --preset macos-qt-debug
```

Recorded result:

- `cmake --preset macos-qt-debug`: passed
- `cmake --build --preset macos-qt-debug --parallel`: passed
- `savt_backend_tests`: passed
- `savt_ai_tests`: passed
- `savt_snapshot_tests`: failed

Known failing area:

- `python_tooling_data_project` snapshot output currently differs on macOS and writes artifacts under `build/macos-qt-debug/snapshot_artifacts/actual/`

## Windows status

The Windows MSVC and Windows MinGW entry points are standardized in this repository revision, but they were not revalidated inside this macOS workspace on March 28, 2026.

They should be treated as the required next verification targets when Windows access is available.
