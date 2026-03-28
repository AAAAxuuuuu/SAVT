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
| 发起入口 | Host |
| 后台支撑 | Plugins |

---

## 模块详情

### procedure_centric Host

**职责：** 作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。 Covers 1 module(s) and 3 file(s), spans stage 0, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: host. Example files: host/PluginHost.cpp, host/PluginHost.h, host/main.cpp. Key symbols: PluginHost, main, start. Supporting roles inside this cluster: procedure_centric. Direct collaborators: Plugins. It is shown in the default view.

- 源文件：**3 个**
- 被依赖：**0 次** / 依赖他人：**1 次**
- 协作模块：Plugins

**核心类 / 符号：**
- `PluginHost`
- `main`
- `start`

**代表文件：**
- `host/PluginHost.cpp`
- `host/PluginHost.h`
- `host/main.cpp`

---

### type_centric Plugins

**职责：** 提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

提供底层支撑服务，保持核心能力流程的稳定性和可复用性。 Covers 1 module(s) and 2 file(s), spans stage 1, and appears in 1 main flow(s). It is reachable from an entry capability. Main modules: plugins. Example files: plugins/metrics/MetricsPlugin.cpp, plugins/metrics/MetricsPlugin.h. Key symbols: MetricsPlugin, activate. Supporting roles inside this cluster: type_centric. Direct collaborators: Host. It is shown in the default view.

- 源文件：**2 个**
- 被依赖：**1 次** / 依赖他人：**0 次**
- 协作模块：Host

**核心类 / 符号：**
- `MetricsPlugin`
- `activate`

**代表文件：**
- `plugins/metrics/MetricsPlugin.cpp`
- `plugins/metrics/MetricsPlugin.h`

---

