# SAVT 项目说明书

## 1. 文档信息

| 项目项 | 内容 |
| --- | --- |
| 项目名称 | SAVT |
| 英文全称 | Software Architecture Visualization Tool |
| 文档名称 | 项目说明书 |
| 文档版本 | v1.0 |
| 编制日期 | 2026-04-04 |
| 文档状态 | 基于当前仓库实现整理 |
| 适用对象 | 项目负责人、架构师、研发工程师、测试工程师、技术评审人员 |

## 2. 项目概述

SAVT 是一个基于 Qt 6、QML 与现代 C++ 构建的软件架构分析与可视化工具，面向中大型代码仓库的结构理解、模块关系梳理、能力域划分、组件协作分析以及工程文档辅助生成场景。

本项目的核心定位不是通用 IDE，也不是单文件代码阅读器，而是一个“面向架构理解”的工程分析平台。它通过项目扫描、语法/语义分析、图模型聚合、分层布局、可视化呈现以及受约束的 AI 解释能力，帮助使用者快速建立对目标系统的整体认知。

## 3. 建设背景与目标

### 3.1 建设背景

随着项目规模增大、模块边界复杂化以及多语言技术栈的广泛使用，单纯依赖文件树或源码全文检索难以高效建立系统级认知。尤其在重构评审、项目接手、技术尽调、架构汇报和跨角色沟通场景中，团队更需要一种能够从“源码事实”出发生成结构化架构视图的工具。

### 3.2 建设目标

SAVT 当前阶段的建设目标如下：

1. 支持对目标项目进行工程化扫描与结构化建模。
2. 以 C/C++ 为优先方向，兼顾 QML、JavaScript/TypeScript、Python、Java 等架构相关文件清单能力。
3. 明确区分语法级分析与语义级分析，避免在语义条件不满足时给出误导性的“高精度”结论。
4. 构建统一的分析图模型，为多级架构视图、报告和后续导出能力提供公共数据底座。
5. 提供 L1-L4 多层级展示能力，兼顾全局理解与局部下钻。
6. 建立可复现的构建、测试、基线验证和性能验证体系，符合工程化演进要求。

## 4. 项目定位

从产品属性看，SAVT 属于“软件架构分析与可视化平台”；从工程属性看，它是一个采用分层模块化设计的 Qt/C++ 桌面应用；从能力属性看，它强调以下三个关键词：

- 可验证：分析结论尽量来源于源码、结构规则和证据包，而不是纯展示层推断。
- 可解释：节点、边和能力域均支持携带事实、规则、结论和置信提示。
- 可演进：语法分析、语义分析、项目级配置、增量缓存、AI 解释、导出报告等能力可独立扩展。

## 5. 总体架构

### 5.1 分层架构

SAVT 采用“分析底座与展示层解耦”的分层架构。总体技术结构如下：

```text
+--------------------------------------------------------------+
| Desktop App Layer                                            |
| apps/desktop  +  QML views                                   |
+--------------------------+-----------------------------------+
                           |
                           v
+--------------------------------------------------------------+
| UI / Orchestration Layer                                     |
| AnalysisController / AnalysisOrchestrator / SceneMapper      |
| ReportService / AstPreviewService / AiService                |
+--------------------------+-----------------------------------+
                           |
                           v
+--------------------------------------------------------------+
| Layout Layer                                                 |
| LayeredGraphLayout                                           |
+--------------------------+-----------------------------------+
                           |
                           v
+--------------------------------------------------------------+
| Core Model Layer                                             |
| AnalysisReport / ArchitectureOverview / CapabilityGraph      |
| ComponentGraph / ProjectAnalysisConfig                       |
+--------------------------+-----------------------------------+
                           |
                           v
+--------------------------------------------------------------+
| Parser & Analyzer Layer                                      |
| Tree-sitter parser / SyntaxProjectAnalyzer /                 |
| SemanticProjectAnalyzer / CppProjectAnalyzer                 |
+--------------------------+-----------------------------------+
                           |
                           v
+--------------------------------------------------------------+
| External Dependencies                                        |
| tree-sitter / tree-sitter-cpp / optional libclang backend    |
+--------------------------------------------------------------+
```

