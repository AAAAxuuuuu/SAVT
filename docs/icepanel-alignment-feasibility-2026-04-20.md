# IcePanel 逻辑对齐可行性分析

日期：2026-04-20

## 结论

可以往 IcePanel 的方向靠，但不建议直接从 QML 画线层硬改。

当前工程已经具备一部分“像 IcePanel”的基础：

- 有统一 scene 数据源，UI 主要消费 `scene.nodes / scene.edges / scene.groups / scene.bounds`
- 有 L2 -> L3 下钻
- 有组件图、能力图、关系聚焦、拖拽重排、边路由和焦点关系视图

但当前实现的本质仍然是：

- backend 先生成一张“分析结果图”
- layout 把它排版成场景
- QML 再把这张场景图渲染出来

它还不是 IcePanel 那种“对象和连接是长期存在的统一模型，可跨图复用、可上下卷积、可在多层图里同步变化”的模型驱动逻辑。

所以如果目标只是“视觉和交互更像 IcePanel”，改造量中等；
如果目标是“连线语义、组件语义、上下钻语义都和 IcePanel 一样”，就需要把图模型层往通用架构模型升级。

## 当前真实入口

先确认一个关键事实：当前运行中的主 UI 已经不是旧的 `graph/*.qml`。

- `apps/desktop/qml/Main.qml`
- `apps/desktop/qml/greenfield/shell/WorkspaceStage.qml`
- `apps/desktop/qml/greenfield/atlas/ArchitectureCanvas.qml`

`Main.qml` 现在直接加载 `greenfield/MainSurface`，组件视图实际走的是 `WorkspaceStage.qml` 里的 `ArchitectureCanvas`。

这意味着如果要做 IcePanel 对齐，主战场应该是：

- greenfield QML 画布和交互
- `SceneMapper`
- `ComponentGraph` / `CapabilityGraph`
- `LayeredGraphLayout`

而不是回头大改旧版 `apps/desktop/qml/graph/*.qml`。

## 当前实现和 IcePanel 的主要差异

### 1. 当前节点/边仍然是“场景结果”，不是“统一架构模型”

当前组件图模型定义在：

- `src/core/include/savt/core/ComponentGraph.h`
- `src/core/src/ComponentGraph.cpp`

它的节点类型只有：

- `Entry`
- `Component`
- `Support`

边类型只有：

- `Activates`
- `Coordinates`
- `UsesSupport`

这说明当前 L3 图更像“针对分析结果的一次视图投影”，而不是通用的 C4 / IcePanel 风格对象模型。

IcePanel 风格通常需要更稳定的模型属性，例如：

- `id`
- `name`
- `type`
- `parentId`
- `external`
- `status`
- `description`
- `technologyIds`
- `tags`
- `originId`
- `targetId`
- `direction`
- `lineShape`
- `via`
- `lowerConnectionOf`

当前仓库里这些语义并不完整。

### 2. 当前组件图是“能力域内局部图”，不是“统一层级对象树”

`buildComponentGraphForCapabilityImpl(...)` 明确是围绕一个 capability 生成内部组件图：

- `src/core/src/ComponentGraph.cpp`

当前逻辑本质上是：

- 先选中一个 capability
- 再把这个 capability 对应的 overview 节点收拢成 L3 组件图

这和 IcePanel 的差异在于：

- IcePanel 的对象天然属于一个层级树
- 下钻不是“重新推导一张局部图”，而是“进入同一模型的更低层视图”

### 3. 当前 layout 仍然偏“分析排版”，不是“对象端口 + 连接模型”

当前 L3 layout 在这里：

- `src/layout/src/LayeredGraphLayout.cpp`

`layoutComponentScene(...)` 的核心特征是：

- 按 `stageIndex` 把节点排成列
- 每个节点给固定宽高
- 边默认只是从右侧中点连到左侧中点
- route points 主要还是为“展示”服务

这和 IcePanel 的差异在于：

- IcePanel 连接更像从对象的固定连接点出发
- 同一连接在不同图里仍保持模型身份
- 线型、方向、标签位置、lower connection 等属于连接本身

### 4. 当前 QML 连线逻辑很强，但仍然是“渲染层聪明”

当前 greenfield 画布在：

