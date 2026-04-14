# SAVT 仓库深度分析报告

编制日期：2026-04-13  
分析范围：`src/`、`apps/desktop/`、`tests/`、`config/`、`docs/`、`scripts/` 与 `samples/`  
排除范围：`.git/`、`build/`、`third_party/`、`tools/`、Qt/CMake 生成物与本地密钥配置  
当前最近提交：`87828d6 feat(desktop): 优化架构聚焦交互与组件探测可读性`

## 1. 总体结论

SAVT 是一个以 Qt 6、QML 和现代 C++ 构建的软件架构分析与可视化桌面系统。项目目标不是做一个通用 IDE，而是把源码事实转化为架构图、能力图、组件图、报告和证据链，帮助开发者在接手、评审、重构和讲解大型项目时快速建立系统级理解。

从当前仓库状态看，SAVT 已经具备比较完整的工程骨架。它包含独立的核心模型层、项目分析层、图布局层、UI 编排层、桌面 QML 层、AI 辅助解释层、项目级配置、快照测试、性能基线和多平台构建脚本。项目的主要亮点在于它明确区分语法级分析与语义级分析，并在语义条件不满足时输出具体阻断原因，而不是把启发式结果包装成“工业级精度”。

当前最值得继续打磨的方向有三个。第一，语义分析能力已经具备 libclang 路径，但工业级精度仍依赖目标项目提供 `compile_commands.json`、LLVM/Clang 环境和可解析的系统头文件。第二，桌面端正在从旧 QML 组件体系转向 `greenfield` 工作台，当前仓库中仍保留大量未在 `apps/desktop/CMakeLists.txt` 注册的历史 QML 文件，需要明确归档、注册或清理策略。第三，AI 能力已经限定为证据包内解释，但若用于比赛提交，需要在作品说明中明确数据边界、接口边界和 AI 使用声明。

## 2. 仓库规模与语言构成

本次扫描统计了自研源代码、测试、配置、脚本、样例和文档文件，不把 `third_party/` 和 `build/` 计入项目自研规模。当前有效扫描范围内共有 179 个文件，其中 QML 文件 68 个、C++ 源文件 35 个、C/C++ 头文件 29 个、Markdown 文档 25 个、批处理脚本 7 个、JSON 配置 3 个、Shell 脚本 2 个、JavaScript 文件 2 个、PowerShell 脚本 1 个。

按主要工程语言统计，C/C++ 相关文件 64 个，约 19184 行；QML 与 JavaScript 文件 70 个，约 12200 行；C++ 自动化测试文件 5 个，约 2674 行；仓库主要 Markdown 文档 27 个，约 3164 行。这个规模说明项目已经超出一次性演示原型，进入了需要模块边界、回归基线和文档治理的阶段。

## 3. 顶层目录结构

```text
SAVT
|-- apps/desktop          Qt/QML 桌面应用入口与工作台界面
|-- config                项目分析配置与 AI 配置模板
|-- docs                  架构说明、阶段验收、构建与工具链文档
|-- samples               小型手工样例
|-- scripts/dev           构建、验证、环境准备、编码检查脚本
|-- src/ai                AI 配置、请求构造、响应解析
|-- src/analyzer          项目扫描、语法分析、语义分析、图构建
|-- src/core              分析报告、架构图、能力图、组件图、项目配置模型
|-- src/layout            L2/L3 图布局算法
|-- src/parser            Tree-sitter C++ 解析适配层
|-- src/ui                Qt 控制器、编排器、服务层与 scene 映射
|-- tests                 单元测试、快照测试、性能基线、黄金结果
|-- third_party           第三方依赖，按项目规范不视为自研代码
```

仓库自己的结构规则已经写入 `docs/project-structure.md`：解析逻辑保持独立于 Qt，UI 层保持轻控制，图聚合和布局放在专门模块中，第三方代码不能混入项目逻辑。实际目录与这一规则基本一致。

## 4. 技术栈与构建体系

