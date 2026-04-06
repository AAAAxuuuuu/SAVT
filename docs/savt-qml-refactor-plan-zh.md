# SAVT QML 组件拆分与主界面改造计划

## 1. 文档目的

本文档定义 SAVT 桌面端 QML 层的目标架构、组件拆分策略、主界面改造步骤、状态边界与迁移顺序，用于指导 `apps/desktop` 的渐进式重构实施。

文档目标不是一次性重写全部界面，而是在尽量不大改 C++ backend 的前提下，把当前桌面端从“单体 Main.qml + 少量大组件”的结构演进为“应用壳 + 页面容器 + 场景视图 + Inspector/面板 + 共享状态”的稳定架构。

## 2. 当前结构盘点

## 2.1 当前 QML 模块入口

当前 `apps/desktop/CMakeLists.txt` 注册的 QML 文件包括：

- `qml/Main.qml`
- `qml/components/AppButton.qml`
- `qml/components/AppTheme.qml`
- `qml/components/TopControlBar.qml`
- `qml/components/L2GraphView.qml`
- `qml/components/L3ComponentView.qml`
- `qml/components/RightInspector.qml`
- `qml/components/SurfaceCard.qml`
- `qml/components/TagChip.qml`

当前主界面仍高度集中在 [Main.qml](../apps/desktop/qml/Main.qml)，其核心问题是：

- 同时承载 app shell、页面导航、选中态、scene 索引、Inspector 数据组织与边绘制逻辑
- 页面级状态与对象级状态耦合
- L1/L2/L3/L4 结构与 UI 任务流耦合过深
- 重构时难以局部替换

## 2.2 当前后端契约

[AnalysisController.h](../src/ui/include/savt/ui/AnalysisController.h) 已经提供较完整的 UI 契约：

- 项目分析状态：`projectRootPath`、`statusMessage`、`analysisProgress`、`analysisPhase`
- 总览数据：`systemContextData`、`systemContextCards`
- 能力图数据：`capabilityScene`
- 组件图数据：`componentSceneCatalog`
- AST 数据：`astFileItems`、`astPreview*`
- AI 数据：`aiAvailable`、`aiSummary`、`aiEvidence`、`aiNextActions` 等
- 用户动作：`analyzeCurrentProject()`、`stopAnalysis()`、`requestAiExplanation()`、`copyCodeContextToClipboard()` 等

这意味着本轮重构的主策略应该是：

- 保持 `AnalysisController` 作为后端状态与命令入口
- 通过 QML 层重新组织展示、状态分发和交互流程
- 仅在确有必要时为 UI 解耦增加只读属性或轻量命令

## 3. 重构目标

### 3.1 架构目标

- 将 `Main.qml` 缩减为应用壳和页面编排器
- 将页面级状态与对象级状态分离
- 将 Inspector 从全量展示改造为摘要 + 详情承接
- 让 L2/L3 画布拥有清晰的容器边界
- 把复用组件、页面容器、图视图和状态对象拆分到可维护目录

### 3.2 实施目标

- 支持渐进式迁移，不要求一次性重写
- 每个阶段都有可运行的程序壳
- C++ backend 改动最小化
- 新旧页面可阶段性共存

## 4. 目标 QML 目录结构

建议将 `apps/desktop/qml` 重组为以下结构：

```text
apps/desktop/qml
├─ Main.qml
├─ shell
│  ├─ AppShell.qml
│  ├─ AppTopBar.qml
│  ├─ SideNavTree.qml
│  ├─ BreadcrumbBar.qml
│  ├─ WorkspaceHeader.qml
│  └─ BottomContextTray.qml
├─ state
│  ├─ DesktopUiState.qml
│  ├─ NavigationState.qml
│  ├─ SelectionState.qml
│  ├─ CompareState.qml
│  └─ InspectorState.qml
├─ pages
│  ├─ ProjectOverviewPage.qml
│  ├─ CapabilityMapPage.qml
│  ├─ ComponentWorkbenchPage.qml
│  ├─ ReportPage.qml
│  ├─ ComparePage.qml
│  └─ ViewLevelPage.qml
├─ workspace
│  ├─ MapToolbar.qml
│  ├─ ComponentToolbar.qml
│  ├─ ContextActionStrip.qml
│  ├─ InspectorPanel.qml
│  ├─ InspectorSummaryCard.qml
│  ├─ InspectorDetailsDrawer.qml
│  └─ AskSavtPanel.qml
├─ graph
│  ├─ CapabilityGraphCanvas.qml
│  ├─ ComponentGraphCanvas.qml
│  ├─ GraphViewport.qml
│  ├─ CapabilityNodeItem.qml
│  ├─ ComponentNodeItem.qml
│  ├─ GraphEdgeCanvas.qml
│  ├─ GraphMiniMap.qml
│  └─ GraphLegend.qml
├─ report
│  ├─ ReportSummarySection.qml
│  ├─ ReportCapabilitySection.qml
│  ├─ ReportEvidenceSection.qml
│  ├─ ReportActionsSection.qml
│  └─ PromptPackSection.qml
├─ common
│  ├─ AppButton.qml
│  ├─ AppCard.qml
│  ├─ AppChip.qml
│  ├─ AppEmptyState.qml
│  ├─ AppSection.qml
│  ├─ SearchField.qml
│  ├─ SegmentedControl.qml
│  └─ StatusBadge.qml
├─ logic
│  ├─ GraphSelection.js
│  ├─ GraphVisibility.js
│  ├─ InspectorFormatter.js
│  └─ ShortcutRegistry.js
└─ theme
   ├─ AppTheme.qml
   ├─ DesignTokens.qml
   └─ NodeStyleCatalog.js
```

