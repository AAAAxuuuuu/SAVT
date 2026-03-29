# 第 5 阶段验收记录：统一 layout / scene 事实源

日期：2026-03-28

## 阶段目标

第 5 阶段的目标不是改 UI 样式，也不是重写 layout 算法，而是把 L2 视图里关于节点、边、分组、坐标和场景尺寸的事实源统一起来。

本阶段要求：

- layout 层成为 L2 坐标和边路由的唯一事实来源
- UI 不再兜底计算节点矩形和边路径
- controller 不再把 scene 事实拆成多份内部状态分别维护
- scene 数据以单一 DTO 形式从 `AnalysisOrchestrator` 进入 `AnalysisController`

## 本轮完成内容

### 1. 引入统一 capability scene DTO

`SceneMapper` 现在除了生成 `CapabilitySceneData` 外，还负责把它映射成单一 `QVariantMap` scene payload。

scene payload 统一包含：

- `nodes`
- `edges`
- `groups`
- `bounds`

其中 `bounds` 直接来自 layout 结果的场景宽高，不再由 QML 单独维护一套尺寸事实。

### 2. 收拢 `AnalysisOrchestrator` 和 `AnalysisController` 的 scene 状态

`PendingAnalysisResult` 不再分别存 `nodeItems`、`edgeItems`、`groupItems`、`sceneWidth`、`sceneHeight`。

改为只保存一份 `CapabilitySceneData`。

`AnalysisController` 也同步改成单一内部 scene 状态：

- 新增 `capabilityScene` 属性，作为 QML 的主读取入口
- 旧的 `capabilityNodeItems` / `capabilityEdgeItems` / `capabilityGroupItems` / `capabilitySceneWidth` / `capabilitySceneHeight` 保留为兼容 getter，但全部派生自同一份 scene 数据

这意味着 controller 内部不再有多份 scene 事实各自更新。

### 3. 去掉 QML 侧对布局事实的兜底计算

L2 视图和 `Main.qml` 已改成直接使用 `analysisController.capabilityScene`。

本轮明确移除了两类兜底逻辑：

- 节点矩形不再从 `x/y/width/height` 与 `layoutBounds` 之间二选一
- 边路径不再在 `routePoints` 缺失时由 QML 临时补算直线或贝塞尔路径

现在如果 scene payload 里没有这些事实，就视为后端没有提供，而不是让 UI 再造一套事实。

## 验证结果

本轮已在 macOS 语义构建基线上完成验证：

- `cmake --build --preset macos-qt6.9.2-debug --parallel` 通过
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_snapshot_tests|savt_ai_tests'` 通过

新增回归约束：

- `savt_backend_tests` 现在直接校验 `SceneMapper` 会把节点、边、分组和 bounds 统一发布到单一 scene payload 中
- scene payload 中的节点必须包含 layout bounds
- scene payload 中的边必须包含 route points

## 本阶段验收结论

第 5 阶段可以视为完成当前验收。

当前 L2 相关事实的边界已经变成：

- layout 负责坐标和路由
- `SceneMapper` 负责把 layout 结果和 capability graph 拼成统一 scene DTO
- controller 只维护并暴露单一 scene 状态
- QML 只消费 scene，不再自行推导布局事实

## 未在本阶段处理的内容

以下内容明确不属于本阶段：

- 真实 L3 组件视图
- 结果证据链和 inspector 追溯
- 分层分析 / 局部分析 / 全量分析的产品模式
- 增量分析和缓存

这些内容应分别进入第 6、7、8、10 阶段处理。
