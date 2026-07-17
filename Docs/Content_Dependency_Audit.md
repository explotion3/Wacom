---
type: content-audit
scope: wacom-content
status: active
updated: 2026-07-17
tags:
  - wacom/content
  - wacom/assets
  - wacom/editor
---

# Wacom Content Dependency Audit

> [!info] 本文职责
> 本文记录 `/Game/Wacom` 对 ignored Content 的可复现审计方法、当前基线和迁移分批。审计结果不等于资产所有权证明；没有确认来源和授权前不得把外部资产迁入 Git LFS。

## 审计入口

`WacomEditor` 提供只读 Commandlet：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  '<ProjectRoot>\Wacom.uproject' `
  -run=WacomAuditContentDependencies `
  -Unattended -NoPause -NoSplash -NullRHI
```

默认扫描 `/Game/Wacom`，报告写入 `Saved/Reports/WacomContentDependencyAudit.json`。可选参数：

- `-Root=/Game/Wacom`：覆盖扫描根路径；只接受 `/Game` package path。
- `-Output=Saved/Reports/Custom.json`：覆盖报告路径，相对路径以项目根目录解析。
- `-FailOnExternal`：存在任一外部 package 时返回退出码 `2`，用于未来 CI gate；当前迁移完成前不要默认启用。
- `-FailOnPlaceholder`：`/Game/Wacom/Art/Placeholders` 下存在任一 package 时返回退出码 `3`；开发审计默认只报告，发布审计必须启用。

JSON schema v2 使用稳定排序且不写时间戳，除原有分类、引用方、最短引用链、依赖属性、磁盘状态、数量和资产类外，还显式输出稳定排序的 `placeholder_packages`。连续运行可直接比较内容变化。

## 2026-07-16 基线

| 指标 | 结果 |
|---|---:|
| `/Game/Wacom` 起始 package | 163 |
| 遍历到的 `/Game` package | 277 |
| 外部 package | 114 |
| 直接由 `/Game/Wacom` 引用 | 92 |
| 仅传递引用 | 22 |
| Hard + Game 依赖 | 88 |
| `/Game/Art` | 7 |
| `/Game/Asset` | 82 |
| `/Game/DreamMaterials` | 25 |
| `/Game/L_TestBattle` | 0 |
| 其它未知 `/Game` 根目录 | 0 |

`Content/DreamMaterials` 当前共 61 个本地 `.uasset`，已整体通过 Git LFS 纳入版本控制；其中 24 个 Material / Material Instance 出现在本次 `/Game/Wacom` 依赖图中并有磁盘资产。这里的“外部”只表示 package 不在 `/Game/Wacom` 根路径，不表示这些 package 未受版本管理。

当前仍有以下 1 个 DreamMaterials package 在 AssetRegistry 依赖图中存在但本地磁盘没有对应资产，应视为旧生成记录或缺失生成物：

- `/Game/DreamMaterials/M_Card_Step2_Inst`

## TrainingWarrior 正式素材晋升（2026-07-17）

项目所有者于 2026-07-17 明确确认 `/Game/Art/PaperAssets/Party/BattleWarrior` 中本批选用素材可以进入 Git LFS 并随项目发布。正式包只选用 Idle、Attack、Block、Cleave、Downed 五个 Flipbook 及其递归 Sprite / Texture 依赖；Item 动画未迁移。

| 项目 | 路径 / 结果 |
|---|---|
| 原始本地目录 | `/Game/Art/PaperAssets/Party/BattleWarrior` |
| 正式目标 | `/Game/Wacom/Art/Enemies/TrainingWarrior` |
| 正式闭包 | 5 Flipbook、30 Sprite、1 Texture，共 36 个 package |
| 语义重命名 | Downed → Destroyed；其余为 Idle / Attack / Block / Cleave |
| 复制方式 | `IAssetTools::AdvancedCopyPackages`，统一重写内部引用；不做文件系统复制 |
| 依赖结果 | scoped audit 对 `/Game/Art`、`/Game/Asset`、`/Game/DreamMaterials` 为 0 |

命令为 `-run=WacomBuildEnemyPack -Pack=TrainingWarrior -PromoteArt`；`-ForceArtRefresh` 仅用于显式重晋升。目标完整时命令跳过复制。正式 DataAsset、Style 与 Host 只引用 `/Game/Wacom`，因此普通 `WacomBuildEnemyPack` 和 `WacomRegenerateContent` 不要求 ignored PaperAssets 存在。本地 Snake/BattleWarrior PIE 脚本继续保留为调试入口，但不再代表正式 TrainingWarrior。

## Snake 受控占位美术包（2026-07-17）

项目所有者于 2026-07-17 允许 `/Game/Art/PaperAssets/Enemies/Slime` 的选定闭包进入 Git LFS，仅用于 Snake 的版本化占位、构建和开发验证；这不构成正式出货许可。晋升入口只读取 `Slime__Idle`、四个 Frame Sprite 和一个 Texture，并在正式副本上生成 Head / Body / Tail 三个单帧 Destroyed Flipbook。

| 项目 | 路径 / 结果 |
|---|---|
| 原始本地目录 | `/Game/Art/PaperAssets/Enemies/Slime` |
| 受控占位目标 | `/Game/Wacom/Art/Placeholders/Enemies/Snake` |
| 占位闭包 | 1 Idle Flipbook、4 Sprite、1 Texture、3 生成的 Destroyed Flipbook，共 9 个 package |
| 晋升命令 | `-run=WacomBuildEnemyPack -Pack=Snake -PromotePlaceholderArt` |
| 日常重建 | `-run=WacomBuildEnemyPack -Pack=Snake`，只读取已提交 `/Game/Wacom` 资产 |
| 依赖结果 | Snake Host 与 Placeholder 闭包对 `/Game/Art`、`/Game/Asset`、`/Game/DreamMaterials` 为 0 |
| 发布门槛 | `WacomAuditContentDependencies -FailOnPlaceholder` 必须失败，直到占位包被正式素材替换并删除 |

Snake 不接受 `-PromoteArt`，避免把 Slime 占位冒充正式美术；`-ForceArtRefresh` 只允许与 `-PromotePlaceholderArt` 同用。正式蛇素材到位后应晋升到 `/Game/Wacom/Art/Enemies/Snake`，切换 `BP_EnemyHost_Snake` 引用并删除已知生成的 Placeholder package，发布审计才允许通过。

主要直接引用源：

- `/Game/Wacom/UI/Card/WBP_CardView`：30 个外部 package。
- `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`：29 个。
- `/Game/Wacom/UI/Card/WBP_CardEffectBadge`：15 个。
- `/Game/Wacom/UI/Card/DA_CardDetailTheme_Default`：9 个。
- `/Game/Wacom/Blueprint/BP_RunTunnelGrassPaperLayer`：7 个。
- `/Game/Wacom/Maps/L_Exploration`：7 个。

## 迁移分批

| 批次 | 当前目录 | 建议目标 / 处理方式 | 前置条件 |
|---|---|---|---|
| A：卡面正式素材 | `/Game/Asset/Card_Luo` | 若为项目自有，迁入 `/Game/Wacom/UI/Card/Art` | 确认来源、出货授权和 Git LFS 许可 |
| B：Run 隧道素材 | `/Game/Asset/Plane` | 若为项目自有，迁入 `/Game/Wacom/Run/Tunnel/Art` | 同上；迁移后验证 `L_Exploration` |
| C：材质实验贴图 | `/Game/Asset/Chong_CardAsset` | 正式使用则迁入 `/Game/Wacom/Material/Textures`；仅调试则删除引用 | 确认 `NewMaterial*` 是否正式保留 |
| D：音频、字体、UI 包 | `/Game/Asset/Audio`、`Fonts`、`UI` | 默认按第三方依赖处理，建立带版本、来源、哈希和安装路径的 manifest | 明确每一包的授权；不得按目录整体假定自有 |
| E：Boar 调试占位 | `/Game/Art/PaperAssets/Enemies/Boar` | 优先替换或移除 `BP_SnakeHost_Debug` 的样例依赖 | 确认是否为第三方样例及 Debug Host 去留 |
| F：DreamShader 输出 | `/Game/DreamMaterials` | `.dsm/.dsh` 保持制作真源；整个生成目录同时以 Git LFS 管理 | 处理 `M_Card_Step2_Inst` 剩余缺失引用，并验证可重复生成 |

迁移顺序建议为 A → B → C，再处理 D/E/F。每批必须独立执行 Content Browser Move、Fix Up Redirectors、Save All、Blueprint Compile、审计重跑和相关 PIE；不要一次移动 113 个 package。

## 完成门槛

- 所有项目自有且随游戏出货的依赖已迁入 `/Game/Wacom` 并由 Git LFS 管理。
- 所有第三方本地依赖都有版本化 manifest，记录来源、授权、版本或哈希、安装目标和验证方式。
- DreamShader 从真源可重复生成；整个 `/Game/DreamMaterials` 都有版本化 `.uasset`，不依赖某个 worktree 中唯一的副本。
- 报告中不再出现未登记的 `OtherGame`；最终启用 `-FailOnExternal` 时，只允许显式白名单的版本化第三方/生成依赖。
- 每批迁移均通过对应自动化、Blueprint 编译和 PIE，不留下 redirector。