## 5. 组件边界设计

## 5.1 应用壳层

### `Main.qml`

职责：

- 创建 `DesktopUiState`
- 接入 `analysisController`
- 挂载 `AppShell`
- 处理顶层窗口属性

禁止继续承载：

- 图关系计算
- Inspector 文本拼接
- 页面内部工具条逻辑
- 大量选中态帮助函数

### `AppShell.qml`

职责：

- 定义左侧导航、顶部栏、主工作区、右侧 Inspector、底部托盘的全局布局
- 根据导航状态装载当前 page
- 接收全局 keyboard shortcut

## 5.2 状态对象层

### `DesktopUiState.qml`

职责：

- 汇总导航状态、页面模式、布局偏好
- 管理 Inspector 是否展开、底部托盘是否展开、当前主页面等全局 UI 状态

### `SelectionState.qml`

职责：

- 管理当前能力节点、能力边、组件节点、组件边、多选集合
- 提供统一的 `clearSelection()`、`selectCapabilityNode()`、`selectComponentNode()` 等动作

### `InspectorState.qml`

职责：

- 从 `SelectionState` 和 `analysisController` 派生当前摘要对象
- 控制 Inspector 摘要态/详情态/固定态

说明：

- 这里不建议把所有逻辑都塞回 JS；简洁的状态对象更利于 QML 绑定与测试

## 5.3 页面层

### `ProjectOverviewPage.qml`

职责：

- 展示项目总览、主要能力、目录映射、推荐起点
- 不直接管理图选中态

### `CapabilityMapPage.qml`

职责：

- 组织 L2 工具条、能力图画布、上下文条
- 将选中与下钻行为委托给 `SelectionState` 和 `NavigationState`

### `ComponentWorkbenchPage.qml`

职责：

- 展示 L3 工具条、组件图画布、底部托盘内容
- 保留当前父能力上下文

### `ReportPage.qml`

职责：

- 以工程分析报告形式重新组织现有报告与证据数据

## 5.4 图视图层

### `GraphViewport.qml`

职责：

- 统一管理 pan/zoom/fit/selection frame/minimap 坐标系

### `CapabilityGraphCanvas.qml` / `ComponentGraphCanvas.qml`

职责：

- 画布级容器
- 根据 scene 数据组织节点、组、边和网格
- 不直接知道整个 app 的导航结构

### `CapabilityNodeItem.qml` / `ComponentNodeItem.qml`

职责：

- 节点外观与交互
- 不持有全局选中逻辑，只通过属性与信号和外部连接

### `GraphEdgeCanvas.qml`

职责：

- 统一边绘制逻辑
- 将当前 `Main.qml` 中的 `drawCapabilityEdge()` / `drawComponentEdge()` 收拢到专用组件

## 5.5 Inspector 与详情层

### `InspectorPanel.qml`

职责：

- 挂载摘要卡、动作区和详情入口

### `InspectorSummaryCard.qml`

职责：

- 展示固定的 5 个摘要区块

### `InspectorDetailsDrawer.qml`

职责：

- 承接长证据链、长文件列表、AI 长输出等

### `AskSavtPanel.qml`

职责：

- 承接 Ask SAVT 的解释、建议和提示词结果
- 可由 Inspector、顶部栏或节点快捷动作拉起

## 6. 当前组件到目标组件的映射