项目使用 C++20 和 C11 作为核心实现语言，使用 Qt 6.9 的 Core、Gui、Qml、Quick、QuickControls2、QuickDialogs2、Network 和 Concurrent 模块完成桌面应用、异步任务、网络请求和 QML 集成。构建系统基于 CMake 3.24+，并通过 `CMakePresets.json` 提供 macOS、Windows MSVC 和 Windows MinGW 兼容路径。仓库默认导出 `compile_commands.json`，这对于后续语义分析和开发工具链都很关键。

第三方解析依赖以 `tree-sitter` 和 `tree-sitter-cpp` 为主。CMake 在 `third_party/` 不完整时可以通过 `FetchContent` 拉取指定版本，也支持关闭 `SAVT_FETCH_THIRD_PARTY` 走离线预置依赖。语义分析路径由 `SAVT_ENABLE_CLANG_TOOLING` 控制，启用后会查找 LLVM/Clang 的头文件和 libclang 库，并把可用性状态写入编译定义。

仓库对 Windows 构建做了较多工程化处理。顶层 `CMakeLists.txt` 会在 Windows 平台上查找 MSVC `cl.exe`、Windows SDK `rc.exe` 和 `mt.exe`，同时对 MSVC 与 GCC 系编译器配置 UTF-8 源码选项。`.gitignore` 明确排除了 `config/*.local.json`、`CMakeUserPresets.json`、构建产物、工具目录和第三方目录，密钥与本地路径治理较清晰。

## 5. 核心架构

SAVT 的架构主线可以概括为“扫描源码，抽取事实，聚合架构，计算布局，映射场景，展示与解释”。一次典型分析从桌面 UI 触发，经 `AnalysisController` 和 `AnalysisOrchestrator` 进入 `IncrementalAnalysisPipeline`，再由 `CppProjectAnalyzer` 选择语法或语义分析路径，生成 `AnalysisReport`。随后 `core` 层构建 `ArchitectureOverview`、`CapabilityGraph`、`ComponentGraph` 与规则报告，`layout` 层计算 L2/L3 场景坐标，`SceneMapper` 把 C++ 图模型转换为 QML 可消费的 `QVariantMap` 与列表。

```text
QML Desktop
  -> AnalysisController
  -> AnalysisOrchestrator / IncrementalAnalysisPipeline
  -> CppProjectAnalyzer
  -> SyntaxProjectAnalyzer 或 SemanticProjectAnalyzer
  -> AnalysisReport
  -> ArchitectureOverview / CapabilityGraph / ComponentGraph
  -> LayeredGraphLayout
  -> SceneMapper
  -> L1-L4 可视化、报告、AST 预览、AI 解释
```

这一结构的优点是分析事实不依赖 UI 渲染，布局算法不发明业务语义，AI 也不直接代替静态分析结论。对竞赛材料而言，这条链路可以写成“源码事实驱动的架构理解闭环”。

## 6. 模块职责

`src/core` 是项目的事实模型与领域模型中心。`ArchitectureGraph.h` 定义 `SymbolNode`、`SymbolEdge`、`AnalysisReport`、`SymbolKind`、`EdgeKind` 与 `FactSource`，表达源码符号、关系边和语义来源。`CapabilityGraph.h` 在架构总览之上组织能力节点、能力边、流程、分组、技术标签、风险信号和证据包。`ComponentGraph.h` 则支持对某个能力域进行 L3 组件下钻。

`src/analyzer` 是项目的源码理解层。`CppProjectAnalyzer` 先构建扫描清单与元数据指纹，再根据 `AnalyzerPrecision` 决定执行 `SyntaxOnly`、`SemanticPreferred` 或 `SemanticRequired`。如果语义模式被请求，系统会探测 `compile_commands.json`、libclang 可用性和运行时解析结果，并把 `missing_compile_commands`、`backend_unavailable`、`llvm_not_found`、`system_headers_unresolved` 等状态写入报告。若是语义优先模式，失败后可回退到 Tree-sitter 语法路径；若是语义必需模式，则直接生成阻断报告。

