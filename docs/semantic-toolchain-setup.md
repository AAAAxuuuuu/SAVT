# Semantic Toolchain Setup

## Goal

Semantic analysis is expected to run on the Windows MSVC toolchain path, not on the MinGW compatibility path.

This document defines the portable setup contract for that workflow.

## Required components

- Visual Studio 2022 with C++ build tools
- A Qt 6.9.x `msvc2022_64` kit
- LLVM/Clang for Windows with CMake package files
- Ninja on `PATH`, or a local directory exposed through `SAVT_NINJA_ROOT`

## Environment variables

- `SAVT_QT_ROOT`
  Example: `C:\Qt\6.9.3\msvc2022_64`
- `SAVT_LLVM_ROOT`
  Example: `D:\llvm\clang+llvm-21.1.7-x86_64-pc-windows-msvc`
- Optional: `SAVT_VCVARS_PATH`
  Use this when automatic `vcvars64.bat` detection is not enough.
- Optional: `SAVT_NINJA_ROOT`
  Use this when `ninja.exe` is not already on `PATH`.

## Standard entry points

1. Run `scripts\dev\enter-semantic-env.bat` to open a shell with MSVC, Qt, and LLVM on `PATH`.
2. Run `scripts\dev\configure-msvc-qt-debug.bat` to configure the shared `windows-msvc-qt-debug` preset with `SAVT_ENABLE_CLANG_TOOLING=ON`.
3. Run `scripts\dev\build-msvc-qt-debug.bat` to build that preset.
4. Run `scripts\dev\verify-windows-msvc-semantic-probe.bat` to execute the phase-1 semantic readiness probe.
5. If you need a Visual Studio generator build instead of Ninja, run `scripts\dev\configure-msvc-vs-debug.bat`.

## Expected failure modes

The repository should fail clearly in these cases instead of silently downgrading:

- LLVM/Clang is not installed or `SAVT_LLVM_ROOT` is wrong
- Qt MSVC kit is missing or `SAVT_QT_ROOT` is wrong
- MSVC environment cannot be initialized
- the analyzed project does not provide a usable `compile_commands.json`

## Standard runtime status codes

The analyzer should now surface these states explicitly:

- `semantic_ready`
- `missing_compile_commands`
- `backend_unavailable`
- `llvm_not_found`
- `llvm_headers_missing`
- `libclang_not_found`
- `compilation_database_load_failed`
- `compilation_database_empty`
- `system_headers_unresolved`
- `translation_unit_parse_failed`

`semantic_required` mode must stop on these blocked states.

`semantic_preferred` mode may fall back to syntax analysis, but it must keep the blocked reason visible in status text, diagnostics, and the detailed report.

## Notes

- `compile_commands.json` is required for semantic-required analysis of the target project.
- The phase-1 baseline standardizes build probing, runtime status codes, and blocked-reason reporting first; it does not claim that every project analyzed by SAVT already has full industrial-grade semantic extraction.
