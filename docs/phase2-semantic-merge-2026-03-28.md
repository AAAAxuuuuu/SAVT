# 第 2 阶段收口记录

日期：2026-03-28

## 本轮目标

第 2 阶段的目标不是再证明“仓库里有一份 SemanticProjectAnalyzer.cpp”，而是把语义抽取链收口成一条可验证的后端路径：

- 符号以语义声明 / `USR` 为主 identity
- declarations / definitions / overrides / inherits / calls / uses-type 可以跨 TU 合并
- 语义事实不会再被名字级启发式误合并
- 输出里能明确区分 `FactSource::Semantic` 和 `FactSource::Inferred`

## 本轮完成内容

### 1. 修正图构建层的 identity 合并规则

`AnalysisGraphBuilder` 之前存在一个关键漏洞：

- 当新节点带有语义 identity，但 builder 还找不到该 identity 时，仍会继续按 `qualifiedName` 回退合并。
- 对 C++ 重载函数，这会把两个不同的语义实体错误压成一个节点。

本轮修正后：

- 优先按 identity 合并。
- 仅当 `qualifiedName` 唯一且 identity 兼容时，才允许按名字回退合并。
- 对同名但不同 identity 的语义节点，不再任意压扁。
- `findSymbolByQualifiedName()` 在 qualified name 不唯一时返回“不确定”，不再随机指向其中一个节点。

### 2. 增补 builder 级回归测试

新增的测试覆盖了两类关键行为：

- syntax 路径下，唯一且无 identity 的重复声明仍然按 `qualifiedName` 合并。
- semantic 路径下，同名重载但 identity 不同的节点必须保留为两个独立符号；再次看到同一 identity 时仍能稳定合并。

这组测试不依赖本机是否安装 LLVM/Clang，因此可以在当前 macOS 基线里持续守住 phase-2 的核心约束。

### 3. 修正文档口径

`industrial-precision-roadmap.md` 之前仍把“真实 semantic analyzer implementation”写成缺失项，这已经与当前代码状态不符。

本轮已改为：

- 语义后端实现已经存在
- 当前缺口主要在样本覆盖、复杂语义场景以及后续证据链建设

## 验证结果

首轮在当前 macOS 基线下完成以下验证：

- `cmake --build --preset macos-qt-debug --parallel --target savt_backend_tests` 通过
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_backend_tests` 通过
- `cmake --build --preset macos-qt-debug --parallel --target savt_snapshot_tests` 通过
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_snapshot_tests` 仍只有既有失败 `python_tooling_data_project`

补充验收时，在已启用 `SAVT_ENABLE_CLANG_TOOLING=ON`、`SAVT_LLVM_ROOT=/opt/homebrew/opt/llvm` 的 semantic-capable macOS 环境里完成以下验证：

- `ctest --preset macos-qt6.9.2-debug --output-on-failure --tests-regex 'savt_backend_tests|savt_snapshot_tests|savt_ai_tests'` 通过
- `cpp_semantic_cross_tu` 中模板类型 `Box` 的跨 TU identity 归并通过
- `semantic_required_missing_compile_commands` 与 `semantic_required_system_headers_unresolved` 的报告快照已同步到当前语义状态输出

## 当前判断

从当前代码和实机验证结果看，第 2 阶段已经完成当前验收：

- 语义 identity 主路径成立
- 跨 TU 合并主路径成立
- 语义边去重与 supportCount 聚合成立
- “按名字误合并语义节点”的关键漏洞已修正

因此，当前更准确的表述是：

- 第 2 阶段已完成当前验收
- 后续工作重心应转向第 5 阶段 scene/layout 统一，以及第 6 阶段结果可追溯能力
