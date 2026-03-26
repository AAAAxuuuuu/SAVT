# Project Structure

## Design rules

- Keep parser logic independent from Qt.
- Keep UI thin and route all business logic through service/controller classes.
- Keep graph aggregation and layout in dedicated modules so rendering remains a consumer, not the place where analysis logic lives.
- Treat third-party code as external; do not mix project code into vendored repositories.

## Directory map

- `apps/desktop`
  - Desktop target and QML entry point.
- `src/core`
  - Source files, syntax trees, graph/domain models, and export/report formatting.
- `src/parser`
  - Language adapters and parser facades.
- `src/analyzer`
  - Project scanning, symbol extraction, include/call analysis, and module aggregation.
- `src/layout`
  - Layered layout and future visualization-oriented graph algorithms.
- `src/ui`
  - Qt controller layer.
- `samples`
  - Small reproducible inputs used for manual testing.
- `tests`
  - Unit and integration tests.
- `third_party`
  - External code only. Do not place project logic here.
