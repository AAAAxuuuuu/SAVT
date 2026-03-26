# Open-Source Evaluation

## Candidate summary

### Sourcetrail

- Role: reference implementation only.
- Why it is useful:
  - Strong product benchmark for source-code exploration.
  - Mature separation between application, indexing, data model, and UI.
  - Demonstrates one workable Qt/C++ architecture for this problem space.
- Why it is not the right codebase to modify directly:
  - Archived upstream.
  - GPLv3, which is restrictive if the project later changes licensing or distribution strategy.
  - UI stack is based on Qt Widgets and `QGraphicsView`, while this project should stay free to move toward modern Qt Quick or custom rendering.
  - Dependency footprint is large and old.

### tree-sitter

- Role: primary parsing runtime for the MVP.
- Why it is useful:
  - MIT licensed.
  - Pure C runtime, easy to embed into CMake-based C++ projects.
  - Very fast and resilient for syntax-tree construction.
  - Good starting point for multi-language support later.
- Known limitation:
  - It gives syntax structure first, not full semantic resolution.

### tree-sitter-cpp

- Role: first language adapter.
- Why it is useful:
  - MIT licensed.
  - Pairs directly with `tree-sitter`.
  - Ships with generated parser sources, so it can be built without adding a code-generation step to the MVP.

### Clang / LibTooling

- Role: second-stage semantic parser for C/C++.
- Why it is useful:
  - Best option for accurate symbol resolution, include handling, compile-command fidelity, and semantic analysis.
- Why it is not fully pulled in yet:
  - Heavy dependency and high integration cost.
  - Better introduced after the parser, model, and UI pipeline already exists.

### Codevis

- Role: industrial-reference architecture for C++ extraction.
- Why it is useful:
  - Uses `LLVM and Clang` as the heavy-lifting layer for understanding C++ codebases.
  - Stores extracted code intelligence in a queryable database layer instead of tying parsing directly to UI rendering.
  - Treats the analysis result as a reusable graph/model, which aligns with SAVT's need to support both architecture views and programmer drill-down.
- What SAVT should borrow:
  - Compilation-database-first semantic extraction for C/C++.
  - A clean boundary between extraction, persisted/intermediate model, and presentation.
  - The idea that UI views are consumers of a common analysis model, not owners of the analysis logic.

### ContextFlow

- Role: product-reference architecture view system.
- Why it is useful:
  - Shows the same architecture elements through multiple synchronized views instead of forcing one overloaded diagram.
  - Uses flow-oriented reading, strategic grouping, and ownership/context lenses to reduce cognitive overload.
  - Emphasizes progressive reveal: start simple, then switch view to answer a different question.
- What SAVT should borrow:
  - Keep the same node set but let users switch between C4/system, flow, and programmer-oriented views.
  - Use focused relationship modes and grouped reading paths instead of drawing every edge at once.
  - Preserve node identity across views so non-programmers and programmers are still talking about the same part of the system.

### DocuEye

- Role: documentation-oriented presentation reference.
- Why it is useful:
  - Pairs architecture views with readable documentation instead of treating the diagram as self-explanatory.
  - Strong fit for C4-style communication where a node should carry narrative, not only shape and color.
  - Encourages drill-down from high-level view to evidence, files, and structured descriptions.
- What SAVT should borrow:
  - Every node should have a human-readable explanation.
  - The right-side inspector should explain what a node does, why SAVT thinks so, and which artifacts support that conclusion.
  - Architecture view and documentation panel should stay synchronized.

## Working decision

The project should be built as a fresh Qt/C++ codebase with:

- `tree-sitter` as the first parser runtime,
- `tree-sitter-cpp` as the initial language grammar,
- `Clang / LibTooling` as the industrial-precision semantic stage for C/C++,
- `Codevis` used as an extraction-pipeline reference,
- `ContextFlow` used as a multi-view interaction reference,
- `DocuEye` used as a documentation-and-evidence presentation reference,
- `Sourcetrail` kept only as a reference repository under `third_party`.

## Adoption plan

### What SAVT should borrow now

- `Sourcetrail`
  - Borrow the product decomposition only: keep a clear split between scanning/indexing, architecture model building, and UI presentation.
  - Use it as a benchmark for programmer-oriented traceability, not as a code donor.
- `tree-sitter`
  - Keep it as the syntax runtime and extend it gradually toward more languages that matter for C4 extraction, especially QML and Python.
  - Use it for resilient partial parsing even when semantic information is unavailable.
- `Clang / LibTooling`
  - Keep it as the industrial-precision semantic stage for C/C++.
  - Feed its results into the same architecture model instead of building a separate UI path.
- `Codevis`
  - Use semantic extraction to produce a reusable model that can serve reports, C4 views, evidence cards, and future exports.
  - Keep compilation database handling explicit and first-class.
  - Do not let the UI define what the parser extracts.
- `ContextFlow`
  - Keep C4 levels, relationship modes, and future flow views as different lenses over one shared set of nodes.
  - Default to a readable focused view and let users expand toward more complete views only when needed.
- `DocuEye`
  - Give each node a synchronized explanation panel with responsibility, evidence, collaborators, and drill-down paths.
  - Treat architecture comprehension as both visualization and documentation.

### What SAVT should not do

- Do not turn `Sourcetrail` into the application foundation.
- Do not treat syntax-only output as final architectural truth when semantic data is missing.
- Do not stay C++-only at the project inventory level if the target C4 model clearly depends on QML, Web, Python, or resource evidence.
- Do not let diagram density become the default interaction when a focused reading path answers the user's question faster.
- Do not rely on generic node descriptions when evidence exists to explain a module more specifically.
