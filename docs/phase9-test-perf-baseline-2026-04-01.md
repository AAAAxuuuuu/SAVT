# 第 9 阶段收口记录

日期：2026-04-01

## 目标

补齐测试矩阵和性能基线，让核心模块既有自动化守护，也有可复现的性能记录。

## 本次落地

### 1. UI 服务层自动化回归

新增 [ui_service_tests.cpp](/Users/adg/Documents/code/SAVT/tests/cpp/ui_service_tests.cpp)：

- 覆盖 `AstPreviewService`
  - AST 文件列表生成
  - 默认 AST 文件选择
  - C++ 文件 AST 预览
- 覆盖 `ReportService`
  - 状态消息生成
  - L1 system context 数据和卡片生成
- 覆盖 `AnalysisController`
  - 启动默认状态
  - 默认项目根
  - 默认 AST / scene / AI 状态

### 2. 性能基线工具

新增 [perf_baseline.cpp](/Users/adg/Documents/code/SAVT/tests/cpp/perf_baseline.cpp)：

- `savt_perf_smoke`
  - 默认 quick 模式
  - 跑代表性 fixture 和小型合成工作区
- `savt_perf_baseline --full`
  - 跑代表性 fixture
  - 跑 10k+ 文件规模的合成工程
  - 输出：
    - discovered / parsed files
    - overview nodes
    - capability nodes / edges
    - analyze / overview / capability / layout 耗时
    - total 耗时
    - 峰值内存

### 3. 测试矩阵扩展

更新 [tests/CMakeLists.txt](/Users/adg/Documents/code/SAVT/tests/CMakeLists.txt) 和 [tests/README.md](/Users/adg/Documents/code/SAVT/tests/README.md)：

- 新增 `savt_ui_tests`
- 新增 `savt_perf_smoke`
- 把性能基线工具纳入标准测试入口说明

## 实际验证

本机已验证：

- `ctest --preset macos-qt6.9.2-debug --output-on-failure`
  - `savt_backend_tests`
  - `savt_snapshot_tests`
  - `savt_ai_tests`
  - `savt_ui_tests`
  - `savt_perf_smoke`
  - 全部通过

## full 基线记录

执行：

```bash
./build/vscode-qt6.9.2-macos-debug/tests/savt_perf_baseline --full
```

输出如下：

```text
label,discovered,parsed,overview_nodes,capability_nodes,capability_edges,analyze_ms,overview_ms,capability_ms,layout_ms,total_ms,peak_memory_mb
cpp_vendored_dependency_app,6,6,2,2,1,2.45,0.29,0.26,0.05,3.05,31.05
qml_dashboard_workspace,8,5,5,5,2,1.07,0.29,0.19,0.01,1.56,31.17
synthetic_large_10000,10001,10001,21,2,0,887.76,3088.43,4.05,0.01,3980.26,62.86
```

## 当前结论

第 9 阶段已完成当前验收：

- 核心模块自动化守护已扩展到 UI 服务层
- 已有可重复执行的性能基线工具
- 已记录 10k+ 文件规模下的耗时和峰值内存
