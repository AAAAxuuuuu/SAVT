# 第 4 阶段验收记录：AnalysisController 等价重构

日期：2026-03-28

## 阶段目标

第 4 阶段的目标不是改 UI，也不是改 analyzer 算法，而是把 `AnalysisController` 收回到“QML 状态 + 用户动作入口”的角色。

本阶段要求：

- 等价重构，不顺手改产品行为。
- 继续下沉控制器内残留的分析编排和结果组装逻辑。
- 清理已经被服务层替代的旧实现。

## 本轮完成内容

### 1. 统一分析结果组装入口

`AnalysisOrchestrator::run(...)` 不再依赖外部 `ResultAssembler` 回调。

现在分析流程内部直接负责：

- AST 预览组装
- capability scene 组装
- 状态消息生成
- Markdown 报告生成
- system context 数据组装

这样 `AnalysisController` 不再承接“分析完以后怎么拼结果”的编排责任。

### 2. 删除控制器中的历史重复实现

从 `AnalysisController.cpp` 中移除了已经失效或被服务层替代的旧逻辑，包括：

- 旧的 AST 预览辅助函数
- 旧的 scene/layout 组装路径
- 旧的 status/system context/report 拼装函数
- 旧的整条 `runAnalysisTask(...)` 分析执行路径
- 旧的 AI request / reply 辅助构造函数
- 未被使用的 L1/L4 私有导出函数

这次删除的重点不是“换一种写法”，而是把同一件事只保留一条事实来源。

### 3. 控制器只保留 UI 控制职责

当前 `AnalysisController` 主要保留：

- QML 暴露状态
- 用户动作入口
- 分析任务启动/结束后的状态同步
- AST 预览切换
- AI 请求触发与结果回填
- 复制上下文到剪贴板

这更符合控制器本职。

## 结果

- `AnalysisController.cpp`：`2221` 行 -> `825` 行
- `AnalysisOrchestrator.cpp`：`208` 行 -> `256` 行

这说明复杂度没有消失，而是被收回到更合理的边界里。

## 验证结果

本轮已在 macOS 基线上完成验证：

- `cmake --build --preset macos-qt-debug --parallel`
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_ai_tests'`
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_snapshot_tests`

结果：

- `savt_backend_tests` 通过
- `savt_ai_tests` 通过
- `savt_snapshot_tests` 通过

## 本阶段验收结论

第 4 阶段可以视为完成当前验收。

理由是：

- `AnalysisController` 已经不再承担核心分析逻辑。
- 现有 UI 行为保持不变。
- 分析编排与结果组装已经统一回到服务层 / orchestrator。

## 未在本阶段处理的内容

以下内容明确不属于本阶段：

- L2/L3 布局事实源进一步统一
- 证据链模型
- L3 下钻视图
- 图形样式和界面美化

这些内容应分别进入第 5、6、7 阶段处理。