`src/layout` 负责把架构图和组件图转为可读的平面布局。`LayeredGraphLayout` 支持模块布局、能力场景布局和组件场景布局。桌面端的 `ArchitectureCanvas.qml` 又在此基础上实现缩放、平移、节点聚焦、关系聚焦和组件全景分区。

`src/ui` 是 Qt 服务与编排层。`AnalysisController` 暴露大量 `Q_PROPERTY` 和 `Q_INVOKABLE` 给 QML，包括项目路径、分析状态、报告文本、AST 文件列表、能力场景、组件场景、AI 状态和复制操作。`IncrementalAnalysisPipeline` 提供扫描、解析、聚合、布局四层同进程缓存。`ReportService`、`ReadingGuideService`、`SemanticReadinessService`、`AstPreviewService` 和 `SceneMapper` 则把底层结果转成用户可读内容。

`src/ai` 与 `src/ui/AiService` 形成受约束的 AI 辅助解释路径。它读取 `config/deepseek-ai.local.json` 或模板配置，构造 Chat Completions 风格请求，要求模型只处理 SAVT 架构阅读任务，并只使用系统提供的证据包。该能力适合作为“辅助解释层”，不应被描述为静态分析事实本身。

## 7. 关键数据模型

`AnalysisReport` 是底层分析结果载体，记录项目根路径、主分析引擎、精度级别、语义状态码、编译数据库路径、发现文件数、解析文件数、符号节点、关系边和诊断信息。`FactSource` 明确区分 `Inferred` 与 `Semantic`，这使文档能够解释“哪些结论来自语义后端，哪些结论是启发式推断”。

`ArchitectureOverview` 是面向整体架构的中间表示，负责把底层符号和文件关系聚合成模块、依赖、分组和流程。`CapabilityGraph` 面向 L2 能力域视图，节点里包含角色、职责、摘要、示例文件、协作者、技术标签、风险信号、可见性、优先级和证据包。`ComponentGraph` 面向 L3 组件下钻，围绕单个能力域构建组件节点、协调边和阶段分组。

证据包是 SAVT 叙事中最有价值的模型之一。每个能力节点或关系边都可以携带事实、规则、结论、来源文件、符号、模块、关系、置信标签和置信原因。它使“图上为什么出现这个节点”和“这条边为什么成立”可以被追溯，这比单纯画出一张漂亮关系图更接近工程评审需要。

## 8. 已实现功能

当前后端已经能够递归扫描项目，识别 C/C++、QML、JavaScript、TypeScript、Python、Java、HTML、CSS、JSON、CMake 等架构相关文件，并过滤构建目录、第三方目录、缓存目录和生成文件。C/C++ 语法分析基于 Tree-sitter，已支持命名空间、类、结构体、函数、方法、包含关系和初步调用关系抽取。语义路径基于 libclang，在条件满足时能够支持编译数据库驱动的真实编译上下文分析、符号身份识别和跨翻译单元合并。

桌面 UI 当前采用 `greenfield` 工作台作为主入口，`apps/desktop/CMakeLists.txt` 注册了 `Main.qml` 与 12 个 `greenfield` QML 文件。界面包含项目选择、命令条、架构全景图、组件探测、配置页、报告页、语义状态页、AST 预览和 AI 解释入口。主程序 `apps/desktop/src/main.cpp` 设置 Basic 风格，创建 `AnalysisController`，允许通过命令行参数预设项目根路径，并把控制器注入 QML 上下文。

报告与导出能力已经具备基础形态。`core` 层能格式化分析报告、序列化 JSON、输出 DOT；`ui` 层能生成系统上下文、Markdown 报告和适合复制给 AI 的上下文片段。这个能力链可直接支撑比赛提交材料中的“系统设计”“实现说明”“测试结果”和“应用演示脚本”。

## 9. 增量缓存与性能基线