### 5.2 核心处理流程

SAVT 的一次完整分析大致包含以下步骤：

1. 扫描项目目录，收集架构相关文件并过滤构建产物、工具目录和忽略目录。
2. 根据精度模式选择语法分析路径或语义分析路径。
3. 生成基础分析结果 `AnalysisReport`，构建节点、边和诊断信息。
4. 聚合得到 `ArchitectureOverview` 与 `CapabilityGraph`。
5. 对 L2 能力视图和 L3 组件视图执行布局计算。
6. 将后端图模型映射为 QML 可消费的 scene 数据。
7. 在桌面端展示 L1-L4 视图、AST 预览、系统上下文信息和右侧检查面板。
8. 按需触发受约束的 AI 解释能力，为节点或项目提供辅助说明。

## 6. 功能体系说明

### 6.1 项目扫描与文件清单

项目扫描能力负责识别项目中的架构相关文件，当前支持纳入分析或清单建模的文件类型包括：

- C/C++ 源码与头文件
- QML 文件
- JavaScript / TypeScript 文件
- Python 文件
- Java 文件
- HTML / CSS / JSON 等结构相关资源
- Qt、CMake、Gradle、Maven 等项目清单文件

同时，系统会默认跳过典型的构建目录、缓存目录、第三方目录和生成文件，例如 `build`、`node_modules`、`target`、`CMakeFiles`、Qt 自动生成文件以及锁文件等。

### 6.2 分析精度模式

SAVT 明确区分以下三种分析精度模式：

- `SyntaxOnly`：仅执行语法级分析。
- `SemanticPreferred`：优先尝试语义分析，失败时可回退到语法分析。
- `SemanticRequired`：必须进入语义分析，否则直接阻断并输出原因。

这一设计用于保证“精度声明”和“实际能力”一致，避免在缺少语义基础设施时将结果误认为工业级精度输出。

### 6.3 语法分析能力

当前语法分析路径主要基于 `tree-sitter` 与 `tree-sitter-cpp`，可完成以下任务：

- C/C++ 文件解析
- 命名空间、类、结构体、函数、方法等基础符号抽取
- 文件级包含关系抽取
- 首轮调用关系推断
- 模块聚合与依赖边构建

此外，对于 JavaScript、TypeScript、Python、Java 等文件，系统具备启发式架构相关识别与聚合能力，可用于构建项目级结构视图和测试样例覆盖。

### 6.4 语义分析能力

SAVT 支持通过 `Clang/LibTooling` 或 `libclang` 路径进入语义分析后端，用于提升 C/C++ 项目的分析精度。语义分析成功时，系统能够进一步支持：

- 编译数据库驱动的真实编译上下文分析
- 更可靠的符号身份识别
- 跨翻译单元关系合并
- 继承、覆盖、调用、类型使用等语义边增强

语义分析依赖以下关键条件：

1. 构建时启用 `SAVT_ENABLE_CLANG_TOOLING`
2. 提供可用的 LLVM/Clang 运行环境
3. 目标项目存在有效的 `compile_commands.json`
4. 系统头文件与编译环境可被正确解析

### 6.5 语义阻断与诊断

为保证工程可信度，系统对语义不可用状态进行显式诊断。仓库当前已覆盖或体现的典型状态包括：

- `missing_compile_commands`
- `backend_unavailable`
- `llvm_not_found`
- `llvm_headers_missing`
- `libclang_not_found`
- `system_headers_unresolved`
- `translation_unit_parse_failed`
- `compilation_database_load_failed`
- `compilation_database_empty`
- `libclang_index_creation_failed`

在 `SemanticRequired` 模式下，上述问题会导致分析被阻断，而不是静默退化。

### 6.6 多层级架构视图

SAVT 当前桌面端提供四个主要认知层级：

- L1 系统上下文：展示项目定位、系统整体脉络、主要容器和高层摘要。
- L2 能力域/容器视图：展示能力域、模块间关系、依赖链路和分组结果。
- L3 组件下钻视图：对单个能力域进行组件级展开，查看内部协作关系。
- L4 全景报告：输出 Markdown 形式的静态分析报告。

