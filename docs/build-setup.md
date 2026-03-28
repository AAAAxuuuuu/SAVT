# Build Setup

## Goal

This document defines the shared phase-0 build baseline for SAVT.

The repository keeps shared presets portable and avoids hard-coded machine paths. Local Qt, LLVM, and MinGW locations should come from environment variables or `CMakeUserPresets.json`.

## Common requirements

- CMake 3.24 or newer
- Qt 6.9.x
- A C++20-capable toolchain
- Ninja on `PATH` for the shared macOS and Windows MSVC presets

On the first configure, SAVT can fetch `tree-sitter` and `tree-sitter-cpp` automatically if `third_party/` is empty.

## Shared presets

- `macos-qt-debug`
- `macos-qt-release`
- `windows-msvc-qt-debug`
- `windows-msvc-qt-release`
- `windows-mingw-qt-debug`
- `windows-mingw-qt-release`

The recommended Windows path is MSVC.

MinGW remains available as a compatibility path, not the primary baseline.

## Environment variables

### macOS

- `SAVT_QT_ROOT`
  Example: `/Users/you/Qt/6.9.2/macos`

### Windows MSVC

- `SAVT_QT_ROOT`
  Example: `C:\Qt\6.9.3\msvc2022_64`
- `SAVT_LLVM_ROOT`
  Example: `D:\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc`
- Optional: `SAVT_VCVARS_PATH`
  Use this when automatic `vcvars64.bat` detection is not enough.
- Optional: `SAVT_NINJA_ROOT`
  Use this when Ninja is not already on `PATH`.

### Windows MinGW compatibility path

- `SAVT_QT_ROOT`
  Example: `C:\Qt\6.9.3\mingw_64`
- `SAVT_MINGW_ROOT`
  Example: `C:\Qt\Tools\mingw1310_64`

## Optional user presets

If you prefer not to set environment variables globally:

1. Copy `CMakeUserPresets.template.json` to `CMakeUserPresets.json`
2. Fill in your local paths
3. Keep `CMakeUserPresets.json` local only

## Reference commands

### macOS

```bash
export SAVT_QT_ROOT="$HOME/Qt/6.9.2/macos"
cmake --preset macos-qt-debug
cmake --build --preset macos-qt-debug --parallel
ctest --preset macos-qt-debug
```

### Windows MSVC

```bat
set SAVT_QT_ROOT=C:\Qt\6.9.3\msvc2022_64
set SAVT_LLVM_ROOT=D:\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc
scripts\dev\verify-windows-msvc-debug.bat
```

### Windows MinGW

```bat
set SAVT_QT_ROOT=C:\Qt\6.9.3\mingw_64
set SAVT_MINGW_ROOT=C:\Qt\Tools\mingw1310_64
scripts\dev\verify-mingw-debug.bat
```

## Notes

- `config/deepseek-ai.local.json` is local-only and must not be committed.
- `CMakeLists.txt.user` is local Qt Creator state and must not be committed.
- `.qmlc`, `moc_*.cpp`, `qrc_*.cpp`, and similar Qt outputs are generated files and must not be committed.
