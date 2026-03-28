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
| 已发现源码文件 | 8 |
| 已成功解析文件 | 5 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **5 个** |
| 涉及源码文件 | **8 个** |
| 子模块总数 | **5 个** |
| 发起入口 | App |
| 后台支撑 | Backend Service、Data |

---

## 模块详情

### procedure_centric App

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 2 main flow(s). It is reachable from an entry capability. Main modules: app. Example files: app/main.cpp. Key symbols: main. Supporting roles inside this cluster: procedure_centric. Direct collaborators: Backend Controller. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Backend Controller

**核心类 / 符号：**
- `main`

**代表文件：**
- `app/main.cpp`

---

### interaction Backend Controller

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 2 file(s), spans stage 1, and appears in 2 main flow(s). It is reachable from an entry capability. Folder scope: Backend. Main modules: backend/controller. Example files: backend/controller/DashboardController.cpp, backend/controller/DashboardController.h. Key symbols: DashboardController, refresh. Supporting roles inside this cluster: interaction. Direct collaborators: App, Backend Service. It is shown in the default view.

- 源文件：**2 个**（`backend`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：App、Backend Service

**核心类 / 符号：**
- `DashboardController`
- `refresh`

**代表文件：**
- `backend/controller/DashboardController.cpp`
- `backend/controller/DashboardController.h`

---

### 🎨 界面展示 (UI) Ui

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 2 个源码文件。

- 源文件：**2 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `ui/Dashboard.qml`
- `ui/components/MetricCard.qml`

---

### interaction Backend Service

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 2 file(s), spans stage 2, and appears in 1 main flow(s). It is reachable from an entry capability. Folder scope: Backend. Main modules: backend/service. Example files: backend/service/MetricService.cpp, backend/service/MetricService.h. Key symbols: MetricService, load. Supporting roles inside this cluster: interaction. Direct collaborators: Backend Controller. It is shown in the default view.

- 源文件：**2 个**（`backend`）
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Backend Controller

**核心类 / 符号：**
- `MetricService`
- `load`

**代表文件：**
- `backend/service/MetricService.cpp`
- `backend/service/MetricService.h`

---

### data Data

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: data. Example files: data/metrics.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `data/metrics.json`

---

