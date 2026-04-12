# SAVT Greenfield UI 设计蓝图

编制日期：2026-04-12  
设计原则：不沿用任何既有 UI 结构、页面命名、视觉风格、组件拆法或交互习惯  
保留事实：SAVT 是一个 Qt/QML 桌面端软件架构分析工具，能够提供项目分析、架构图、证据包、报告与受约束 AI 解释能力
视觉参考：`C:\Users\Axuuuu\Desktop\ui.html` 中的 Apple 风格浅灰工作台、半透明侧栏、点阵画布、节点卡片和右侧滑入 Inspector。

## 0. 北极星

SAVT 的新 UI 不是“图谱查看器”，也不是“IDE 的另一种皮肤”。

它应该是一个 **Architecture Investigation Desk**：用户把一个代码仓库放进来，SAVT 帮他形成调查线索、可信地图、证据链和下一步行动。

一句话：

> 从源码事实出发，把陌生项目变成可理解、可追溯、可行动的架构调查工作台。

核心体验链路：

```text
Start Case -> Trust Check -> System Brief -> Architecture Atlas
-> Focus Investigation -> Evidence Ledger -> Action Draft
```

这条链路完全不依赖旧 UI 的 L1/L2/L3/L4 前门，也不依赖旧的页面分区。

## 1. Product Framing

### 产品类型

桌面级工程分析与架构调查工具。

### 主要使用场景

- 接手陌生代码仓库。
- 理解 AI 生成项目。
- 重构前做结构调查。
- 技术评审前准备证据。
- 把分析结果转成可复制的报告或 AI 提示词。

### 成功标准

1. 用户 30 秒内知道项目大概是什么。
2. 用户 2 分钟内找到 1 个值得深入看的结构焦点。
3. 用户 5 分钟内能拿到证据支持的改造建议或评审问题。
4. 用户始终知道当前结论的可信度，不会把语法级推断误认为工业级语义结论。
5. UI 在 1440-1920 宽度桌面屏上保持高密度、稳定、专业。

### 非目标

- 不做完整 IDE。
- 不做源码编辑器。
- 不做泛用聊天机器人。
- 不做花哨展示大屏。
- 不把所有信息平铺给用户。

## 2. User Model

### Persona A：项目接管者

目标：尽快知道项目能不能改、该从哪里改。  
痛点：文件很多，结构未知，AI 生成代码边界不清。

JTBD：

> 当我拿到一个陌生项目时，我想知道系统的主要能力、入口和风险，这样我可以安全开始修改。

### Persona B：重构工程师

目标：找到模块边界、依赖路径和改造切口。  
痛点：图如果没有证据就无法信任，单靠搜索很慢。

JTBD：

> 当我准备重构某个能力时，我想看到它背后的组件、文件、符号和依赖证据，这样我能判断修改影响面。

### Persona C：架构评审者

目标：形成可以展示、讨论和追溯的分析结果。  
痛点：静态报告和图经常断开，结论很难回到证据。

JTBD：

> 当我做架构评审时，我想把地图、证据和结论组织成可交付材料，这样团队能围绕事实讨论。

## 3. Core Metaphor

新 UI 使用“调查桌”的心智模型。

```text
Case          当前被调查的项目
Signal        分析可信度、风险、阻断原因
Atlas         架构地图
Focus         当前调查对象
Ledger        证据账本
Draft         行动草稿：报告、建议、提示词
```

术语原则：

- 面向用户任务，不面向内部层级。
- 专业概念可以存在，但放在高级视角或详情里。
- 每个界面都回答一个问题：我现在能理解什么，下一步能做什么。

## 4. Information Architecture

```text
SAVT
├─ Case
│  ├─ Start Case
│  ├─ Trust Check
│  └─ System Brief
├─ Atlas
│  ├─ Capability Atlas
│  ├─ Dependency Paths
│  ├─ Risk Signals
│  └─ Search Results
├─ Investigation
│  ├─ Focus Board
│  ├─ Component Map
│  ├─ Source Evidence
│  ├─ Relationship Evidence
│  └─ Confidence Notes
├─ Draft
│  ├─ Ask SAVT
│  ├─ Refactor Plan
│  ├─ Review Questions
│  ├─ Prompt Pack
│  └─ Engineering Report
└─ Lab
   ├─ Raw Graph Views
   ├─ AST Preview
   ├─ Export Data
   └─ Settings
```

### Navigation Model

主导航只保留 5 个一级入口：

```text
Case | Atlas | Investigate | Draft | Lab
```

