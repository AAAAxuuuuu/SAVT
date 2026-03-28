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
| 已成功解析文件 | 4 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **5 个** |
| 涉及源码文件 | **5 个** |
| 子模块总数 | **5 个** |
| 发起入口 | Cli |
| 后台支撑 | Data、Storage、Support |

---

## 模块详情

### tooling Cli

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0. It is reachable from an entry capability. Main modules: cli. Example files: cli/main.py. Key symbols: main. Supporting roles inside this cluster: tooling. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `main`

**代表文件：**
- `cli/main.py`

---

### analysis Services

**职责：** 负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。

负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: services. Example files: services/job_service.py. Key symbols: run_jobs. Supporting roles inside this cluster: analysis. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `run_jobs`

**代表文件：**
- `services/job_service.py`

---

### data Data

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: data. Example files: data/jobs.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `data/jobs.json`

---

### 💾 数据存储 (Database) Storage

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: load_jobs

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `load_jobs`

**代表文件：**
- `storage/job_store.py`

---

### tooling Support

**职责：** 构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。

构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: support. Example files: support/parsing.py. Key symbols: parse_name. Supporting roles inside this cluster: tooling. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `parse_name`

**代表文件：**
- `support/parsing.py`

---

