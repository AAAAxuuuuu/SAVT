# 第 10 阶段验收记录：增量分析与分层缓存

日期：2026-04-01

## 本轮目标

把重复分析同一项目时的主要耗时拆成可验证的四层缓存，而不是每次都从零开始：

- 扫描层：项目源码清单与元数据指纹
- 解析层：`AnalysisReport`
- 聚合层：`ArchitectureOverview` 与 `CapabilityGraph`
- 布局层：L2/L3 scene layout 与组件下钻结果

## 主要改动

- 新增公共扫描清单入口：
  - `src/analyzer/include/savt/analyzer/CppProjectAnalyzer.h`
  - `src/analyzer/src/CppProjectAnalyzer.cpp`
- 新增增量分析管线：
  - `src/ui/include/savt/ui/IncrementalAnalysisPipeline.h`
  - `src/ui/src/IncrementalAnalysisPipeline.cpp`
- `AnalysisOrchestrator` 改为通过分层缓存管线执行分析：
  - `src/ui/src/AnalysisOrchestrator.cpp`
- UI 状态消息可显示增量缓存命中层数：
  - `src/ui/src/ReportService.cpp`
- 回归与基线：
  - `tests/cpp/ui_service_tests.cpp`
  - `tests/cpp/perf_baseline.cpp`
  - `tests/CMakeLists.txt`

## 缓存键与失效策略

- 扫描层：
  - 键包含项目根路径、分析选项、项目配置签名、源码元数据指纹
  - 命中后复用已计算的文件内容指纹
- 解析层：
  - 键包含文件内容指纹、编译数据库签名、项目配置签名、analyzer 版本、精度模式
- 聚合层：
  - 键绑定解析层结果
- 布局层：
  - 键绑定聚合层结果

当前策略是同进程缓存。源码内容、`compile_commands.json`、项目配置或分析模式变化后，会触发对应层及其下游层重新计算。

## 验证结果

已执行：

```bash
cmake --build --preset macos-qt6.9.2-debug --parallel --target savt_ui_tests savt_perf_baseline savt_desktop
ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex savt_ui_tests
ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_snapshot_tests|savt_ai_tests'
ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex savt_perf_smoke
./build/vscode-qt6.9.2-macos-debug/tests/savt_perf_baseline --full
```

结果：

```text
label,discovered,parsed,overview_nodes,capability_nodes,capability_edges,cold_total_ms,hot_total_ms,hot_scan_hit,hot_parse_hit,hot_aggregate_hit,hot_layout_hit,peak_memory_mb
cpp_vendored_dependency_app,6,6,2,2,1,3.08,0.45,1,1,1,1,32.08
qml_dashboard_workspace,8,5,5,5,2,3.00,0.54,1,1,1,1,32.30
synthetic_large_10000,10001,10001,21,2,0,4519.41,303.01,1,1,1,1,75.59
```

## 验收结论

第 10 阶段已完成当前验收：

- 第二次分析同一项目时不再重新执行完整解析、聚合和布局链路
- 缓存命中和失效行为已有自动化回归保护
- full baseline 已能量化 cold/hot 差异与层级命中情况
