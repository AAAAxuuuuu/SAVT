# 项目级配置

SAVT 现在支持在被分析项目根目录放置项目级配置文件，用来纠正项目特例，而不是反复修改内置启发式。

## 查找路径

按以下顺序查找首个存在的配置文件：

1. `config/savt.project.json`
2. `config/savt-project.json`
3. `.savt/project.json`
4. `savt.project.json`

## 支持项

```json
{
  "version": 1,
  "analysis": {
    "ignoreDirectories": ["tests/fixtures", "tests/golden"],
    "moduleMerges": [
      { "match": "src/ui", "module": "workspace/ui" }
    ],
    "roleOverrides": [
      { "match": "src/ui", "role": "presentation" }
    ],
    "entryOverrides": [
      { "match": "apps/desktop", "entry": true }
    ],
    "dependencyFoldRules": [
      { "match": "third_party", "collapse": true, "hideByDefault": true }
    ]
  }
}
```

## 规则说明

- `ignoreDirectories`
  - 目录前缀匹配。
  - 匹配后，该目录及其子树不会进入扫描。

- `moduleMerges`
  - `match` 支持匹配相对路径前缀或当前推断出的模块名前缀。
  - `module` 是最终归并后的模块名。

- `roleOverrides`
  - 用于覆盖 `ArchitectureOverview` 的角色分类结果。
  - 常见值如 `presentation`、`analysis`、`storage`、`dependency`、`core_model`。

- `entryOverrides`
  - 显式指定某个模块是否应被视为入口。
  - 会覆盖默认入口推断。

- `dependencyFoldRules`
  - 命中的 capability 会在 L2 默认折叠。
  - `hideByDefault=true` 时默认隐藏，`collapse=true` 时默认折叠。

## 当前仓库示例

本仓库已提供 [savt.project.json](/Users/adg/Documents/code/SAVT/config/savt.project.json)，用于：

- 忽略 `tests/fixtures`、`tests/golden`、`docs`、`config`
- 固定 `apps/desktop`、`src/ui`、`src/layout`、`src/analyzer`、`src/ai`、`src/core` 的角色归类
- 显式声明 `apps/desktop` 为入口
- 默认折叠 `third_party` 和 `tools/llvm`