解释：

- `Case`：建立项目基本认知和可信度。
- `Atlas`：看全局结构地图。
- `Investigate`：围绕一个焦点深入调查。
- `Draft`：把理解转成行动材料。
- `Lab`：承接专业、原始、调试和高级视图。

这不是旧 UI 的重命名，而是新的任务分层。

## 5. Primary Layout

桌面主结构：

```text
+------------------------------------------------------------------------------------------------+
| Case: /repo/SAVT       Trust: Syntax inferred        Search case...      Analyze   Ask   Draft |
+----------------+-------------------------------------------------------------------------------+
| CASE           |                                                                               |
| Atlas          |  Workspace Header                                                             |
| Investigate    |  Current question: "What is this project made of?"                            |
| Draft          +-------------------------------------------------------------------------------+
| Lab            |                                                                               |
|                |                                                                               |
| Saved Focuses  |                                WORKSPACE                                      |
| Recent Cases   |                                                                               |
|                |                                                                               |
|                +--------------------------------------------------------------+----------------+
|                | Evidence Ledger / Action Draft / Files / Paths                | Focus Brief    |
+----------------+--------------------------------------------------------------+----------------+
```

Layout specs:

| Region | Size |
| --- | --- |
| Command bar | 52 px height |
| Left rail expanded | 196 px |
| Left rail collapsed | 52 px |
| Focus Brief rail | 320-360 px |
| Focus Brief collapsed | 56 px |
| Bottom ledger | 0 / 220 / 320 px |
| Minimum window | 1280 x 780 |
| Preferred window | 1600 x 940 |

Rules:

- 主工作区不能放在装饰性卡片里。
- 主图谱是工作台，不是预览。
- 不使用卡片套卡片。
- 卡片和按钮圆角不超过 8 px。
- 页面背景不要装饰光斑、渐变球或 bokeh。

## 6. Key Screens

### 6.1 Start Case

目标：选择项目、开始分析、解释接下来会发生什么。

```text
+------------------------------------------------------------------+
| Start a Case                                                     |
| Drop a project folder or choose a repository to investigate.      |
|                                                                  |
| [Choose Project] [Use current path]                               |
|                                                                  |
| What SAVT will produce                                            |
| - System brief                                                    |
| - Architecture atlas                                              |
| - Evidence ledger                                                 |
| - Action drafts                                                   |
+------------------------------------------------------------------+
```

状态：

- Empty：没有项目。
- Ready：项目已选择，等待分析。
- Running：展示阶段、进度、当前产物。
- Blocked：说明阻断原因和恢复动作。

### 6.2 Trust Check

目标：先建立可信度，不让用户误读分析精度。

```text
+----------------------------------------------------------------------------------+
| Trust Check                                                                       |
+----------------------------+-----------------------------+-----------------------+
| Analysis mode              | Evidence quality            | Required action       |
| Syntax inferred            | Medium                      | Add compile database  |
+----------------------------+-----------------------------+-----------------------+
| Why this matters                                                                 |
| Some relationships are inferred from syntax and may miss cross-translation-unit   |
| semantic edges.                                                                  |
+----------------------------------------------------------------------------------+
```

设计原则：Visibility of System Status。

### 6.3 System Brief

目标：让用户先用自然语言理解项目。

```text
+----------------------------------------------------------------------------------+
| System Brief                                                                      |
| This project is a desktop architecture analysis tool for source-driven mapping.    |
+-----------------------------------------+----------------------------------------+
| Main capabilities                       | Start investigation                    |
| Analyzer Pipeline                       | 1. Open Atlas                          |
| Graph Layout                            | 2. Inspect Analyzer Pipeline           |
| Desktop Workspace                       | 3. Draft refactor questions            |
+-----------------------------------------+----------------------------------------+
| Entry points                            | Risk signals                           |
| apps/desktop/src/main.cpp               | semantic backend unavailable           |
| src/ui/include/...                      | generated files skipped                |
+----------------------------------------------------------------------------------+
```

设计原则：Front-Door Thinking。

### 6.4 Architecture Atlas

目标：把项目变成可探索地图。

