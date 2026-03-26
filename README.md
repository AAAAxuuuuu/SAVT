# SAVT

SAVT is a Qt/C++ software architecture visualization tool under active development.

## Precision direction

The target is industrial-grade C++ analysis accuracy, not a syntax-only explorer. That requires two conditions:

1. a semantic backend built on Clang/LibTooling
2. a valid `compile_commands.json` for the analyzed project

The current repository is now structured to distinguish clearly between:

- syntax-only analysis
- semantic-preferred analysis
- semantic-required analysis

At the moment, this workspace can detect compilation database readiness and enforce semantic-required mode, but the Clang/LibTooling backend itself is not yet built in this environment because LLVM/Clang is not installed and linked into the project.

## Current backend capability

1. Recursively scans a project and inventories C/C++, QML, Web, Python, and JSON architecture-relevant files.
2. Parses C/C++ source files with `tree-sitter-cpp`.
3. Extracts namespaces, classes, functions, methods, file-level include dependencies, and a first pass of call edges.
4. Aggregates files into adaptive modules and builds weighted module dependency edges.
5. Derives architecture overview and capability/C4-friendly clusters with cross-artifact evidence such as example files and collaborator hints.
6. Computes a layered module layout that is ready for rendering.
7. Provides report, JSON, and DOT export functions from the backend graph model.
8. Detects when semantic analysis is required but unavailable, instead of silently pretending syntax-level output is industrial-grade precision.
9. Exposes an on-demand Tree-sitter AST preview in the desktop UI for analyzed C++ source files.

## AI-Assisted Analysis

SAVT now includes a bounded AI adapter for architecture explanations, supporting both the official DeepSeek API and OpenAI-compatible proxy endpoints.

- Config template: `config/deepseek-ai.template.json`
- Local secret config path: `config/deepseek-ai.local.json`
- Rule and request builder module: `src/ai/`
- Setup notes: `docs/ai-assisted-analysis.md`
- Current UI entry: select a node in the desktop graph, then use the AI explanation card in the right inspector panel

The current adapter is intentionally restricted: it is only allowed to help with SAVT architecture reading tasks, must stay inside the supplied evidence package, and must not behave like a general assistant.

## Build

This workspace includes a ready-to-use `CMakePresets.json` for the local Qt MinGW toolchain discovered on this machine.

Recommended Qt Creator build directories:

- Debug: `build/qtcreator-qt6.9.3-mingw-debug`
- Release: `build/qtcreator-qt6.9.3-mingw-release`

Minimal baseline verification:

```powershell
.\scripts\dev\verify-mingw-debug.bat
```

This is the phase-0 reference path for repeatable local validation. It configures `qt-mingw-debug` with `SAVT_BUILD_TESTS=ON`, builds `savt_backend_tests`, `savt_snapshot_tests`, and `savt_ai_tests`, then runs `ctest` for the current 4-test baseline. The recorded result is documented in `docs/baseline-validation-2026-03-24.md`.

Manual configure:

```powershell
& 'F:\Qt\Tools\CMake_64\bin\cmake.exe' --preset qt-mingw-debug
```

Manual build:

```powershell
& 'F:\Qt\Tools\CMake_64\bin\cmake.exe' --build --preset qt-mingw-debug
```

## Encoding hygiene

This repository treats source and text files as UTF-8.

- `.editorconfig` declares a UTF-8 baseline for editors that support it.
- CMake now passes UTF-8 source flags to MSVC and GCC-family C/C++ compilers.
- Raw byte validation is available via:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\dev\check_utf8.ps1
```

- On Windows CMake builds, you can also run:

```powershell
& 'F:\Qt\Tools\CMake_64\bin\cmake.exe' --build --preset qt-mingw-debug --target savt_check_utf8
```

The UTF-8 check validates file bytes directly instead of trusting terminal display, which helps catch cases where a console looks garbled even though the file itself is still correct.

## Boundary

- Everything under `src/` and `apps/` is your codebase.
- Everything under `third_party/` is vendored dependency or reference material and should be treated as external.

See also: docs/industrial-precision-roadmap.md

## Reference directions

The current product and backend direction is informed by a few external reference projects:

- `Codevis` for LLVM/Clang-backed C++ extraction and analysis/model separation.
- `ContextFlow` for multi-view architecture reading, progressive reveal, and relationship filtering.
- `DocuEye` for pairing diagrams with synchronized documentation and evidence panels.

These are reference inputs only. SAVT keeps its own Qt/QML frontend, analysis model, and C4-oriented presentation pipeline.
