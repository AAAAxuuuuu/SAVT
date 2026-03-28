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
| 后台支撑 | Src Jobs、Src Worker、Src Stores、(root) |

---

## 模块详情

### 🎨 界面展示 (UI) Src Jobs

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 1 个源码文件。
👉 核心类/函数: run

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Services、Src Worker

**核心类 / 符号：**
- `run`

**代表文件：**
- `src/jobs/rebuildIndexJob.js`

---

### analysis Src Services

**职责：** 负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。

负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。 Covers 1 module(s) and 1 file(s), spans stage 2, and appears in 2 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/services. Example files: src/services/indexService.js. Key symbols: refreshIndex. Supporting roles inside this cluster: analysis. Direct collaborators: Src Jobs, Src Stores. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Jobs、Src Stores

**核心类 / 符号：**
- `refreshIndex`

**代表文件：**
- `src/services/indexService.js`

---

### tooling Src Worker

**职责：** 构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。

构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 3 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/worker. Example files: src/worker.js. Key symbols: bootstrapWorker. Supporting roles inside this cluster: tooling. Direct collaborators: Src Jobs. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Src Jobs

**核心类 / 符号：**
- `bootstrapWorker`

**代表文件：**
- `src/worker.js`

---

### tooling Src Stores

**职责：** 构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。

构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。 Covers 1 module(s) and 1 file(s), spans stage 3, and appears in 1 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/stores. Example files: src/stores/searchStore.js. Key symbols: writeIndex. Supporting roles inside this cluster: tooling. Direct collaborators: Src Services. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Src Services

**核心类 / 符号：**
- `writeIndex`

**代表文件：**
- `src/stores/searchStore.js`

---

### data (root)

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. It is reachable from an entry capability. Main modules: (root). Example files: package.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `package.json`

---

