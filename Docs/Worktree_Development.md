---
type: development-workflow
scope: wacom-worktrees
status: active
updated: 2026-07-25
aliases:
  - Worktree 补水层
  - UE Worktree Hydration
tags:
  - wacom/development
  - wacom/worktree
  - wacom/content
---

# Wacom Worktree 补水层

> [!info] 本文职责
> 本文是 Wacom 新建 Git/Codex worktree 后的本地补水、D 盘生成目录、Git LFS、编译与故障排查手册。它不改变 `/Game/Wacom` 的正式资产归档规则，也不授权提交 ignored Content、Junction 或生成缓存。

## 一句话结论

新 worktree 创建后，==必须在第一次编译或打开 Unreal Editor 前==运行：

```powershell
& 'D:\UE_Project\5.7\Wacom\Scripts\InitializeWacomWorktree.ps1' `
  -WorktreePath '<新 worktree 的 Wacom 根目录>' `
  -LocalDataPath 'D:\UE_Project\5.7\WacomWorktreeData\<该 worktree 的唯一短名>' `
  -SeedProjectPath 'D:\UE_Project\5.7\Wacom'
```

脚本会完成两件不同的事：

1. 让 Git LFS 管理的 `.uasset/.umap` 从 pointer 还原为真实文件，并补齐 Git 未管理的本地依赖。
2. 将 `Binaries`、`Intermediate`、`Saved`、`DerivedDataCache` 等生成目录通过 Junction 放到该 worktree 自己的 D 盘 backing directory。

> [!warning] 不要先点击 Missing Modules 对话框里的 Yes
> 如果 worktree 还没有补水，先点 **No** 并关闭 Editor。完成补水后用本文的 `Build.bat` 命令正式编译，再打开准确的 `.uproject`。否则 C 盘 worktree 可能先生成真实的 `Binaries/Intermediate/Saved`，脚本为了保护已有内容不会自动搬走它们。

## 补水层是什么

补水层不是另一份项目，也不是让多个分支共用同一套可写缓存。它由三个路径组成：

| 名称 | 示例 | 职责 |
|---|---|---|
| Worktree | `C:\Users\ahhh\.codex\worktrees\0b47\Wacom` | Git 分支、源码、配置和受控资产的可见工程目录 |
| LocalData | `D:\UE_Project\5.7\WacomWorktreeData\backpack-next` | 该 worktree 独占的本地依赖和生成目录 backing |
| SeedProject | `D:\UE_Project\5.7\Wacom` | 提供 Git LFS 对象和 ignored 本地依赖种子的 main 工程 |

推荐布局：

```text
<Worktree>/Wacom
  Content/Art ------------------------------+
  Content/Asset ----------------------------+
  Binaries ---------------------------------+
  Intermediate -----------------------------+
  Saved ------------------------------------+--> Junction
  DerivedDataCache -------------------------+
  Plugins/DreamShader/Binaries -------------+
  Plugins/DreamShader/Intermediate ---------+
                                                |
D:/UE_Project/5.7/WacomWorktreeData/<short-name>/
  LocalDependencies/Content/Art <--------------+
  LocalDependencies/Content/Asset <------------+
  Generated/Binaries <-------------------------+
  Generated/Intermediate <---------------------+
  Generated/Saved <----------------------------+
  Generated/DerivedDataCache <-----------------+
  Generated/Plugins/DreamShader/Binaries <-----+
  Generated/Plugins/DreamShader/Intermediate <-+
