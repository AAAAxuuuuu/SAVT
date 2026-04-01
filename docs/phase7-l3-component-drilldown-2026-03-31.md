# 第 7 阶段验收记录：L3 组件下钻

日期：2026-03-31

## 阶段目标

把 L3 从占位式 AST 预览替换成真正的组件视图，让用户可以从 L2 能力域继续下钻，查看单个能力域内部的组件、阶段和协作关系。

## 本次落地内容

### 1. 新增 capability -> component 的核心数据模型

- 新增 `ComponentGraph`，由 `ArchitectureOverview + CapabilityGraph` 构建能力域内部组件图。
- 组件节点区分 `entry_component / component / support_component`。
- 组件边区分 `activates / coordinates / uses_support`。
- 节点、边继续复用第 6 阶段的 `evidence package`。

对应文件：

- `src/core/include/savt/core/ComponentGraph.h`
- `src/core/src/ComponentGraph.cpp`

### 2. 新增 L3 专用 layout 和 scene 映射

- `LayeredGraphLayout` 新增 `layoutComponentScene(...)`。
- `SceneMapper` 新增 `ComponentSceneData` 和对应 `toVariantMap(...)`。
- `AnalysisOrchestrator` 在 L2 capability scene 之外，额外构建每个 capability 对应的 component scene catalog。

对应文件：

- `src/layout/include/savt/layout/LayeredGraphLayout.h`
- `src/layout/src/LayeredGraphLayout.cpp`
- `src/ui/include/savt/ui/SceneMapper.h`
- `src/ui/src/SceneMapper.cpp`
- `src/ui/include/savt/ui/AnalysisOrchestrator.h`
- `src/ui/src/AnalysisOrchestrator.cpp`

### 3. 主窗口正式接入 L3 组件页

- `Main.qml` 新增 component scene catalog、组件索引和通用 inspector 选择逻辑。
- L3 页不再加载 AST 占位页，而是加载新的 `L3ComponentView.qml`。
- 支持从 L2 双击节点进入 L3。

对应文件：

- `apps/desktop/qml/Main.qml`
- `apps/desktop/qml/components/L3ComponentView.qml`
- `apps/desktop/CMakeLists.txt`

### 4. Inspector 升级为 L2/L3 通用

- `RightInspector.qml` 不再绑定 capability 专用字段。
- 节点、边、端点跳转、AI 解读入口都改为使用统一的 inspector helper。
- L3 组件节点也能查看证据链和关系证据。

对应文件：

- `apps/desktop/qml/components/RightInspector.qml`

### 5. 补充后端回归测试

- 新增 capability -> component -> scene 的端到端后端回归。
- 覆盖组件节点/边/分组数量、类型分类、权重保留、evidence 透传和 scene payload。

对应文件：

- `tests/cpp/c4_backend_tests.cpp`

## 验收结果

### 功能验收

- L2 节点双击后可以进入对应 capability 的 L3 组件图。
- L3 支持返回 L2、组件过滤、关系聚焦 / 全量切换。
- L3 节点和边都可在右侧 inspector 查看“事实 -> 规则 -> 结论”证据链。
- 没有可展开组件时，会显示明确的空状态和诊断说明。

### 技术验收

执行通过：

- `cmake --build --preset macos-qt6.9.2-debug --parallel`
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_snapshot_tests|savt_ai_tests'`

桌面启动 smoke check 通过：

- `build/vscode-qt6.9.2-macos-debug/apps/desktop/savt_desktop`

## 阶段结论

第 7 阶段已完成当前验收。

SAVT 现在已经具备：

- L1 系统上下文
- L2 能力域 / 容器图
- L3 单能力域组件下钻
- L4 Markdown 全景报告

下一阶段应进入第 8 阶段：项目级配置系统与硬编码启发式收口。
