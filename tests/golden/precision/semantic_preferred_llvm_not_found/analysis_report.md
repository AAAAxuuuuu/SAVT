# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 分析引擎状态

| 项目 | 值 |
| --- | --- |
| 当前精度 | 语义失败后回退到语法分析 |
| 主引擎 | tree-sitter |
| 语义状态 | 阻断：未找到 LLVM/Clang |
| 语义状态码 | `llvm_not_found` |
| 编译数据库 | <fixture_root>/compile_commands.json |
| 已发现源码文件 | 2 |
| 已成功解析文件 | 1 |

**状态说明：** SAVT_ENABLE_CLANG_TOOLING was requested, but no compatible LLVM/Clang installation was found. Set SAVT_LLVM_ROOT to a distribution that contains clang-c/Index.h and libclang.

**处理建议：** 请安装带 clang-c/Index.h 和 libclang 的 LLVM/Clang 发行包，并正确设置 SAVT_LLVM_ROOT。

**诊断日志：**
- SAVT_ENABLE_CLANG_TOOLING was requested, but no compatible LLVM/Clang installation was found. Set SAVT_LLVM_ROOT to a distribution that contains clang-c/Index.h and libclang.
- Analyzer precision mode: semantic_preferred

---

## 项目概览

| 指标 | 数值 |
| --- | --- |
| 识别模块数 | **1 个** |
| 涉及源码文件 | **2 个** |
| 子模块总数 | **1 个** |
| 发起入口 | (root) |

---

## 模块详情

### 🔧 辅助工具 (Utils) (root)

**职责：** 提供通用的边角料功能（如时间转换、字符串处理等）。

提供通用的边角料功能（如时间转换、字符串处理等）。 包含了 2 个源码文件。
👉 核心类/函数: helper, main

- 源文件：**2 个**
- 被依赖：**0 次** / 依赖他人：**0 次**

**核心类 / 符号：**
- `helper`
- `main`

**代表文件：**
- `compile_commands.json`
- `main.cpp`

---