```

每个 worktree 必须使用不同的 `<short-name>` 和不同的 `LocalDataPath`。例如：

```text
enemy-next        -> D:/UE_Project/5.7/WacomWorktreeData/enemy-next
backpack-next     -> D:/UE_Project/5.7/WacomWorktreeData/backpack-next
card-presentation -> D:/UE_Project/5.7/WacomWorktreeData/card-presentation
```

不要直接把 `codex/foo` 当 Windows 子目录层级；使用稳定、简短、没有斜杠的名字。

## 当前补水清单

补水真相由 [`Scripts/WorktreeLocalDependencies.json`](../Scripts/WorktreeLocalDependencies.json) 管理。

### 本地依赖

| 路径 | 策略 | 说明 |
|---|---|---|
| `Content/Art` | `ReadOnlySeed` + Junction | 从 main 缺失补齐到当前 worktree 的独立 D 盘副本 |
| `Content/Asset` | `ReadOnlySeed` + Junction | 从 main 缺失补齐到当前 worktree 的独立 D 盘副本 |
| `Content/L_TestBattle.umap` | 独立 Seed File | 缺失时复制到 worktree；不是 Junction |

`ReadOnlySeed` 是项目合同，不是 Windows ACL。脚本会检查文件是否缺失、大小是否与 SeedProject 一致，但不会把文件系统设成只读。不要在这些目录中制作需要长期保留的资产。

### 每个 worktree 独占的生成目录

| 路径 | 为什么必须隔离 |
|---|---|
| `Binaries` | DLL 与当前 branch、HEAD、编译配置和引擎版本绑定 |
| `Intermediate` | UHT、Unity、UBT、生成代码和增量编译状态不能跨分支复用 |
| `Saved` | 日志、Config、Crash、Automation、Profiling 和本地会话状态应独立 |
| `DerivedDataCache` | 当前项目策略是每 worktree 独立，避免调试资产/Shader 缓存互相污染 |
| `Plugins/DreamShader/Binaries` | DreamShader 插件 DLL 与当前 worktree 编译结果绑定 |
| `Plugins/DreamShader/Intermediate` | DreamShader 插件生成状态与当前源码绑定 |

> [!danger] 绝不能让两个 worktree 指向同一套可写 backing
> `Binaries`、`Intermediate`、`Saved`、插件生成目录和当前项目的 `DerivedDataCache` 都必须独占。`Content/Art`、`Content/Asset` 也使用独立副本，避免某个 Editor 保存或导入时改动另一条分支的本地现场。

### 不属于补水层的内容

以下内容继续由 Git/Git LFS 管理，不建立 Junction：

- `Source/`、`Config/`、`Docs/`、`Scripts/`
- `Content/Wacom/`
- `Content/DreamMaterials/`
- `DShader/`
- 其它已受 Git 管理的插件源码和配置

`Content/DreamMaterials` 已整体纳入 Git LFS。不要恢复旧的 DreamMaterials 本地 Junction，也不要通过补水脚本绕过分支提交。

## 标准流程

### 1. 准备 main 和新 worktree

关闭所有指向目标 worktree 的 Unreal Editor、`UnrealEditor-Cmd`、编译进程和 MCP writer。确认 main 的 LFS 对象完整：

```powershell
$Main = 'D:\UE_Project\5.7\Wacom'

git -C $Main status --short
git -C $Main lfs pull
git -C $Main lfs fsck
```

如果 Codex 已经创建了 worktree 和 branch，不要再次运行 `git worktree add`。

若要手工创建新 branch/worktree，可在 main 干净且目标路径不存在时执行：

```powershell
$Main = 'D:\UE_Project\5.7\Wacom'
$Worktree = 'D:\UE_Project\5.7\WacomWorktrees\world-card-activities-replan\Wacom'
$Branch = 'codex/world-card-activities-replan'

git -C $Main worktree add -b $Branch $Worktree main
```

若 branch 已存在，使用：

```powershell
git -C $Main worktree add $Worktree $Branch
```

### 2. 在第一次编译或打开 Editor 前执行 Seed

```powershell
$Main = 'D:\UE_Project\5.7\Wacom'
$Worktree = 'D:\UE_Project\5.7\WacomWorktrees\world-card-activities-replan\Wacom'
$LocalData = 'D:\UE_Project\5.7\WacomWorktreeData\world-card-activities-replan'

& "$Main\Scripts\InitializeWacomWorktree.ps1" `
  -WorktreePath $Worktree `
  -LocalDataPath $LocalData `
  -SeedProjectPath $Main `
  -Mode Seed
```

`Seed` 会：

1. 要求目标 worktree 的 tracked 文件干净。
2. 执行 `git lfs checkout` 并拒绝仍是 pointer text 的 LFS 资产。
3. 只复制缺失的 ignored 本地依赖，不覆盖已有文件。
4. 为尚不存在的生成目录创建独立 D 盘 Junction。
5. 再次确认 tracked 文件仍然干净。

脚本是 fail-closed 的。路径错误、Junction 指向其它 worktree、LFS 不完整或本地依赖不一致时会停止，不会静默覆盖。

### 3. 执行 Verify

使用完全相同的三个路径，将模式改成 `Verify`：

```powershell
& "$Main\Scripts\InitializeWacomWorktree.ps1" `
  -WorktreePath $Worktree `
  -LocalDataPath $LocalData `
  -SeedProjectPath $Main `
  -Mode Verify
```

`Verify` 是只读检查，确认：

- tracked 工作区干净；
- 所有 Git LFS 资产已经物化；
- ignored 本地依赖完整；
- Content Junction 指向当前 worktree 的 LocalData；
- 生成目录存在，已有 Junction 没有串到其它 worktree。

`Verify` 同样要求 tracked 工作区干净。开发中途有未提交源码/资产时，先审计并提交或明确处理现场，不要为了验证自动 stash。

### 4. 快速查看 Junction

```powershell
$Worktree = 'D:\UE_Project\5.7\WacomWorktrees\world-card-activities-replan\Wacom'
$Paths = @(
  'Content\Art',
  'Content\Asset',
  'Binaries',
  'Intermediate',
  'Saved',
  'DerivedDataCache',
  'Plugins\DreamShader\Binaries',
  'Plugins\DreamShader\Intermediate'
)