`IncrementalAnalysisPipeline` 将一次分析拆成扫描、解析、聚合和布局四层缓存。缓存键绑定项目根路径、分析选项、项目配置签名、编译数据库签名、源码元数据指纹和缓存版本。源码内容、项目配置、`compile_commands.json` 或精度模式变化后，会触发对应层及下游层失效。

阶段文档记录的性能基线显示，在 `synthetic_large_10000` 这个 10001 文件合成样例中，冷启动总耗时约 4519.41ms，热缓存总耗时约 303.01ms，扫描、解析、聚合、布局四层均命中，峰值内存约 75.59MB。对于比赛材料，这组数据可以作为“系统具备面向中大型仓库的增量分析潜力”的工程证据。

## 10. 测试与质量保障

测试体系包含 `savt_backend_tests`、`savt_snapshot_tests`、`savt_ai_tests`、`savt_ui_tests`、`savt_perf_smoke` 和 Windows 下的 `savt_utf8_check`。`tests/fixtures/precision` 与 `tests/golden/precision` 当前各有 21 个命名样例，覆盖 C++ 分层项目、插件宿主、Header-only 库、vendored dependency、Node 后端、Python 工具项目、QML 混合项目、Spring Boot 项目，以及语义成功和语义阻断场景。

快照测试不仅比较 JSON、DOT、Markdown、能力图和布局文本，还包含针对代表性样例的关键子串审计，防止重要事实在大型 golden diff 中被悄悄抹掉。UI 服务层测试覆盖 AST 预览、报告服务、控制器默认状态和增量缓存命中/失效。性能基线工具能输出发现文件数、解析文件数、overview 节点、capability 节点、耗时和峰值内存。

本次分析没有重新执行完整构建和测试，因为任务目标是文档生成，且当前 Windows 环境的 Qt/LLVM 路径未在本轮验证中重新配置。文档中引用的测试结论来自仓库现有阶段验收记录和测试配置。

## 11. 技术债与风险

第一类风险来自语义精度依赖。SAVT 的工业级 C++ 分析需要有效的 `compile_commands.json`、可用的 LLVM/Clang、系统头文件解析能力和更大规模的语义样例。当前代码已经能明确报告阻断原因，但参赛演示时需要准备一个语义路径可跑通的样例工程，或者在材料中诚实区分语法演示和语义演示。

第二类风险来自 QML 资源治理。当前 `apps/desktop/qml` 下共有 68 个 QML 文件，但 `apps/desktop/CMakeLists.txt` 只注册了 13 个主工作台文件，另有 55 个历史 QML 文件未注册。它们可能是保留的旧界面、迁移中的组件或待清理资源。参赛文档应以已注册的 `greenfield` 工作台为当前实现口径，并把旧 QML 文件列为后续治理项。

第三类风险来自 AI 能力边界。SAVT 的 AI 解释依赖外部 API 与本地密钥配置。比赛环境中如果无网络或不允许调用外部模型，AI 卡片应作为可选功能处理，核心演示必须依赖静态分析、图模型、报告与 AST 预览完成闭环。

第四类风险来自多语言分析深度。项目已能识别多种语言的架构相关文件，并通过启发式方式构建跨技术栈视图，但语义级深度当前集中在 C/C++ 路径。文档中不宜夸大 JavaScript、Python、Java 的语义精度，应表述为“架构级清单与启发式聚合”。

## 12. 参赛材料可提炼的核心卖点

SAVT 最适合作为“软件应用与开发/算法设计与应用”方向的作品申报。它的应用痛点是中大型代码仓库难以快速理解，技术路线是静态分析、语义诊断、图模型聚合、分层布局、证据链和受约束 AI 解释，最终成果是可交互的桌面架构工作台。

参赛叙事应避免把作品写成单纯“可视化界面”。更强的叙事是：SAVT 面向软件工程中的架构理解问题，建立了从源码事实到架构认知的可追溯链路。它能告诉用户“看到了什么”“为什么这样判断”“精度是否足够”“下一步该从哪里读”。这正是与普通文件浏览器、关系图绘制器和通用聊天机器人区分开的地方。

