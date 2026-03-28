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
| 已发现源码文件 | 6 |
| 已成功解析文件 | 5 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **5 个** |
| 涉及源码文件 | **6 个** |
| 子模块总数 | **5 个** |
| 发起入口 | Src Main Bootstrap |
| 后台支撑 | Src Main Model、(root) |

---

## 模块详情

### type_centric Src Main Bootstrap

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0. It is reachable from an entry capability. Folder scope: Src. Main modules: src/main/bootstrap. Example files: src/main/java/com/example/schedule/ScheduleApplication.java. Key symbols: ScheduleApplication. Supporting roles inside this cluster: type_centric. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `ScheduleApplication`

**代表文件：**
- `src/main/java/com/example/schedule/ScheduleApplication.java`

---

### analysis Src Main Service

**职责：** 负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。

负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。 Covers 1 module(s) and 2 file(s), spans stage 1, and appears in 2 main flow(s). Folder scope: Src. Main modules: src/main/service. Example files: src/main/java/com/example/schedule/service/GradeService.java, src/main/java/com/example/schedule/service/LoginService.java. Key symbols: GradeService, LoginService. Supporting roles inside this cluster: analysis. Direct collaborators: Src Main Controller, Src Main Model. It is shown in the default view.

- 源文件：**2 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Main Controller、Src Main Model

**核心类 / 符号：**
- `GradeService`
- `LoginService`

**代表文件：**
- `src/main/java/com/example/schedule/service/GradeService.java`
- `src/main/java/com/example/schedule/service/LoginService.java`

---

### interaction Src Main Controller

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 2 main flow(s). Folder scope: Src. Main modules: src/main/controller. Example files: src/main/java/com/example/schedule/controller/ScheduleController.java. Key symbols: ScheduleController. Supporting roles inside this cluster: interaction. Direct collaborators: Src Main Service. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Src Main Service

**核心类 / 符号：**
- `ScheduleController`

**代表文件：**
- `src/main/java/com/example/schedule/controller/ScheduleController.java`

---

### core_model Src Main Model

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 1 file(s), spans stage 2, and appears in 1 main flow(s). Folder scope: Src. Main modules: src/main/model. Example files: src/main/java/com/example/schedule/model/Course.java. Key symbols: Course. Supporting roles inside this cluster: core_model. Direct collaborators: Src Main Service. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Src Main Service

**核心类 / 符号：**
- `Course`

**代表文件：**
- `src/main/java/com/example/schedule/model/Course.java`

---

### type_centric (root)

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: (root). Example files: pom.xml. Key symbols: none. Supporting roles inside this cluster: type_centric. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `pom.xml`

---

