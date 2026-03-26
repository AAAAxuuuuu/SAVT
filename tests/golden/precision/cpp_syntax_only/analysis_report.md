# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **3 个** |
| 涉及源码文件 | **6 个** |
| 子模块总数 | **3 个** |
| 发起入口 | App Frontend、App |
| 后台支撑 | App Backend |

---

## 模块详情

### 🎨 界面展示 (UI) App Frontend

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 2 个源码文件。
👉 核心类/函数: MainWindow, show

- 源文件：**2 个**（`App`）
- 被依赖：**1 次** / 依赖他人：**1 次**
- 协作模块：App、App Backend

**核心类 / 符号：**
- `MainWindow`
- `show`

**代表文件：**
- `App/frontend/view/MainWindow.cpp`
- `App/frontend/view/MainWindow.h`

---

### procedure_centric App

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 2 main flow(s). It is reachable from an entry capability. Main modules: App. Example files: App/main.cpp. Key symbols: main. Supporting roles inside this cluster: procedure_centric. Direct collaborators: App Backend, App Frontend. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**2 次**
- 协作模块：App Backend、App Frontend

**核心类 / 符号：**
- `main`

**代表文件：**
- `App/main.cpp`

---

### interaction App Backend

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 3 file(s), spans stage 2, and appears in 1 main flow(s). It is reachable from an entry capability. Folder scope: App. Main modules: App/backend. Example files: App/backend/model/Task.h, App/backend/service/TaskService.cpp, App/backend/service/TaskService.h. Key symbols: Task, TaskService, current, nextId. Supporting roles inside this cluster: interaction. Direct collaborators: App, App Frontend. It is shown in the default view.

- 源文件：**3 个**（`App`）
- 被依赖：**2 次** / 依赖他人：**0 次**
- 协作模块：App、App Frontend

**核心类 / 符号：**
- `Task`
- `TaskService`
- `current`
- `nextId`

**代表文件：**
- `App/backend/model/Task.h`
- `App/backend/service/TaskService.cpp`
- `App/backend/service/TaskService.h`

---