- `apps/desktop/qml/greenfield/atlas/ArchitectureCanvas.qml`

这里已经做了很多高级处理：

- hover / selected 焦点边过滤
- overview 和 component 两套路由
- 曲线 / 正交混合
- 避障绕行
- 手动拖拽节点后的连线跟随
- 关系聚焦 overlay

这说明“画布能力”其实不弱。

但它的问题是：

- 这些逻辑大多还在 QML 渲染层里
- 连线可见性、焦点规则、边 bundling、关系 overlay 都是视图策略
- 不是 backend 持有的统一连接语义

也就是说，现在更像“一个很聪明的 viewer”，还不是“一个模型驱动的架构编辑/浏览器”。

### 5. 当前关系过滤是“按焦点裁剪”，不是“模型连接复用”

关系过滤现在主要在：

- `apps/desktop/qml/logic/GraphSelection.js`
- `apps/desktop/qml/state/SelectionState.qml`
- `apps/desktop/qml/greenfield/atlas/ArchitectureCanvas.qml`

这套逻辑更偏：

- 选中节点 -> 只显示关联边
- hover 节点 -> 临时突出相关边
- 关系聚焦时切到 overlay

而 IcePanel 的关键价值更偏：

- 连接是模型对象
- 可以从 lower level 向 higher level 复用
- 同一连接可在多个 diagram 中出现
- 改 connection 本体会同步影响多个图

## 已经接近 IcePanel 的基础

尽管差异明显，但当前仓库有几块基础已经能复用：

### 1. scene 事实源已经统一

- `src/ui/include/savt/ui/SceneMapper.h`
- `src/ui/src/SceneMapper.cpp`
- `docs/phase5-scene-layout-unification-2026-03-28.md`

这一点非常关键，因为它让后续从“专用 scene DTO”升级到“通用模型投影”成为可能。

### 2. greenfield 画布已经具备复杂交互底座

`ArchitectureCanvas.qml` 已经支持：

- 缩放
- 平移
- 节点 hover / select / double click
- 非组件态下的手动拖拽重排
- 边避障
- 焦点关系视图

所以不需要从零重写画布。

### 3. 仓库已经有 IcePanel import 文件

- `icepanel-landscape-import.yaml`

虽然这不是运行时代码，但它说明当前项目已经在概念上接受了：

- 对象层级
- 连接单独建模
- 领域 / 系统 / app / component 这种 C4 风格结构

这个文件可以作为后续模型重构的参考目标。

## 三种改造路线

### 路线 A：只做“像 IcePanel 的体验”

目标：

- 节点更像对象卡
- 连接更像端口连接
- 交互更像架构图浏览器

基本做法：

1. 保留现有 `CapabilityGraph / ComponentGraph / SceneMapper`
2. 继续使用 `ArchitectureCanvas.qml`
3. 把边的出入口改成离散 port，而不是只靠左右中点和临时 offset
4. 给 edge 增加渲染属性，例如 `direction`、`lineShape`、`label`
5. 让手动拖拽位置可持久化
6. 给组件卡增加更稳定的 object meta 呈现，例如父对象、类型、外部标识、技术标签

优点：

- 成本最低
- 现有 UI 改动最集中
- 很快能获得“视觉和交互接近 IcePanel”的效果

缺点：

- 语义层没有真正变成 IcePanel 逻辑
- lower connections、跨图复用、模型同步仍然做不到

适合：

- 你现在最在意的是“看起来和操作起来像”

### 路线 B：做“半模型驱动”的过渡方案

目标：

- UI 和连接语义明显向 IcePanel 靠拢
- 但不一次性推翻现有 backend

建议新增一层通用模型，例如：

- `ArchitectureObject`
- `ArchitectureConnection`
- `ArchitectureViewProjection`

建议字段：

- `ArchitectureObject`
  - `id`
  - `name`
  - `type`
  - `parentId`
  - `external`
  - `status`
  - `summary`
  - `technologyTags`
- `ArchitectureConnection`
  - `id`
  - `originId`
  - `targetId`
  - `direction`
  - `kind`
  - `lineShape`
  - `label`
  - `weight`
  - `implied`
  - `sourceLevel`

然后让：

