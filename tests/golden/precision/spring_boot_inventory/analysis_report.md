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
| 发起入口 | Src Main Bootstrap |
| 后台支撑 | Src Main Repository、Src Main Model、(root) |

---

## 模块详情

### type_centric Src Main Bootstrap

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0. It is reachable from an entry capability. Folder scope: Src. Main modules: src/main/bootstrap. Example files: src/main/java/com/example/inventory/InventoryApplication.java. Key symbols: InventoryApplication. Supporting roles inside this cluster: type_centric. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `InventoryApplication`

**代表文件：**
- `src/main/java/com/example/inventory/InventoryApplication.java`

---

### analysis Src Main Service

**职责：** 负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。

负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。 Covers 1 module(s) and 1 file(s), spans stage 1, and appears in 3 main flow(s). Folder scope: Src. Main modules: src/main/service. Example files: src/main/java/com/example/inventory/service/InventoryService.java. Key symbols: InventoryService. Supporting roles inside this cluster: analysis. Direct collaborators: Src Main Controller, Src Main Model, Src Main Repository. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**2 次**
- 协作模块：Src Main Controller、Src Main Model、Src Main Repository

**核心类 / 符号：**
- `InventoryService`

**代表文件：**
- `src/main/java/com/example/inventory/service/InventoryService.java`

---

### interaction Src Main Controller

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 3 main flow(s). Folder scope: Src. Main modules: src/main/controller. Example files: src/main/java/com/example/inventory/controller/InventoryController.java. Key symbols: InventoryController. Supporting roles inside this cluster: interaction. Direct collaborators: Src Main Service. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Src Main Service

**核心类 / 符号：**
- `InventoryController`

**代表文件：**
- `src/main/java/com/example/inventory/controller/InventoryController.java`

---

### 💾 数据存储 (Database) Src Main Repository

**职责：** 负责将数据持久化保存到本地数据库或文件中。

负责将数据持久化保存到本地数据库或文件中。 包含了 1 个源码文件。
👉 核心类/函数: InventoryRepository

- 源文件：**1 个**（`src`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：Src Main Model、Src Main Service

**核心类 / 符号：**
- `InventoryRepository`

**代表文件：**
- `src/main/java/com/example/inventory/repository/InventoryRepository.java`

---

### core_model Src Main Model

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 1 file(s), spans stage 3, and appears in 1 main flow(s). Folder scope: Src. Main modules: src/main/model. Example files: src/main/java/com/example/inventory/model/InventoryItem.java. Key symbols: InventoryItem. Supporting roles inside this cluster: core_model. Direct collaborators: Src Main Repository, Src Main Service. It is shown in the default view.

- 源文件：**1 个**（`src`）
- 被依赖：**2 次** / 依赖他人：**0 次**
- 协作模块：Src Main Repository、Src Main Service

**核心类 / 符号：**
- `InventoryItem`

**代表文件：**
- `src/main/java/com/example/inventory/model/InventoryItem.java`

---

### type_centric (root)

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: (root). Example files: pom.xml. Key symbols: none. Supporting roles inside this cluster: type_centric. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `pom.xml`

---