### 6.7 AST 预览能力

桌面端支持在完成项目分析后，对已分析的 C++ 源文件进行 Tree-sitter AST 预览，用于辅助理解语法树层级和局部源码结构。

### 6.8 报告与导出能力

当前后端已具备多种结果格式化能力，包括：

- 文本化分析摘要
- Markdown 报告
- JSON 序列化输出
- DOT 图描述输出

这些输出能力为后续的文档交付、调试核验、自动化测试和可视化扩展提供了基础。

### 6.9 AI 辅助解释

SAVT 具备受约束的 AI 架构解释能力，当前特点如下：

- 支持官方 DeepSeek API 和兼容 OpenAI Chat Completions 风格的代理网关
- 仅在桌面端由用户手动触发
- 仅使用系统提供的证据包，不直接发送整个仓库
- 明确限制为 SAVT 架构阅读任务，不承担通用聊天助手角色
- 输出为结构化结果，便于 UI 消费
- 默认以简体中文返回

该能力是“辅助解释层”，不替代静态分析事实本身。

### 6.10 项目级配置能力

SAVT 支持项目级分析配置，用于减少硬编码启发式带来的局限。当前支持的配置项包括：

- `ignoreDirectories`
- `moduleMerges`
- `roleOverrides`
- `entryOverrides`
- `dependencyFoldRules`

系统会优先在以下路径中查找配置文件：

1. `config/savt.project.json`
2. `config/savt-project.json`
3. `.savt/project.json`
4. `savt.project.json`

### 6.11 增量分析与分层缓存

SAVT 已具备同进程分层增量缓存机制，用于提升重复分析的效率。当前缓存分为四层：

- 扫描层：项目清单与元数据指纹
- 解析层：`AnalysisReport`
- 聚合层：`ArchitectureOverview` 与 `CapabilityGraph`
- 布局层：L2/L3 场景布局结果

缓存键会绑定项目路径、分析选项、项目配置、编译数据库状态、源文件内容指纹以及缓存版本等信息，用于保证命中与失效行为可解释。

## 7. 关键数据模型

SAVT 当前的关键数据模型如下：

### 7.1 AnalysisReport

作为底层分析结果载体，包含：

- 根路径
- 主分析引擎
- 精度级别
- 语义状态码与状态信息
- 编译数据库路径
- 节点集合
- 边集合
- 诊断信息

### 7.2 ArchitectureOverview

面向整体架构总览的中间模型，包含：

- 模块节点
- 依赖边
- 分组信息
- 分层阶段信息
- 流程信息
- 诊断信息

### 7.3 CapabilityGraph

面向能力域表达的核心图模型，包含：

- 能力节点
- 能力边
- 分组
- 流信息
- 技术标签
- 风险信号
- 证据包

### 7.4 ComponentGraph

面向 L3 下钻的组件级图模型，支持：

- 组件节点
- 组件关系边
- 阶段分组
- 与 capability 的映射关系
- 证据包透传

### 7.5 Evidence Package

当前节点和边可以携带统一证据包，典型字段包括：

- `facts`
- `rules`
- `conclusions`
- `sourceFiles`
- `symbols`
- `modules`
- `relationships`
- `confidenceLabel`
- `confidenceReason`

该机制是 SAVT“可解释分析”的关键基础。

## 8. 模块划分与目录说明

### 8.1 业务模块划分

| 目录 | 职责说明 |
| --- | --- |
| `apps/desktop` | 桌面应用入口、QML 资源与应用壳层 |
| `src/core` | 核心数据模型、架构图、能力图、组件图、项目配置 |
| `src/parser` | 语法解析器适配层，当前以 Tree-sitter C++ 为主 |
| `src/analyzer` | 项目扫描、语法分析、语义分析、图构建 |
| `src/layout` | 图布局、L2/L3 场景坐标与分组结果计算 |
| `src/ui` | 控制器、编排器、场景映射、报告服务、AST/AI 服务 |
| `src/ai` | AI 配置读取、请求构造、响应解析 |
| `config` | 项目默认配置与 AI 配置模板 |
| `scripts` | 构建、验证、环境与编码检查脚本 |
| `tests` | 自动化测试、夹具、黄金结果与性能基线 |
| `samples` | 小型人工样例工程 |
| `docs` | 设计说明、阶段记录、构建说明和路线文档 |

