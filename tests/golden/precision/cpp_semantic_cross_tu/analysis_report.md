# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 分析引擎状态

| 项目 | 值 |
| --- | --- |
| 当前精度 | 语义分析 |
| 主引擎 | libclang-cindex |
| 语义状态 | 已进入 Clang/LibTooling |
| 语义状态码 | `semantic_ready` |
| 编译数据库 | <fixture_root>/compile_commands.json |
| 已发现源码文件 | 4 |
| 已成功解析文件 | 2 |

**状态说明：** Clang/LibTooling semantic backend completed successfully.

**诊断日志：**
- Analyzer precision mode: semantic_required

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **1 个** |
| 涉及源码文件 | **4 个** |
| 子模块总数 | **1 个** |

---

## 模块详情

### interaction (root)

**职责：** 承担项目中某项核心业务能力，参与主要的用户可见工作流。

承担项目中某项核心业务能力，参与主要的用户可见工作流。 Covers 1 module(s) and 4 file(s), spans stage 0. It is reachable from an entry capability. Main modules: (root). Example files: base.cpp, compile_commands.json, derived.cpp, model.h. Key symbols: Base, Derived, Box, callBase, makeBox. Supporting roles inside this cluster: interaction. It is shown in the default view.

- 源文件：**4 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `Base`
- `Derived`
- `Box`
- `callBase`
- `makeBox`

**代表文件：**
- `base.cpp`
- `compile_commands.json`
- `derived.cpp`
- `model.h`

---

