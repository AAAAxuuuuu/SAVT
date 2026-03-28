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
| 发起入口 | Tools |
| 后台支撑 | Analytics、Data、Storage、Support |

---

## 模块详情

### 💾 数据存储 (Database) Tools

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: main

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `main`

**代表文件：**
- `tools/export_report.py`

---

### 🎨 界面展示 (UI) Analytics

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 1 个源码文件。
👉 核心类/函数: build_summary

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `build_summary`

**代表文件：**
- `analytics/summary_builder.py`

---

### 💾 数据存储 (Database) Data

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `data/report_snapshot.json`

---

### 💾 数据存储 (Database) Storage

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: load_snapshot

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `load_snapshot`

**代表文件：**
- `storage/cache_store.py`

---

### tooling Support

**职责：** 构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。

构建或预处理阶段的辅助模块，在主运行时流程之外完成转换、验证或生成工作。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: support. Example files: support/formatting.py. Key symbols: normalize_title. Supporting roles inside this cluster: tooling. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `normalize_title`

**代表文件：**
- `support/formatting.py`

---

