# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **5 个** |
| 涉及源码文件 | **5 个** |
| 子模块总数 | **5 个** |
| 发起入口 | 图数据管理 — Tools |
| 后台支撑 | Analytics、Data、Storage、Support |

---

## 模块详情

### 🧱 核心数据结构 图数据管理 — Tools

**职责：** 负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。

负责加载、管理节点与边等图结构数据，是后续算法和业务分析的底层数据来源。 包含了 1 个源码文件。
👉 核心类/函数: main

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `main`

**代表文件：**
- `tools/build_graph.py`

---

### 🎨 界面展示 (UI) Analytics

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 1 个源码文件。
👉 核心类/函数: build_report

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `build_report`

**代表文件：**
- `analytics/report_builder.py`

---

### data Data

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: data. Example files: data/schedule.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `data/schedule.json`

---

### 💾 数据存储 (Database) Storage

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: load_schedule

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `load_schedule`

**代表文件：**
- `storage/schedule_store.py`

---

### tooling Support

**职责：** 构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。

构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: support. Example files: support/formatting.py. Key symbols: normalize_name. Supporting roles inside this cluster: tooling. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `normalize_name`

**代表文件：**
- `support/formatting.py`

---

