---
type: content-guideline
scope: wacom-content
status: active
updated: 2026-07-15
tags:
  - wacom/content
  - wacom/assets
  - wacom/editor
---

# Wacom Content Organization

> [!info] 本文职责
> 本文记录 `/Game/Wacom` 下资产目录的长期归档口径。新增资产、Commandlet 生成路径和蓝图迁移先按本文判断归属。

## 顶层目录

| 目录 | 职责 |
|---|---|
| `/Game/Wacom/Data` | 规则数据资产：卡牌、角色、敌人、商店、探索事件 |
| `/Game/Wacom/Core` | 项目核心蓝图：GameMode、PlayerCharacter、PlayerController 等 |
| `/Game/Wacom/Run` | Run / 探索表现资产，包括 Run Tunnel 蓝图和后续探索场景表现 |
| `/Game/Wacom/UI` | UMG / CommonUI Widget Blueprint 和 UI 视觉资产 |
| `/Game/Wacom/Input` | Enhanced Input 资产，仍由 `WacomCreateInputAssetsCommandlet` 生成 |
| `/Game/Wacom/Maps` | 关卡与临时关卡验证用场景资产 |
| `/Game/Wacom/Material` | Wacom 自有材质、材质实例和贴图入口 |

## 数据资产

`WacomEditor` 的内容生成器只把静态规则数据写到 `/Game/Wacom/Data`：

```text
/Game/Wacom/Data/Cards/BugGirl
/Game/Wacom/Data/Cards/Rewards
/Game/Wacom/Data/Characters
/Game/Wacom/Data/Enemies/Snake
/Game/Wacom/Data/Encounters
/Game/Wacom/Data/Events
/Game/Wacom/Data/Shops
```

不要把 Input、UI、Map、Material 放进 `/Data`。`/Data` 表示规则数据，不表示“所有可以被加载的资产”。

## 蓝图资产

玩家核心蓝图归档到：

```text
/Game/Wacom/Core/GameModes
/Game/Wacom/Core/Player
/Game/Wacom/Core/Controllers
```

Run Tunnel authoring 蓝图归档到：

```text
/Game/Wacom/Run/Tunnel/Blueprints
```

`/Game/Wacom/_GAME` 是历史临时目录。新增资产不得继续放入 `_GAME`；现有资产迁移时使用编辑器 Content Browser 的 Move，再对 `/Game/Wacom` 执行 Fix Up Redirectors。

## 迁移规则

- 移动 `.uasset` 必须通过编辑器 Content Browser，不用资源管理器直接移动文件。
- 迁移后执行 Fix Up Redirectors 并 Save All。
- Commandlet 生成路径先于资产迁移更新，避免新旧目录继续分叉。
- 不用 Core Redirect 处理资产路径迁移；Core Redirect 只处理 C++ 类型、属性、函数重命名。

## Worktree 本地依赖

Git worktree 只保证受版本管理内容完整。当前 `/Game/Art`、`/Game/Asset` 和 `/Game/L_TestBattle` 仍受 `Content/*` ignore policy 排除；`/Game/DreamMaterials` 已整体纳入版本控制，其中 `.uasset` 使用 Git LFS 管理并随 worktree 检出。

多 worktree 开发使用 [`Worktree_Development.md`](./Worktree_Development.md) 记录的独立本地依赖层：每个 worktree 拥有自己的 D 盘 backing directory，通过 Content 子目录 Junction 暴露给 UE。该机制只是开发期补水，不把 ignored 目录提升为规则或资产真相。

长期迁移规则保持不变：

- Wacom 自有并会随游戏出货的资产，通过 Content Browser 迁入 `/Game/Wacom` 并由 Git LFS 管理。
- 第三方或授权上不适合进入主仓库的内容，建立带版本与安装清单的本地依赖包。
- DreamShader 以 `.dsm`、`.dsh` 和生成脚本作为可编辑制作真源；`/Game/DreamMaterials` 的全部生成 `.uasset` 同时提交，保证任何 checkout 和 worktree 都取得一致运行时资产。

## 外部依赖审计

移动 ignored Content 前，先运行 `WacomAuditContentDependencies` 的只读 AssetRegistry 审计。命令、JSON contract、当前基线和迁移批次见 [`Content_Dependency_Audit.md`](./Content_Dependency_Audit.md)。

审计只说明“哪些 Wacom 资产引用了什么”，不证明资产归属或授权。迁移到 `/Game/Wacom` 和 Git LFS 前必须逐批确认来源、出货授权与长期维护责任；来源未确认的音频、字体、UI 包和样例美术默认按第三方本地依赖处理。
