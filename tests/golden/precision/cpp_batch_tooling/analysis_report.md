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
| 已发现源码文件 | 5 |
| 已成功解析文件 | 5 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **2 个** |
| 涉及源码文件 | **5 个** |
| 子模块总数 | **2 个** |
| 发起入口 | Tools |
| 后台支撑 | Domain |

---

## 模块详情

### tooling Tools

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 3 file(s), spans stage 0, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: tools. Example files: tools/export/ExportJob.cpp, tools/export/ExportJob.h, tools/main.cpp. Key symbols: ExportJob, main, run. Supporting roles inside this cluster: tooling. Direct collaborators: Domain. It is shown in the default view.

- 源文件：**3 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Domain

**核心类 / 符号：**
- `ExportJob`
- `main`
- `run`

**代表文件：**
- `tools/export/ExportJob.cpp`
- `tools/export/ExportJob.h`
- `tools/main.cpp`

---

### 💾 数据存储 (Database) Domain

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 2 个源码文件。
👉 核心类/函数: ReportStore, write

- 源文件：**2 个**
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Tools

**核心类 / 符号：**
- `ReportStore`
- `write`

**代表文件：**
- `domain/report/ReportStore.cpp`
- `domain/report/ReportStore.h`

---