- `CapabilityGraph` 变成 L2 投影
- `ComponentGraph` 变成 L3 投影
- `SceneMapper` 改成“从统一对象模型投影 scene”

优点：

- 风险比全量重写小
- 后面可以逐步接 lower connection / implied connection
- 现有分析结果还能继续复用

缺点：

- 要改 C++ 数据结构和映射链路
- 比单纯改 QML 明显更大

适合：

- 你要的不只是像，而是想把后续扩展空间也留出来

### 路线 C：直接做“统一架构模型 + 多层视图投影”

目标：

- 真正按 IcePanel 思路重构

核心思路：

1. 先建立统一架构模型树
2. 所有 diagram 都只是模型投影
3. connection 成为第一类对象
4. 支持 lower / implied connection
5. L2 / L3 不再分别建独立专用图，而是统一对象树的不同视角

这条路线最终效果最好，但改造面最大，会触及：

- `src/core`
- `src/layout`
- `src/ui`
- `apps/desktop/qml/greenfield`

如果没有明确版本规划，不建议直接一步到位。

## 推荐方案

建议走：

1. 先做路线 A 的 UI/交互对齐
2. 同时按路线 B 的思路补一层通用对象/连接模型
3. 等通用模型稳定后，再考虑是否继续走向路线 C

换句话说：

- 第一阶段先把“看起来和操作起来像 IcePanel”做出来
- 第二阶段再把“底层语义也像 IcePanel”做出来

这是风险最低、收益最稳定的办法。

## 建议的分阶段实施

### Phase 1：只改 greenfield 画布

涉及文件：

- `apps/desktop/qml/greenfield/atlas/ArchitectureCanvas.qml`
- `apps/desktop/qml/greenfield/shell/WorkspaceStage.qml`

目标：

- 统一对象卡样式
- 引入固定 port 概念
- 为边增加 line shape / direction / label 的渲染能力
- 默认把组件边的展示逻辑改成更接近 IcePanel 的连接浏览

### Phase 2：扩展 scene edge / node 元数据

涉及文件：

- `src/ui/src/SceneMapper.cpp`
- `src/ui/include/savt/ui/SceneMapper.h`
- `src/core/include/savt/core/ComponentGraph.h`
- `src/core/include/savt/core/CapabilityGraph.h`

目标：

- 给节点补 `parentId / external / status / tags / technologies`
- 给边补 `direction / lineShape / label / impliedSource`

### Phase 3：引入统一对象/连接模型

建议新增：

- `src/core/include/savt/core/ArchitectureModel.h`
- `src/core/src/ArchitectureModel.cpp`

目标：

- L2 / L3 都从统一模型投影
- 逐步弱化 capability 专用图和 component 专用图的硬编码边界

### Phase 4：接入 lower / implied connection

目标：

- 子层连接可以汇总到父层
- 父层和子层之间具备可追溯映射

这一步才是真正接近 IcePanel 语义的关键节点。

## 不建议的做法

### 1. 不建议只改线条样式

只把线画成直角、曲线或者加箭头，不会让逻辑变成 IcePanel。

### 2. 不建议继续在旧 `graph/*.qml` 上投入

旧图组件不是当前运行主链路，继续在那里深改会形成双份实现。

### 3. 不建议先重写 layout 再想模型

如果模型不先定，layout 很容易变成第二套临时事实源。

## 最后判断

如果你的意思是：

- “现在 UI 的连线方式、组件节点呈现方式，能不能改成更像 IcePanel？”

答案是：

- 能，而且建议直接在 `greenfield/atlas/ArchitectureCanvas.qml` 上做，成本可控。

如果你的意思是：

- “能不能让当前系统的连接语义、节点层级语义、下钻逻辑都变成 IcePanel 那种模型驱动方式？”

答案是：

- 能，但这已经不是单纯 UI 调整，而是一次中等规模的模型层重构。

## 建议的下一步

如果要正式开改，最稳的切入口是先做下面两件事：

1. 先把 `ArchitectureCanvas.qml` 抽出统一的 node port / edge route / edge meta 渲染层
2. 再给 `SceneMapper` 和 `ComponentGraph` 增加最小一组 IcePanel 风格字段

这样第一轮就能看到明显效果，而且不会把 backend 一次性推翻。
