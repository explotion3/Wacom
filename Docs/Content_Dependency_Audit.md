---
type: content-audit
scope: wacom-content
status: active
updated: 2026-07-15
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

JSON 使用稳定排序且不写时间戳，包含分类、直接 Wacom 引用方、全部遍历引用方、最短引用链、hard/soft/game/editor/build 属性、磁盘资产状态、资产数量和资产类。连续运行可直接比较内容变化。

## 2026-07-15 基线

| 指标 | 结果 |
|---|---:|
| `/Game/Wacom` 起始 package | 161 |
| 遍历到的 `/Game` package | 270 |
| 外部 package | 109 |
| 直接由 `/Game/Wacom` 引用 | 91 |
| 仅传递引用 | 18 |
| Hard + Game 依赖 | 83 |
| `/Game/Art` | 7 |
| `/Game/Asset` | 82 |
| `/Game/DreamMaterials` | 20 |
| `/Game/L_TestBattle` | 0 |
| 其它未知 `/Game` 根目录 | 0 |

外部 package 约 `15.41 MiB`。其中以下 5 个 DreamMaterials package 在 AssetRegistry 依赖图中存在，但当前本地磁盘没有对应资产，应视为旧生成记录或缺失生成物，先用 DreamShader 真源重新生成并核对，不得作为普通 `.uasset` 迁移：

- `/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_HandTargetImpact_Default`
- `/Game/DreamMaterials/Card/MI_WacomCardSurfaceComposite_Default`
- `/Game/DreamMaterials/M_Card_Step2_Inst`
- `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartImpactPixel_Default`
- `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartTargetPreviewPixel_Default`

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
| F：DreamShader 输出 | `/Game/DreamMaterials` | 保持生成物语义；以 `.dsm`、`.dsh` 和生成脚本为真源 | 修复 5 个缺失 package，并验证可重复生成 |

迁移顺序建议为 A → B → C，再处理 D/E/F。每批必须独立执行 Content Browser Move、Fix Up Redirectors、Save All、Blueprint Compile、审计重跑和相关 PIE；不要一次移动 109 个 package。

## 完成门槛

- 所有项目自有且随游戏出货的依赖已迁入 `/Game/Wacom` 并由 Git LFS 管理。
- 所有第三方本地依赖都有版本化 manifest，记录来源、授权、版本或哈希、安装目标和验证方式。
- DreamShader 从真源可重复生成，不依赖某个 worktree 中唯一的 `.uasset` 副本。
- 报告中不再出现未登记的 `OtherGame`；最终启用 `-FailOnExternal` 时，只允许显式白名单的版本化第三方/生成依赖。
- 每批迁移均通过对应自动化、Blueprint 编译和 PIE，不留下 redirector。