| 当前文件 | 当前问题 | 目标落点 |
| --- | --- | --- |
| `qml/Main.qml` | 单体过大、职责混杂 | `Main.qml` + `shell/AppShell.qml` + `state/*` + `pages/*` + `graph/*` |
| `qml/components/TopControlBar.qml` | 同时承担项目入口与状态展示 | `shell/AppTopBar.qml` + `pages/ProjectOverviewPage.qml` 的局部摘要 |
| `qml/components/L2GraphView.qml` | 工具条、画布、状态逻辑耦合 | `pages/CapabilityMapPage.qml` + `graph/CapabilityGraphCanvas.qml` + `workspace/MapToolbar.qml` |
| `qml/components/L3ComponentView.qml` | 页面、工具条、过滤、图和上下文耦合 | `pages/ComponentWorkbenchPage.qml` + `graph/ComponentGraphCanvas.qml` + `workspace/ComponentToolbar.qml` |
| `qml/components/RightInspector.qml` | 内容过载、AI 与证据堆叠 | `workspace/InspectorPanel.qml` + `workspace/InspectorSummaryCard.qml` + `workspace/InspectorDetailsDrawer.qml` + `workspace/AskSavtPanel.qml` |
| `qml/components/AppTheme.qml` | token 与语义组件耦合度不够 | `theme/DesignTokens.qml` + `theme/AppTheme.qml` |
| `qml/components/SurfaceCard.qml` | 命名偏展示层，不利于组件系统 | `common/AppCard.qml` |
| `qml/components/TagChip.qml` | 元数据与行为边界不清 | `common/AppChip.qml` + `common/StatusBadge.qml` |

## 7. Main.qml 改造目标

## 7.1 改造前

`Main.qml` 当前承担：

- 顶层背景和窗口
- 页面切换
- 大量派生状态与函数
- 节点/边的选择与查找
- Inspector 数据选择
- 部分场景尺寸与边绘制逻辑

## 7.2 改造后

`Main.qml` 应缩减为：

```qml
ApplicationWindow {
    AppTheme { id: theme }
    DesktopUiState { id: uiState }

    AppShell {
        anchors.fill: parent
        theme: theme
        uiState: uiState
        analysisController: analysisController
    }
}
```

换句话说，`Main.qml` 只保留：

- 窗口属性
- 主题实例
- 顶层状态对象
- AppShell 挂载

## 8. 推荐状态分层

## 8.1 原则

- Backend 状态放在 `AnalysisController`
- UI 导航状态放在 `NavigationState`
- 对象选中状态放在 `SelectionState`
- 展示摘要状态放在 `InspectorState`
- 页面模式和布局偏好放在 `DesktopUiState`

## 8.2 不建议的做法

- 不建议继续在 `Main.qml` 写几十个辅助函数拼状态
- 不建议让多个页面分别维护自己的全局选中源
- 不建议让 Inspector 自己“猜测”证据与解释

## 9. 页面装载方式

推荐使用 `Loader + page component registry` 或 `StackLayout` 的轻量壳，但页面标识不再直接绑定 `selectedC4LevelIndex`。

建议统一为：

- `pageId = "project.overview"`
- `pageId = "project.capabilityMap"`
- `pageId = "project.compare"`
- `pageId = "report.engineering"`
- `pageId = "levels.l1"`
- `pageId = "levels.l2"`
- `pageId = "levels.l3"`
- `pageId = "levels.l4"`

这样既能保持清晰导航，又能在实现中继续复用现有 L1-L4 数据来源。

## 10. CMake 与 QML module 改造建议

当前 `qt_add_qml_module` 直接枚举少量文件。重构后应：

- 将新增目录中的 QML 文件纳入 `QML_FILES`
- 保持 `URI SAVT.Desktop`
- 如引入 JS helper，确保通过资源系统打包

推荐分阶段更新 `apps/desktop/CMakeLists.txt`，不要一次性重写全部条目。

## 11. 分阶段实施计划

## Phase A：建立新壳，不动旧页面

目标：

- 新增 `shell/`、`state/`、`common/`、`theme/` 基础结构
- 保留旧的 `TopControlBar`、`L2GraphView`、`L3ComponentView`、`RightInspector`
- 先让 `AppShell` 接管全局布局

产出：

- 新的左侧导航壳
- 顶部全局栏
- 可切换的主工作区

## Phase B：接入项目总览页

目标：

- 新增 `ProjectOverviewPage.qml`
- 将原 `systemContextPageComponent` 从 `Main.qml` 中剥离
- 总览页与导航树打通

产出：

- `项目 / 总览` 可独立工作
- `L1` 可通过专业入口继续保留

## Phase C：拆分 L2 能力地图

