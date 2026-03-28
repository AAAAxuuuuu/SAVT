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
| 识别模块数 | **6 个** |
| 涉及源码文件 | **6 个** |
| 子模块总数 | **6 个** |
| 发起入口 | Src Server |
| 后台支撑 | Src Repositories、(root) |

---

## 模块详情

### interaction Src Server

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 4 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/server. Example files: src/server.js. Key symbols: bootstrapServer. Supporting roles inside this cluster: interaction. Direct collaborators: Src Routes. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Src Routes

**核心类 / 符号：**
- `bootstrapServer`

**代表文件：**
- `src/server.js`

---

### 🎨 界面展示 (UI) Src Routes

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 1 个源码文件。
👉 核心类/函数: buildInventoryRoutes

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Controllers、Src Server

**核心类 / 符号：**
- `buildInventoryRoutes`

**代表文件：**
- `src/routes/inventory.js`

---

### interaction Src Controllers

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 1 file(s), spans stage 2, and appears in 3 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/controllers. Example files: src/controllers/inventoryController.js. Key symbols: listInventory. Supporting roles inside this cluster: interaction. Direct collaborators: Src Routes, Src Services. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Routes、Src Services

**核心类 / 符号：**
- `listInventory`

**代表文件：**
- `src/controllers/inventoryController.js`

---

### analysis Src Services

**职责：** 负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。

负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。 Covers 1 module(s) and 1 file(s), spans stage 3, and appears in 2 main flow(s). It is reachable from an entry capability. Folder scope: Src. Main modules: src/services. Example files: src/services/inventoryService.js. Key symbols: loadInventory. Supporting roles inside this cluster: analysis. Direct collaborators: Src Controllers, Src Repositories. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Controllers、Src Repositories

**核心类 / 符号：**
- `loadInventory`

**代表文件：**
- `src/services/inventoryService.js`

---

### 💾 数据存储 (Database) Src Repositories

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: readInventory

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Src Services

**核心类 / 符号：**
- `readInventory`

**代表文件：**
- `src/repositories/inventoryRepository.js`

---

### data (root)

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: (root). Example files: package.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `package.json`

---

