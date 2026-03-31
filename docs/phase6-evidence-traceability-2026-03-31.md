# 第 6 阶段验收记录：补齐结果可追溯能力

日期：2026-03-31

## 阶段目标

第 6 阶段的目标不是继续美化界面，也不是提前做 L3，而是让当前 L2 图里的结论可追溯。

本阶段要求：

- capability node 和 edge 都带有统一 evidence package
- evidence package 至少包含事实、规则、结论、来源文件、相关符号、关系线索和置信提示
- scene payload 完整透传 evidence，不让 QML 自己拼解释
- inspector 能按“事实 -> 规则 -> 结论”展示，并支持关系级证据下钻
- AI 说明继续保留，但明确降为附加层

## 本轮完成内容

### 1. 在 core 层为 node / edge 生成统一 evidence package

`CapabilityGraph` 现在会直接给 `CapabilityNode` 和 `CapabilityEdge` 生成 evidence package。

evidence package 当前统一包含：

- `facts`
- `rules`
- `conclusions`
- `sourceFiles`
- `symbols`
- `modules`
- `relationships`
- `confidenceLabel`
- `confidenceReason`

这意味着证据链的主来源已经固定在 core 层，而不是散落在 controller 或 QML 里。

### 2. scene payload 透传 evidence

`SceneMapper` 现在会把 node / edge 的 evidence package 直接序列化进 scene payload。

scene payload 中：

- 节点包含 `evidence`
- 边包含 `evidence`

QML inspector 不再需要自己用 `topSymbols`、`exampleFiles`、`summary` 去临时拼一份“像解释的东西”。

### 3. inspector 改成“事实 -> 规则 -> 结论”结构

右侧 inspector 已重组为以下信息流：

- 当前选中图元素摘要
- 证据结论和置信提示
- 事实
- 规则
- 结论
- 证据来源
- 关系入口
- AI 附加说明

其中“关系入口”支持从已选中节点继续点选某条关系，查看该 edge 的证据包。

这一步明确补上了之前只有节点、没有边级证据入口的问题。

### 4. AI 退回附加层

本轮没有移除 AI，但把它放回到更合理的位置：

- 当选中模块节点时，AI 仍可生成补充解释
- 当选中关系边时，AI 区域会明确提示“关系解释先看证据链”

因此，证据链现在是主路径，AI 说明是可选补充，而不是反过来。

## 验证结果

本轮已在 macOS 语义构建基线上完成验证：

- `cmake --build --preset macos-qt6.9.2-debug --parallel` 通过
- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_snapshot_tests|savt_ai_tests'` 通过
- 桌面程序 3 秒启动 smoke check 通过，未出现新的 QML 运行时告警

新增回归约束：

- `savt_backend_tests` 现在直接校验 capability node / edge 会生成 evidence package
- `SceneMapper` 测试会校验 scene payload 中节点和边都包含 nested evidence map

## 本阶段验收结论

第 6 阶段可以视为完成当前验收。

当前证据链边界已经变成：

- core 负责生成 node / edge 的 evidence package
- scene 负责把 evidence 原样透传给前端
- QML inspector 只负责展示，不再自己构造解释事实
- AI 解读在证据链之后出现，不再替代证据本身

## 未在本阶段处理的内容

以下内容明确不属于本阶段：

- 真实 L3 组件视图
- 项目级配置系统
- 分层分析 / 局部分析 / 全量分析能力落地
- 增量分析和缓存

这些内容应分别进入第 7、8、10 阶段处理。