目标：

- 从 `L2GraphView.qml` 中拆出工具条、画布、节点项、边绘制层
- 引入 `SelectionState`

产出：

- `CapabilityMapPage.qml`
- `MapToolbar.qml`
- `CapabilityGraphCanvas.qml`
- `CapabilityNodeItem.qml`
- `GraphEdgeCanvas.qml`

## Phase D：拆分 Inspector

目标：

- 将 `RightInspector.qml` 改为摘要壳
- 增加详情抽屉与 Ask SAVT 面板

产出：

- `InspectorPanel.qml`
- `InspectorSummaryCard.qml`
- `InspectorDetailsDrawer.qml`
- `AskSavtPanel.qml`

## Phase E：拆分 L3 组件工作台

目标：

- 拆出组件工具条、图画布、底部托盘、路径追踪上下文

产出：

- `ComponentWorkbenchPage.qml`
- `ComponentToolbar.qml`
- `ComponentGraphCanvas.qml`
- `BottomContextTray.qml`

## Phase F：报告与 Compare

目标：

- 以新导航体系接入报告页与对比页

产出：

- `ReportPage.qml`
- `ComparePage.qml`

## 12. 后端改动建议（最小化）

默认不改动已有 scene 与报告数据结构，仅考虑以下可选增强：

- 新增更稳定的 `projectSummary` 只读属性，减少总览页二次拼装
- 为 Compare 增加更明确的 UI 输入接口
- 为 Ask SAVT 增加区分 action type 的命令入口

如果这些增强没有时间做，也可以先由 QML 基于现有属性组织 UI。

## 13. 测试与验收建议

### 13.1 每阶段验收

- 程序能启动，无新的 QML 运行时警告
- 原有分析主路径可走通
- 新旧页面切换不丢核心功能
- Inspector 与图选中联动正确

### 13.2 回归关注点

- 选中能力后，L3 下钻是否仍能正确加载 `componentSceneCatalog`
- 右侧摘要是否还能准确消费 evidence package
- AI 解释是否仍能从节点数据发起请求
- `analysisProgress`、`analysisPhase`、`statusMessage` 是否继续正确呈现

### 13.3 建议测试层次

- QML smoke check
- UI service tests（复用现有 `savt_ui_tests` 思路）
- 桌面端手工走查：总览、能力地图、L3 下钻、Inspector、Ask SAVT、报告页

## 14. 风险与规避策略

### 风险 1：重构初期页面与状态双轨并存导致复杂度上升

规避：

- 先明确 `AppShell + State` 的边界
- 一次只替换一类页面，不并行做多个高耦合重构

### 风险 2：QML 侧派生逻辑迁移时行为回归

规避：

- 保留旧函数作为参考
- 先抽纯函数到 `logic/*.js`
- 再将 UI 绑定迁移到新组件

### 风险 3：图交互拆分后性能下降

规避：

- 节点 item、边 canvas、viewport 容器边界清晰
- 不在多个层级重复遍历 scene
- 优先复用 scene 事实，不让 QML 自己重算布局

### 风险 4：Inspector 迁移后证据链展示失真

规避：

- 摘要层只消费 evidence package
- 长内容统一迁到详情抽屉
- 不在多个组件中重复“猜测式拼文案”

## 15. 第一阶段建议落地文件清单

建议优先新增以下文件：

- `apps/desktop/qml/shell/AppShell.qml`
- `apps/desktop/qml/shell/AppTopBar.qml`
- `apps/desktop/qml/shell/SideNavTree.qml`
- `apps/desktop/qml/state/DesktopUiState.qml`
- `apps/desktop/qml/state/NavigationState.qml`
- `apps/desktop/qml/state/SelectionState.qml`
- `apps/desktop/qml/pages/ProjectOverviewPage.qml`
- `apps/desktop/qml/common/AppCard.qml`
- `apps/desktop/qml/common/AppChip.qml`
- `apps/desktop/qml/theme/DesignTokens.qml`

这批文件能先把 app shell、导航和总览页搭起来，同时不阻塞后续拆 L2/L3/Inspector。

## 16. 推荐实施顺序总结

1. 先把 `Main.qml` 减负为应用壳入口
2. 先建立新 app shell 和状态层
3. 再迁移项目总览页
4. 再拆 L2 能力地图
5. 再拆 Inspector
6. 再拆 L3 工作台
7. 最后接入报告与 Compare

## 17. 与方案文档的关系

本技术计划对应的产品与设计依据见：

- [savt-ui-refactor-plan-zh.md](./savt-ui-refactor-plan-zh.md)
