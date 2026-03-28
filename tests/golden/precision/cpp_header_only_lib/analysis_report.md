# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 分析引擎状态

| 项目 | 值 |
| --- | --- |
| 当前精度 | 语法分析 |
| 主引擎 | tree-sitter |
| 语义状态 | 未请求 |
| 语义状态码 | `not_requested` |
| 编译数据库 | (none) |
| 已发现源码文件 | 4 |
| 已成功解析文件 | 4 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **3 个** |
| 涉及源码文件 | **4 个** |
| 子模块总数 | **3 个** |
| 发起入口 | App |
| 后台支撑 | 图数据管理 — Lib |

---

## 模块详情

### procedure_centric App

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 2 main flow(s). It is reachable from an entry capability. Main modules: app. Example files: app/main.cpp. Key symbols: main. Supporting roles inside this cluster: procedure_centric. Direct collaborators: Ui. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Ui

**核心类 / 符号：**
- `main`

**代表文件：**
- `app/main.cpp`

---

### 🧱 核心数据结构 图数据管理 — Ui

**职责：** 负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。

负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。 包含了 2 个源码文件。
👉 核心类/函数: GraphWindow, nodeCount

- 源文件：**2 个**
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：App、Lib

**核心类 / 符号：**
- `GraphWindow`
- `nodeCount`

**代表文件：**
- `ui/GraphWindow.cpp`
- `ui/GraphWindow.h`

---

### 🧱 核心数据结构 图数据管理 — Lib

**职责：** 负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。

负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。 包含了 1 个源码文件。
👉 核心类/函数: GraphSpec, GraphBuilder, build

- 源文件：**1 个**
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Ui

**核心类 / 符号：**
- `GraphSpec`
- `GraphBuilder`
- `build`

**代表文件：**
- `lib/graph/GraphBuilder.h`

---