```text
+----------------------------------------------------------------------------------+
| Atlas     View: Capability  Density: Focused  Relations: Important  [Fit] [Trace] |
+----------------------------------------------------------------------------------+
|                                                                                  |
|       +----------------+          +----------------+                              |
|       | Analyzer       | -------> | Scene Mapping  |                              |
|       +----------------+          +----------------+                              |
|              |                            |                                       |
|              v                            v                                       |
|       +----------------+          +----------------+                              |
|       | Core Model     | <------> | Desktop App    |                              |
|       +----------------+          +----------------+                              |
|                                                                                  |
+---------------------------------------------------------------+------------------+
| Selected signal: Analyzer Pipeline                            | Focus Brief      |
| [Investigate] [Trace evidence] [Ask] [Add to draft]            | summary/actions  |
+---------------------------------------------------------------+------------------+
```

交互：

- 单击节点：设为 Focus。
- 双击节点：进入 Investigate。
- 单击边：解释关系。
- `F`：fit focus。
- `T`：trace dependency path。
- `Shift + click`：加入对比。
- `Esc`：退出临时状态。

### 6.5 Focus Board

目标：围绕一个对象做调查，而不是继续浏览全图。

```text
+----------------------------------------------------------------------------------+
| Investigate: Analyzer Pipeline                         Back to Atlas  Add Draft   |
+----------------------------------------------------------------------------------+
| Question                                                                           |
| "What parts implement analysis and what risks affect refactoring?"                 |
+----------------------------------------------------------------------------------+
| Component Map                                                                      |
| FileScanner -> Parser -> GraphBuilder -> SceneMapper                               |
|                       \-> Diagnostics                                              |
+---------------------------------------------------------------+------------------+
| Ledger tabs: Evidence | Files | Paths | Confidence | AI Notes    | Focus Brief    |
+---------------------------------------------------------------+------------------+
```

设计原则：Progressive Disclosure。

### 6.6 Evidence Ledger

目标：所有结论都能回到证据。

```text
+----------------------------------------------------------------------------------+
| Evidence Ledger: Analyzer Pipeline                                      [Copy]    |
+----------------------------------------------------------------------------------+
| Facts                    | Rules                    | Conclusions                 |
| - source files           | - module merge           | - central analysis role     |
| - symbols                | - dependency fold        | - review before refactor    |
+----------------------------------------------------------------------------------+
| Source Files                                                                      |
| src/analyzer/src/AnalysisGraphBuilder.cpp                                         |
| src/ui/src/SceneMapper.cpp                                                        |
+----------------------------------------------------------------------------------+
```

设计原则：Recognition over Recall。

### 6.7 Draft Studio

目标：把调查结果变成可用文本。

```text
+----------------------------------------------------------------------------------+
| Draft Studio                                                                       |
| Preset: Refactor plan / Review questions / Prompt pack / Report                    |
+----------------------------------------------------------------------------------+
| Context                                                                            |
| Focus: Analyzer Pipeline                                                           |
| Evidence: 12 facts, 5 files, medium confidence                                     |
+----------------------------------------------------------------------------------+
| Draft                                                                              |
| - Suggested next actions                                                           |
| - Risks to verify                                                                  |
| - Prompt for coding agent                                                          |
+----------------------------------------------------------------------------------+
| [Copy] [Send to Ask SAVT] [Append to report]                                       |
+----------------------------------------------------------------------------------+
```

设计原则：把理解变成行动。

## 7. Visual System

方向名：`Forensic Workbench Light`

### 不要的视觉方向

- 不要延续旧浅蓝卡片感。
- 不要大面积深蓝、蓝灰、紫蓝渐变。
- 不要米色、咖啡色、棕橙主题。
- 不要装饰性光斑、球、模糊背景。
- 不要把页面做成网页 landing page。
- 不要卡片套卡片。

### 视觉关键词

- 明亮
- 精确
- 冷静
- 有证据感
- 多色信号而非单色品牌

### QML-ready Tokens

