# 系统上下文 (L1)

## 发起入口
- **Src Main Bootstrap** (type_centric)：作为项目的启动入口，负责初始化运行环境并将控制权交给核心模块。

## 核心能力
- **Src Main Service** (analysis)：负责核心的数据分析或业务逻辑处理，将输入转化为结构化的结果。
- **Src Main Controller** (interaction)：承担项目中某项核心业务能力，参与主要的用户可见工作流。

## 后台支撑
- **Src Main Repository** (💾 数据存储 (Database))：负责将数据持久化保存到本地数据库或文件中。
- **Src Main Model** (core_model)：提供底层支撑服务，保持核心能力流程的稳定性和可复用性。
- **(root)** (type_centric)：提供底层支撑服务，保持核心能力流程的稳定性和可复用性。