$Paths | ForEach-Object {
  $Item = Get-Item -LiteralPath (Join-Path $Worktree $_) -Force
  [pscustomobject]@{
    Path = $_
    LinkType = $Item.LinkType
    Target = @($Item.Target) -join ';'
  }
} | Format-Table -AutoSize
```

正常结果的 `LinkType` 应为 `Junction`，`Target` 应位于该 worktree 唯一的 `LocalDataPath` 下。

### 5. 用 worktree 自己的 uproject 编译

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  WacomEditor Win64 Development `
  -Project="$Worktree\Wacom.uproject" `
  -WaitMutex `
  -NoHotReloadFromIDE
```

编译成功后再打开：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' `
  "$Worktree\Wacom.uproject"
```

不要从另一个 worktree 的快捷方式、最近项目列表或错误的 `.uproject` 启动。若要使用 Unreal MCP，还必须按 [`UnrealMCPWorkflow.md`](./UnrealMCPWorkflow.md) 校验准确 worktree、branch、HEAD、PID 和 endpoint。

## 常见问题

### Missing Wacom Modules

表现：打开 `.uproject` 时提示模块缺失或引擎版本不同。

处理：

1. 点 **No**，关闭 Editor。
2. 执行 `Seed` 和 `Verify`。
3. 用目标 worktree 自己的 `.uproject` 执行正式 `Build.bat`。
4. 编译成功后重新打开。

不要让弹窗直接构建尚未补水的 C 盘 worktree。

### `Keeping existing generated directory on its current drive`

原因：在补水前已经打开或编译过该 worktree，目标中已有真实的 `Binaries`、`Intermediate`、`Saved` 或 DDC。

脚本会保留真实目录并给出 warning，不会自动移动或删除。此时该目录仍占用 worktree 所在磁盘。

最安全的处理是新建干净 worktree，并在第一次构建前补水。若必须迁移现有 worktree，先关闭所有 Editor/编译进程，再由主会话单独审计和迁移；不要手工批量删除或覆盖。

### `Refusing to replace an existing real directory`

这通常发生在 `Content/Art` 或 `Content/Asset` 已是普通目录。脚本拒绝将其替换为 Junction，以免丢失用户或 Agent 的本地资产。

不要直接删除。先确认目录所有权和差异，必要时备份到明确路径，再单独授权迁移或改用新 worktree。

### `Git LFS checkout is incomplete`

目标中仍有 LFS pointer，没有真实 `.uasset/.umap`：

```powershell
git -C 'D:\UE_Project\5.7\Wacom' lfs pull
git -C '<worktree>' lfs checkout
git -C '<worktree>' lfs fsck
```

之后重新执行 `Seed`。不要从资源管理器复制整个 `Content/Wacom`。

### `Existing junction points somewhere else`

该可见目录已经指向另一份 LocalData。不要改成“先用着”，也不要让两个 worktree 共用 backing。

记录实际 `Target`，确认旧 worktree 所有权后再决定修复。脚本会保持 fail-closed。

### `Tracked worktree changes must be committed or handled`

补水和验证不会自动处理用户改动。先运行：

```powershell
git -C '<worktree>' status --short
git -C '<worktree>' diff
git -C '<worktree>' diff --cached
```

确认改动所有权后再提交或另行处理。不要为了运行脚本执行 `reset --hard`、批量 restore 或自动 stash。

### Seed 内容后来更新

脚本是“补缺失 + 验证”，不是双向同步器：

- 新增的 Seed 文件可以再次运行 `Seed` 补齐。
- 已存在但内容不同的文件不会被覆盖，验证会报告差异。
- 需要刷新已存在依赖时，应明确比较和迁移，或为新 worktree 使用新的 LocalData。

这能避免 main 的 ignored 内容静默覆盖 Agent 或用户现场。

## 日常使用和收尾

- 每次 Editor 生命周期只打开一个准确 worktree；编译前关闭 Editor。
- 切 branch、更新 HEAD 或集成 main 前先关闭该 worktree 的 Editor/MCP writer。
- 只提交 Git 受控内容；不要提交 Junction、LocalData、缓存、日志或 ignored 本地依赖。
- 删除 worktree 不等于可以立即删除 LocalData。只有确认 branch 已集成、worktree 已移除且没有任何 Editor/进程使用后，才能另行授权清理 D 盘 backing。
- `Content/Art`、`Content/Asset` 的长期引用与迁移决策看 [`Content_Dependency_Audit.md`](./Content_Dependency_Audit.md)。
- 正式内容归档边界看 [`Content_Organization.md`](./Content_Organization.md)。

## 新 worktree 检查清单

- [ ] main 与目标 branch/commit 已确认。
- [ ] 目标 worktree tracked clean。
- [ ] main 已执行 `git lfs pull` 和 `git lfs fsck`。
- [ ] LocalData 使用唯一、稳定、位于项目树之外的 D 盘路径。
- [ ] 第一次打开 Editor/编译前已执行 `-Mode Seed`。
- [ ] `-Mode Verify` 通过。
- [ ] 八个目录的 Junction Target 均属于当前 LocalData。
- [ ] 使用目标 worktree 自己的 `Wacom.uproject` 编译成功。
- [ ] 若使用 MCP，已通过 endpoint/worktree/branch/HEAD/PID 校验。
