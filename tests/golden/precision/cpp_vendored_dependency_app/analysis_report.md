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
| 已成功解析文件 | 6 |

**状态说明：** Semantic analysis was not requested for this run.

**诊断日志：**
- Analyzer precision mode: syntax_only

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **2 个** |
| 涉及源码文件 | **6 个** |
| 子模块总数 | **2 个** |
| 发起入口 | MainSystem |
| 后台支撑 | QXlsx 1 5 0 |

---

## 模块详情

### 🎨 界面展示 (UI) MainSystem

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 3 个源码文件。
👉 核心类/函数: MainWindow, show

- 源文件：**3 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：QXlsx 1 5 0

**核心类 / 符号：**
- `MainWindow`
- `show`

**代表文件：**
- `mainSystem/main.cpp`
- `mainSystem/mainwindow.cpp`
- `mainSystem/mainwindow.h`

---

### dependency QXlsx 1 5 0

**职责：** 第三方或外部依赖库，为主系统提供可复用的基础能力，不属于核心业务代码。

第三方或外部依赖库，为主系统提供可复用的基础能力，不属于核心业务代码。 Covers 1 module(s) and 3 file(s), spans stage 1, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: QXlsx-1.5.0. Example files: QXlsx-1.5.0/HelloWorld/main.cpp, QXlsx-1.5.0/QXlsx/header/xlsxdocument.h, QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp. Key symbols: QXlsxDocument, main, save. Supporting roles inside this cluster: dependency. Direct collaborators: MainSystem. It is shown in the default view.

- 源文件：**3 个**
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：MainSystem

**核心类 / 符号：**
- `QXlsxDocument`
- `main`
- `save`

**代表文件：**
- `QXlsx-1.5.0/HelloWorld/main.cpp`
- `QXlsx-1.5.0/QXlsx/header/xlsxdocument.h`
- `QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp`

---