```qml
// Fonts
readonly property string displayFontFamily: "Segoe UI Variable Display, Microsoft YaHei UI, PingFang SC"
readonly property string textFontFamily: "Segoe UI Variable Text, Microsoft YaHei UI, PingFang SC"
readonly property string monoFontFamily: "Cascadia Code, IBM Plex Mono, Consolas"

// Neutral surfaces
readonly property color base0: "#FCFCFD"
readonly property color base1: "#F6F7F9"
readonly property color base2: "#ECEFF3"
readonly property color base3: "#D7DDE5"
readonly property color border1: "#D4DAE2"
readonly property color border2: "#AEB7C2"

// Text
readonly property color text1: "#121416"
readonly property color text2: "#363D45"
readonly property color text3: "#66717D"
readonly property color textInverse: "#FFFFFF"

// Signals
readonly property color signalTeal: "#0F766E"
readonly property color signalTealSoft: "#DDF4EF"
readonly property color signalCobalt: "#2563EB"
readonly property color signalCobaltSoft: "#E8F0FF"
readonly property color signalAmber: "#B7791F"
readonly property color signalAmberSoft: "#FFF4D8"
readonly property color signalCoral: "#C2413A"
readonly property color signalCoralSoft: "#FCE8E6"
readonly property color signalRaspberry: "#B4236D"
readonly property color signalRaspberrySoft: "#FBE7F0"
readonly property color signalMoss: "#4D7C0F"
readonly property color signalMossSoft: "#EAF6D8"

// Graph
readonly property color graphCanvas: "#FAFBFC"
readonly property color graphGrid: "#E5E9EE"
readonly property color graphEdge: "#8793A1"
readonly property color graphEdgeFocus: "#121416"

// Layout
readonly property int radius2: 2
readonly property int radius4: 4
readonly property int radius6: 6
readonly property int radius8: 8

readonly property int space4: 4
readonly property int space8: 8
readonly property int space12: 12
readonly property int space16: 16
readonly property int space20: 20
readonly property int space24: 24
readonly property int space32: 32
```

### Typography

| Role | Size / Line | Weight |
| --- | --- | --- |
| Page title | 24 / 30 | 600 |
| Workspace question | 18 / 24 | 600 |
| Section title | 14 / 20 | 600 |
| Body | 13 / 19 | 400 |
| Dense body | 12 / 17 | 400 |
| Metadata | 11 / 15 | 500 |
| Code path | 12 / 17 | 400 mono |

Rules:

- 不使用负字距。
- 不按 viewport width 缩放字号。
- 长路径可换行或中部省略。
- 状态标签使用文字 + 色彩 + 形状，不只靠颜色。

## 8. Interaction Model

### Global

| Shortcut | Action |
| --- | --- |
| `Ctrl+1` | Case |
| `Ctrl+2` | Atlas |
| `Ctrl+3` | Investigate |
| `Ctrl+4` | Draft |
| `Ctrl+K` | Command search |
| `Ctrl+Shift+A` | Ask SAVT |
| `Esc` | Close overlay / clear temporary focus |

### Atlas

| Input | Result |
| --- | --- |
| Click node | Set focus |
| Double click node | Investigate focus |
| Click edge | Explain relationship |
| Shift click | Add to compare set |
| Drag empty area | Pan |
| Wheel | Zoom at cursor |
| Right click | Context menu |

### Feedback Timings

- Hover highlight: 80-120 ms.
- Focus rail update: 120-160 ms.
- Drawer open/close: 160-220 ms.
- Analysis phase update: immediate text update, progress animation no longer than 300 ms.

## 9. Component Architecture

全新 QML 组件命名，不沿用旧 UI 命名。

```text
apps/desktop/qml/greenfield
├─ MainSurface.qml
├─ foundation
│  ├─ Tokens.qml
│  ├─ Palette.qml
│  └─ TypeScale.qml
├─ shell
│  ├─ DeskShell.qml
│  ├─ CommandStrip.qml
│  ├─ CaseRail.qml
│  ├─ WorkspaceStage.qml
│  ├─ FocusBrief.qml
│  └─ LedgerTray.qml
├─ case
│  ├─ StartCaseView.qml
│  ├─ TrustCheckView.qml
│  └─ SystemBriefView.qml
├─ atlas
│  ├─ AtlasView.qml
│  ├─ AtlasToolbar.qml
│  ├─ AtlasViewport.qml
│  ├─ AtlasNode.qml
│  ├─ AtlasEdgeLayer.qml
│  └─ DependencyTraceOverlay.qml
├─ investigation
│  ├─ FocusBoard.qml
│  ├─ ComponentMap.qml
│  ├─ ComponentNode.qml
│  └─ FocusQuestionBar.qml
├─ ledger
│  ├─ EvidenceLedger.qml
│  ├─ FactColumn.qml
│  ├─ SourceList.qml
│  └─ ConfidenceNote.qml
├─ draft
│  ├─ DraftStudio.qml
│  ├─ AskPanel.qml
│  ├─ PromptPack.qml
│  └─ ReportComposer.qml
└─ state
   ├─ CaseState.qml
   ├─ FocusState.qml
   ├─ AtlasState.qml
   └─ DraftState.qml
```

### State Ownership

```text
CaseState
  project path, analysis lifecycle presentation, trust summary

FocusState
  focused node, focused edge, compare set, breadcrumb

AtlasState
  zoom, pan, relation filter, density, search result

DraftState
  selected preset, generated text, copied/exported state
```

