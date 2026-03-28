# 项目架构全景报告

> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。

---

## 分析引擎状态

| 项目 | 值 |
| --- | --- |
| 当前精度 | 语义阻断 |
| 主引擎 | none |
| 语义状态 | 阻断：缺少 compile_commands.json |
| 语义状态码 | `missing_compile_commands` |
| 编译数据库 | (none) |
| 已发现源码文件 | 0 |
| 已成功解析文件 | 0 |

**状态说明：** Semantic analysis is blocked because no compile_commands.json was found for the target project.

**处理建议：** 请先为被分析项目生成 compile_commands.json，并确保它位于项目根目录或 build 目录。

**诊断日志：**
- Semantic analysis requested, but no compile_commands.json was found.
- Compilation database search paths: <fixture_root>/compile_commands.json; <fixture_root>/.qtc_clangd/compile_commands.json; <fixture_root>/build/compile_commands.json; <fixture_root>/build/.qtc_clangd/compile_commands.json
- Analyzer precision mode: semantic_required

---

## 暂无模块结果

当前还没有提炼出可显示的模块视图。请先检查上面的分析引擎状态、编译数据库和诊断日志。
