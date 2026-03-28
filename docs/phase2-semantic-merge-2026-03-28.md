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
- 当前缺口主要在 semantic-capable 机器验收、样本覆盖和复杂语义场景

## 验证结果

本轮在当前 macOS 基线下完成以下验证：

- `cmake --build --preset macos-qt-debug --parallel --target savt_backend_tests` 通过
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_backend_tests` 通过
- `cmake --build --preset macos-qt-debug --parallel --target savt_snapshot_tests` 通过
- `ctest --preset macos-qt-debug --output-on-failure --tests-regex savt_snapshot_tests` 仍只有既有失败 `python_tooling_data_project`

## 当前判断

从代码实现角度看，第 2 阶段的主干能力已经基本收口：

- 语义 identity 主路径成立
- 跨 TU 合并主路径成立
- 语义边去重与 supportCount 聚合成立
- “按名字误合并语义节点”的关键漏洞已修正

但严格按阶段验收口径，仍有一个收尾项不能在这台机器上完成：

- 需要在真正具备 LLVM/Clang headers + `libclang` 的 semantic-capable 环境里，实跑 `cpp_semantic_cross_tu` 和相关 snapshot fixture，完成最终验收

因此，当前更准确的表述是：

- 第 2 阶段代码侧已基本收口
- 第 2 阶段最终验收还差一轮 semantic-capable 机器验证
