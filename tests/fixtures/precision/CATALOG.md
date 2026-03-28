# Precision Fixture Catalog

This catalog tracks the repository-owned precision fixtures used by `savt_snapshot_tests`.

Current inventory on March 28, 2026: 21 fixture directories.

## Syntax and heuristic coverage

| Case | Focus |
| --- | --- |
| `cpp_batch_tooling` | C++ tooling entry + domain dependency |
| `cpp_header_only_lib` | C++ header-only library and UI consumer |
| `cpp_layered_pipeline` | C++ app -> interaction -> service -> storage layering |
| `cpp_plugin_host` | C++ host/plugin boundary |
| `cpp_syntax_only` | C++ application baseline |
| `cpp_vendored_dependency_app` | C++ app plus vendored dependency boundary |
| `js_backend` | Node backend route/controller/service/store chain |
| `js_express_inventory` | Node backend with explicit repository layer |
| `js_worker_pipeline` | Node worker/job/service/store chain |
| `python_cli_pipeline` | Python CLI + service/storage/support/data structure |
| `python_reporting_workspace` | Python tooling/reporting workspace |
| `python_tooling_data_project` | Python tooling entry + analytics/storage/data |
| `qml_dashboard_workspace` | Mixed C++ backend + QML presentation workspace |
| `qml_mixed_project` | Mixed QML/backend/data project |
| `spring_boot_inventory` | Java Spring Boot controller/service/repository/model chain |
| `spring_boot_java` | Java Spring Boot baseline |

## Semantic and blocked-state coverage

| Case | Focus |
| --- | --- |
| `cpp_semantic_cross_tu` | Cross-translation-unit semantic merge success |
| `semantic_required_missing_compile_commands` | Required semantic mode blocked by missing compilation database |
| `semantic_required_system_headers_unresolved` | Required semantic mode blocked by unresolved system headers |
| `semantic_preferred_backend_unavailable` | Preferred semantic mode fallback without backend |
| `semantic_preferred_llvm_not_found` | Preferred semantic mode when LLVM was requested but unavailable |

## Audited cases

The snapshot test also contains explicit audited expectations for selected representative cases, beyond full-file golden matching.

Audited on March 28, 2026:

- `cpp_batch_tooling`
- `cpp_layered_pipeline`
- `cpp_plugin_host`
- `cpp_vendored_dependency_app`
- `js_backend`
- `js_express_inventory`
- `python_cli_pipeline`
- `python_tooling_data_project`
- `qml_dashboard_workspace`
- `qml_mixed_project`
- `spring_boot_inventory`
- `spring_boot_java`
- `semantic_required_missing_compile_commands`
- `semantic_required_system_headers_unresolved`
- `semantic_preferred_backend_unavailable`