### 8.2 外部边界

`third_party` 下内容为第三方依赖或参考仓库，不属于项目自研业务代码边界。当前仓库明确要求将 `src/` 与 `apps/` 视为本项目代码域，将 `third_party/` 视为外部材料。

## 9. 技术选型

### 9.1 开发语言与框架

- C++20
- C11
- Qt 6.9
- Qt Quick / QML
- CMake 3.24+

### 9.2 关键依赖

- `tree-sitter`
- `tree-sitter-cpp`
- `Qt6::Core`
- `Qt6::Gui`
- `Qt6::Qml`
- `Qt6::Quick`
- `Qt6::QuickControls2`
- `Qt6::QuickDialogs2`
- `Qt6::Network`
- `Qt6::Concurrent`
- 可选 `libclang`

### 9.3 参考方向

从现有文档可知，项目在能力方向上参考了以下外部思路：

- Sourcetrail：作为产品和架构分层参考，不作为直接改造基础
- Codevis：作为语义抽取与模型分离参考
- ContextFlow：作为多视图架构阅读参考
- DocuEye：作为图文同步和证据面板参考

## 10. 构建与运行说明

### 10.1 基础要求

- CMake 3.24 或以上
- Qt 6.9.x
- 支持 C++20 的编译器
- 推荐使用 Ninja

### 10.2 推荐构建预设

仓库已提供跨平台 CMake Presets，主要包括：

- `macos-qt-debug`
- `macos-qt-release`
- `windows-msvc-qt-debug`
- `windows-msvc-qt-release`
- `windows-mingw-qt-debug`
- `windows-mingw-qt-release`

其中 Windows 平台推荐使用 MSVC 路径，MinGW 作为兼容路径保留。

### 10.3 关键环境变量

- `SAVT_QT_ROOT`
- `SAVT_LLVM_ROOT`
- `SAVT_MINGW_ROOT`

### 10.4 典型验证入口

仓库当前提供多种标准化验证脚本，例如：

- `scripts/dev/verify-macos-debug.sh`
- `scripts/dev/verify-macos-semantic-probe.sh`
- `scripts/dev/verify-windows-msvc-debug.bat`
- `scripts/dev/verify-windows-msvc-semantic-probe.bat`
- `scripts/dev/verify-mingw-debug.bat`

### 10.5 第三方依赖获取策略

首次配置时，如果 `third_party/` 下缺少 `tree-sitter` 或 `tree-sitter-cpp`，CMake 可以自动拉取相关依赖；如需离线构建，可关闭 `SAVT_FETCH_THIRD_PARTY` 并预先准备依赖目录。

## 11. 配置管理与工程规范

### 11.1 编码与文本规范

仓库当前明确以 UTF-8 作为源码与文本基线，并提供以下保障：

- `.editorconfig` 统一字符集、换行和尾随空白规范
- Windows / GCC 路径下通过编译选项强化 UTF-8 处理
- 提供 `check_utf8.ps1` 与 `savt_check_utf8` 检查入口

### 11.2 本地配置与保密信息

以下内容被视为本地环境内容，不应纳入共享版本控制结果：

- `config/*.local.json`
- `CMakeUserPresets.json`
- `CMakeLists.txt.user`
- Qt 生成文件与构建输出

其中 `config/deepseek-ai.local.json` 用于存放 AI 接口密钥，应始终保持本地化管理。

### 11.3 目录治理原则

项目结构设计强调以下原则：

- 解析逻辑与 Qt 展示层解耦
- UI 尽量保持薄控制层，核心分析逻辑通过服务或编排模块承载
- 图聚合与布局属于独立模块，不在渲染层内发明业务事实
- 第三方代码与自研代码严格隔离

## 12. 测试与质量保障

