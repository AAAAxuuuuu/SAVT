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

The repository now distinguishes these semantic readiness states explicitly:

- `missing_compile_commands`
- `backend_unavailable`
- `llvm_not_found`
- `llvm_headers_missing`
- `libclang_not_found`
- `system_headers_unresolved`
- `translation_unit_parse_failed`

That means semantic mode now either enters the Clang/LibTooling backend successfully, or stops with a specific blocked reason instead of silently degrading.

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

`config/deepseek-ai.local.json` is a local machine file and should stay out of version control.

## Build

This repository now keeps shared presets portable and moves machine-specific Qt paths into environment variables or `CMakeUserPresets.json`.

Recommended shared presets:

- macOS: `macos-qt-debug`
- Windows MSVC: `windows-msvc-qt-debug`
- Windows MinGW compatibility path: `windows-mingw-qt-debug`

Required environment variables:

- `SAVT_QT_ROOT`
- `SAVT_MINGW_ROOT` for the MinGW compatibility path only
- `SAVT_LLVM_ROOT` only when enabling the semantic backend

Useful entry points:

```bash
./scripts/dev/verify-macos-debug.sh
```

```bash
./scripts/dev/verify-macos-semantic-probe.sh
```

```bat
scripts\dev\verify-windows-msvc-debug.bat
```

```bat
scripts\dev\verify-windows-msvc-semantic-probe.bat
```

```bat
scripts\dev\verify-mingw-debug.bat
```

The first configure can automatically fetch `tree-sitter` and `tree-sitter-cpp` if `third_party/` does not already contain them. Set `-DSAVT_FETCH_THIRD_PARTY=OFF` if you need a fully pre-populated offline build.

See:

- `docs/build-setup.md`
- `docs/qtcreator-setup.md`
- `docs/semantic-toolchain-setup.md`
- `docs/project-structure.md`
- `docs/project-config.md`

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
cmake --build --preset windows-msvc-qt-debug --target savt_check_utf8
```

The UTF-8 check validates file bytes directly instead of trusting terminal display, which helps catch cases where a console looks garbled even though the file itself is still correct.

## Boundary

- Everything under `src/` and `apps/` is your codebase.
- Everything under `third_party/` is third-party dependency material and should be treated as external when present locally.

See also: docs/industrial-precision-roadmap.md

## Reference directions

The current product and backend direction is informed by a few external reference projects:

- `Codevis` for LLVM/Clang-backed C++ extraction and analysis/model separation.
- `ContextFlow` for multi-view architecture reading, progressive reveal, and relationship filtering.
- `DocuEye` for pairing diagrams with synchronized documentation and evidence panels.

These are reference inputs only. SAVT keeps its own Qt/QML frontend, analysis model, and C4-oriented presentation pipeline.
