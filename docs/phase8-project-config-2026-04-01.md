# 第 8 阶段收口记录

日期：2026-04-01

## 目标

引入项目级配置系统，削弱硬编码启发式，让项目特例可以在不改源码规则的情况下被纠正。

## 本次落地

### 1. 配置模型与读盘

新增 [ProjectAnalysisConfig.h](/Users/adg/Documents/code/SAVT/src/core/include/savt/core/ProjectAnalysisConfig.h) 和 [ProjectAnalysisConfig.cpp](/Users/adg/Documents/code/SAVT/src/core/src/ProjectAnalysisConfig.cpp)：

- 支持 `config/savt.project.json` 等候选路径
- 支持：
  - `ignoreDirectories`
  - `moduleMerges`
  - `roleOverrides`
  - `entryOverrides`
  - `dependencyFoldRules`
- 配置加载状态和解析异常会进入 diagnostics

### 2. 扫描与模块推断接入

在 [AnalyzerUtilities.cpp](/Users/adg/Documents/code/SAVT/src/analyzer/src/AnalyzerUtilities.cpp) 接入：

- 目录扫描时尊重 `ignoreDirectories`
- 模块推断时尊重 `moduleMerges`

在 [AnalysisGraphBuilder.cpp](/Users/adg/Documents/code/SAVT/src/analyzer/src/AnalysisGraphBuilder.cpp) 接入：

- 注册模块时加载项目配置
- 文件到模块的归并结果可被项目配置覆盖

### 3. Overview / Capability 接入

在 [ArchitectureOverview.cpp](/Users/adg/Documents/code/SAVT/src/core/src/ArchitectureOverview.cpp) 接入：

- `roleOverrides`
- `entryOverrides`

在 [CapabilityGraph.cpp](/Users/adg/Documents/code/SAVT/src/core/src/CapabilityGraph.cpp) 接入：

- `dependencyFoldRules`
- 把配置命中规则写入 capability evidence rules

### 4. 仓库默认配置

新增 [savt.project.json](/Users/adg/Documents/code/SAVT/config/savt.project.json)：

- 忽略 `tests/fixtures`、`tests/golden`、`docs`、`config`
- 固定 `apps/desktop`、`src/ui`、`src/layout`、`src/parser`、`src/analyzer`、`src/ai`、`src/core` 的角色
- 显式声明 `apps/desktop` 为入口
- 默认折叠 `third_party` 和 `tools/llvm`

### 5. 文档与测试

新增 [project-config.md](/Users/adg/Documents/code/SAVT/docs/project-config.md)，说明配置路径、字段和示例。

在 [c4_backend_tests.cpp](/Users/adg/Documents/code/SAVT/tests/cpp/c4_backend_tests.cpp) 新增回归：

- `testProjectConfigSkipsConfiguredDirectoriesAndMergesModules`
- `testProjectConfigOverridesEntryRoleAndDependencyFolding`

## 验证结果

已验证：

- `cmake --build --preset macos-qt6.9.2-debug --parallel --target savt_backend_tests savt_snapshot_tests savt_desktop`
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex savt_backend_tests`
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex savt_snapshot_tests`
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex savt_ai_tests`

## 当前结论

第 8 阶段已完成当前验收：

- 用户可以通过项目配置纠正扫描范围、模块归并、角色分类、入口识别和依赖折叠
- 字符串启发式不再是唯一事实来源
