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
| 已发现源码文件 | 7 |
| 已成功解析文件 | 7 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **4 个** |
| 涉及源码文件 | **7 个** |
| 子模块总数 | **4 个** |
| 发起入口 | App |
| 后台支撑 | Core Storage |

---

## 模块详情

### procedure_centric App

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 3 main flow(s). It is reachable from an entry capability. Main modules: app. Example files: app/main.cpp. Key symbols: main. Supporting roles inside this cluster: procedure_centric. Direct collaborators: Interaction. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Interaction

**核心类 / 符号：**
- `main`

**代表文件：**
- `app/main.cpp`

---

### 🌐 网络通信 (Network) Interaction

**职责：** 负责与外部服务器、大模型或第三方接口进行数据交互。

负责与外部服务器、大模型或第三方接口进行数据交互。 包含了 2 个源码文件。
👉 核心类/函数: ApiController, handle

- 源文件：**2 个**
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：App、Core Service

**核心类 / 符号：**
- `ApiController`
- `handle`

**代表文件：**
- `interaction/http/ApiController.cpp`
- `interaction/http/ApiController.h`

---

### 🧠 核心算法 (Core Engine) Core Service

**职责：** 项目的心脏，负责最核心的业务计算、数据分析或逻辑处理。

项目的心脏，负责最核心的业务计算、数据分析或逻辑处理。 包含了 2 个源码文件。
👉 核心类/函数: ScheduleService, load

- 源文件：**2 个**（`core`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Core Storage、Interaction

**核心类 / 符号：**
- `ScheduleService`
- `load`

**代表文件：**
- `core/service/ScheduleService.cpp`
- `core/service/ScheduleService.h`

---

### 💾 数据存储 (Database) Core Storage

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 2 个源码文件。
👉 核心类/函数: ScheduleRepository, fetch

- 源文件：**2 个**（`core`）
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Core Service

**核心类 / 符号：**
- `ScheduleRepository`
- `fetch`

**代表文件：**
- `core/storage/ScheduleRepository.cpp`
- `core/storage/ScheduleRepository.h`

---