### 12.1 自动化测试目标

仓库当前包含以下测试目标：

- `savt_backend_tests`
- `savt_snapshot_tests`
- `savt_ai_tests`
- `savt_ui_tests`
- `savt_perf_smoke`
- `savt_utf8_check`

### 12.2 测试覆盖内容

当前测试体系主要覆盖：

- 后端分析与图模型逻辑
- 架构总览与能力图构建
- AI 配置与响应解析
- UI 服务层默认状态与数据构建
- 快照对比与黄金结果校验
- 增量缓存行为
- 性能基线与轻量压力验证

### 12.3 样例夹具与黄金基线

`tests/fixtures/precision` 与 `tests/golden/precision` 共同构成快照验证体系。根据仓库说明，当前已维护 21 个命名样例目录，覆盖以下场景：

- C++ 分层项目
- C++ 插件宿主
- Header-only 库
- Vendored dependency 场景
- JS 后端
- Python 工具型项目
- QML 混合项目
- Spring Boot 项目
- 语义成功与语义阻断场景

### 12.4 性能基线

仓库包含性能基线工具 `savt_perf_baseline`，支持默认 smoke 模式以及 `--full` 全量基线记录模式。现有文档已记录对代表性样例和 10k+ 文件合成样例的 cold/hot 分析基线，可用于后续性能回归对比。

## 13. 当前成熟度与已实现成果

结合当前仓库结构与阶段文档，SAVT 已具备以下可交付基础能力：

1. Qt/QML 桌面应用壳层与分析工作台。
2. 语法分析与可选语义分析双路径能力。
3. L1-L4 多层级架构展示框架。
4. L3 能力域组件下钻机制。
5. 统一证据包与检查面板展示能力。
6. 项目级配置系统。
7. 分层增量缓存能力。
8. 自动化测试、快照基线与性能基线。
9. 受约束的 AI 辅助解释能力。

整体上，项目已经不是单纯原型，而是具备明确分层结构、验证路径和持续演进能力的工程化分析工具。

## 14. 适用场景

SAVT 当前适用于以下典型场景：

- 新成员接手大型代码仓库时的结构理解
- 架构评审、技术汇报与答辩展示
- 重构前后的结构对比分析准备
- 多技术栈项目的整体容器视图梳理
- 代码尽调、技术债识别和模块边界研判
- 结合证据链进行架构结论追溯

## 15. 当前约束与后续方向

### 15.1 当前约束

- 工业级 C++ 语义精度仍依赖目标项目提供完整的编译数据库与可用 LLVM 环境。
- AI 能力依赖外部网络接口，不是离线本地模型。
- 当前语义分析重点仍在 C/C++ 路径，其它语言更多用于架构级清单与启发式建模。
- `third_party`、工具链目录和构建产物默认不纳入主分析视图。

### 15.2 后续方向

根据仓库已有路线文档，后续可继续推进的方向包括：

- 更高精度的语义成功样例扩展
- 更复杂 C++ 语言特性的语义覆盖
- 更完善的产品化导出能力
- 更系统的可视化交互增强
- 更强的跨项目、跨版本对比能力

## 16. 结论

SAVT 是一个以“源码事实驱动架构理解”为核心理念的软件架构分析与可视化项目。它以 Qt/C++ 为技术基础，以语法分析和语义分析双路径为能力支撑，以统一图模型为数据底座，以 L1-L4 多层视图、证据链展示、增量缓存和 AI 辅助解释为产品特征，已经形成了较完整的工程框架与演进路径。

从工程规范角度看，项目已经具备较清晰的模块边界、配置治理、构建预设、测试基线、性能验证和本地保密配置约束，可作为后续继续深化架构分析平台能力的可靠基础。

## 17. 建议参考文档

如需进一步展开阅读，建议优先参考以下仓库文档：

- `README.md`
- `docs/build-setup.md`
- `docs/project-structure.md`
- `docs/project-config.md`
- `docs/ai-assisted-analysis.md`
- `docs/industrial-precision-roadmap.md`
- `tests/README.md`
- `tests/fixtures/precision/CATALOG.md`
