# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 分析引擎状态

| 项目 | 值 |
| --- | --- |
| 当前精度 | 语义阻断 |
| 主引擎 | libclang-cindex |
| 语义状态 | 阻断：系统头不可解析 |
| 语义状态码 | `system_headers_unresolved` |
| 编译数据库 | <fixture_root>/compile_commands.json |
| 已发现源码文件 | 2 |
| 已成功解析文件 | 0 |

**状态说明：** compile_commands.json was found, but libclang could not resolve required system headers.

**处理建议：** 请确认 compile_commands.json 由真实工具链生成，并保留 SDK/标准库的搜索路径。

**诊断日志：**
- <fixture_root>/main.cpp:1:10: error: 'vector' file not found
- <fixture_root>/main.cpp:4:5: error: use of undeclared identifier 'std'
- <fixture_root>/main.cpp:4:20: error: expected '(' for function-style cast or type construction
- <fixture_root>/main.cpp:4:22: error: use of undeclared identifier 'values'
- <fixture_root>/main.cpp:5:29: error: use of undeclared identifier 'values'
- Semantic analysis blocked by unresolved system headers for: <fixture_root>/main.cpp
- Semantic backend parsed 0/1 project translation units; unresolved system-header failures: 1.
- Semantic backend did not successfully parse any translation units.
- Analyzer precision mode: semantic_required

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **1 个** |
| 涉及源码文件 | **2 个** |
| 子模块总数 | **1 个** |

---

## 模块详情

### interaction (root)

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 2 file(s), spans stage 0. It is reachable from an entry capability. Main modules: (root). Example files: compile_commands.json, main.cpp. Key symbols: none. Supporting roles inside this cluster: interaction. It is shown in the default view.

- 源文件：**2 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**代表文件：**
- `compile_commands.json`
- `main.cpp`

---