UI state 不拥有分析事实；它只保存用户当前如何看这些事实。

## 10. Accessibility

要求：

- 所有可点击区域最小 28 px，高频按钮 32 px 以上。
- Focus ring 为 2 px，不能只靠阴影。
- 图节点类型使用颜色 + 形状 + 标签。
- 语义阻断、错误和风险必须提供文字原因。
- 主要流程可键盘完成。
- 状态颜色必须满足 WCAG 2.1 AA 对比。

Graph encoding:

| Meaning | Color | Secondary cue |
| --- | --- | --- |
| Capability | Teal | rounded rect + domain label |
| Component | Cobalt | rect + module marker |
| Infrastructure | Amber | dashed border |
| Risk | Coral | warning badge |
| AI draft | Raspberry | draft badge |
| External/support | Moss | outline marker |

## 11. Empty / Loading / Error States

### Empty

```text
No case is open.
Choose a project folder to start an architecture investigation.
```

### Loading

```text
Analyzing project...
Current phase: building architecture graph
Expected outputs: system brief, atlas, evidence ledger
```

### Blocked

```text
Semantic analysis is blocked.
Reason: compile_commands.json was not found.
Impact: call relationships may be syntax-inferred.
Action: add compilation database or continue with syntax-level investigation.
```

### Partial

```text
Atlas is available, but semantic confidence is limited.
Use Evidence Ledger before making refactor decisions.
```

## 12. Usability Heuristic Check

| Heuristic | Greenfield response |
| --- | --- |
| Visibility of system status | Trust Check is first-class |
| Match with real world | Case, Atlas, Investigation, Ledger, Draft |
| User control | Back to Atlas, clear focus, collapse rails |
| Consistency | Every focus object has brief, ledger, draft |
| Error prevention | Blocked semantic states are explicit |
| Recognition | Breadcrumb, focus question, saved focuses |
| Efficiency | Shortcuts and command search |
| Minimalism | No decorative background, no nested cards |
| Error recovery | Each blocked state includes action |
| Help | Draft Studio turns evidence into guidance |

## 13. Implementation Stance

这次重构按“替换产品体验”来做，不按“渐进修补旧 UI”来做。

实施原则：

1. 新建 `apps/desktop/qml/greenfield`。
2. 不读取旧 QML 的组件边界作为设计依据。
3. 第一版只接入必要 backend 数据：项目路径、分析状态、系统摘要、图节点边、证据、AI 输出。
4. 当 greenfield shell 能启动后，`Main.qml` 直接挂载 `greenfield/MainSurface.qml`。
5. 旧 UI 文件只作为未引用文件暂存，等 greenfield 跑通后整体清理。

第一批落地文件：

```text
apps/desktop/qml/greenfield/MainSurface.qml
apps/desktop/qml/greenfield/foundation/Tokens.qml
apps/desktop/qml/greenfield/shell/DeskShell.qml
apps/desktop/qml/greenfield/shell/CommandStrip.qml
apps/desktop/qml/greenfield/shell/CaseRail.qml
apps/desktop/qml/greenfield/shell/WorkspaceStage.qml
apps/desktop/qml/greenfield/case/StartCaseView.qml
apps/desktop/qml/greenfield/case/TrustCheckView.qml
apps/desktop/qml/greenfield/case/SystemBriefView.qml
apps/desktop/qml/greenfield/state/CaseState.qml
apps/desktop/qml/greenfield/state/FocusState.qml
```

首个验收点：

- 程序启动进入全新 shell。
- 没有任何旧 UI 页面被引用。
- 能选择项目并启动分析。
- 能看到 Start Case、Trust Check、System Brief。
- 视觉上没有旧浅蓝卡片风格、没有装饰光斑、没有卡片套卡片。

## 14. Open Product Questions

1. `Draft` 第一版优先做 Prompt Pack，还是优先做 Review Questions？
2. `Lab` 是否应该默认隐藏，还是一直作为专业入口显示？
3. `Atlas` 第一版是否允许直接显示所有关系，还是只显示重要关系？
4. `Focus Board` 是否需要保存多个调查对象形成 side-by-side 对比？
5. 报告输出是否需要可编辑，还是先保持只读生成稿？

## 15. Next Step

从 `greenfield/MainSurface.qml` 开始实现。不要改旧页面，不迁移旧组件，不复用旧命名。第一阶段只做新 shell、Case 入口、Trust Check 和 System Brief。
