# Industrial Precision Roadmap

## Non-negotiable requirements

For C++ architecture analysis, industrial-grade precision requires all of the following:

1. `compile_commands.json` for the target project.
2. A semantic frontend based on `Clang/LibTooling`.
3. Compiler-compatible system include resolution.
4. Symbol identity based on semantic declarations, not syntax-only names.
5. Cross-translation-unit merge logic for declarations, definitions, overrides, templates, and macro-expanded code.

## What the current codebase already has

- Project scanning.
- Syntax graph extraction.
- Include graph extraction.
- First-pass call graph extraction.
- Module aggregation.
- Layered layout.
- Precision-mode enforcement and compilation database discovery.

## What is still missing for true industrial precision

- LLVM/Clang toolchain installation on this machine.
- CMake integration that enables `SAVT_ENABLE_CLANG_TOOLING`.
- A real semantic analyzer implementation behind that flag.
- Semantic symbol canonicalization and cross-TU graph merge.
- High-fidelity call/override/inheritance/usage extraction from Clang AST and index data.

## Practical consequence

The repository is now able to refuse pretending that syntax-level output is industrial-grade precision.
If `SemanticRequired` is selected and the semantic backend is unavailable, analysis is blocked and diagnostics explain why.
