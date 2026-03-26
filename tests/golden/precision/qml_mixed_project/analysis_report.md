# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **4 个** |
| 涉及源码文件 | **7 个** |
| 子模块总数 | **4 个** |
| 发起入口 | App |
| 后台支撑 | Backend、Data |

---

## 模块详情

### procedure_centric App

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 1 file(s), spans stage 0, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: app. Example files: app/main.cpp. Key symbols: main. Supporting roles inside this cluster: procedure_centric. Direct collaborators: Backend. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Backend

**核心类 / 符号：**
- `main`

**代表文件：**
- `app/main.cpp`

---

### 🎨 界面展示 (UI) Ui

**职责：** 负责渲染界面和处理用户的点击/滑动等操作。

负责渲染界面和处理用户的点击/滑动等操作。 包含了 2 个源码文件。

- 源文件：**2 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `ui/MapShell.qml`
- `ui/components/LegendPanel.qml`

---

### interaction Backend

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 3 file(s), spans stage 1, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: backend. Example files: backend/facade/TrafficBackend.cpp, backend/facade/TrafficBackend.h, backend/model/Station.h. Key symbols: TrafficBackend, Station, currentStation, refresh. Supporting roles inside this cluster: interaction. Direct collaborators: App. It is shown in the default view.

- 源文件：**3 个**
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：App

**核心类 / 符号：**
- `TrafficBackend`
- `Station`
- `currentStation`
- `refresh`

**代表文件：**
- `backend/facade/TrafficBackend.cpp`
- `backend/facade/TrafficBackend.h`
- `backend/model/Station.h`

---

### data Data

**职责：** 负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。

负责持久化存储配置、资源或项目运行数据，供其他模块读取和写入。 Covers 1 module(s) and 1 file(s), spans stage 0. Main modules: data. Example files: data/metro.json. Key symbols: none. Supporting roles inside this cluster: data. It is shown in the default view.

- 源文件：**1 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `data/metro.json`

---

