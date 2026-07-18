---
type: workflow
scope: wacom-unreal-mcp
status: active
updated: 2026-07-18
tags:
  - wacom/workflow
  - wacom/unreal
  - wacom/mcp
  - wacom/assets
---

# Unreal MCP Workflow

> [!info] 目标
> 本文定义 Wacom 的 Unreal Editor / MCP 生命周期、固定端点、身份校验和二进制资产写入合同。Git 集成仍以 `Docs/AgentIntegrationWorkflow.md` 为准。

## 1. 工作模型

主会话长期负责架构、代码、Git、冲突判断和最终验收。Unreal MCP 只是连接某个明确 Editor 实例的资产操作通道，不是项目身份来源。

Subagent 不是固定前置条件：

- 当前会话已经拥有目标 named endpoint 的 Unreal 工具，并且启动脚本的身份校验通过时，主会话可以直接执行资产操作。
- 当前会话启动时 MCP 离线、没有暴露 Unreal 工具，或 Editor 已关闭重开时，为本次 Editor 生命周期新建一个 disposable asset agent。
- 多个 Editor 同时存在或修改高风险二进制资产时，优先使用 disposable asset agent，缩小误连和工具上下文残留风险。
- Editor 关闭后，不把旧 asset agent 继续用于下一次 Editor 生命周期。

无论由主会话还是 asset agent 调用，身份校验、写锁和 Package allowlist 完全相同。

## 2. 固定端点

端点真相位于 `Scripts/UnrealMcp/Endpoints.json`：

| Role | Codex endpoint | Port | 默认权限 |
|---|---|---:|---|
| `main` | `ue_wacom_main` | 8100 | read-only |
| `card` | `ue_wacom_card` | 8110 | writer-eligible |
| `enemy` | `ue_wacom_enemy` | 8120 | writer-eligible |
| `backpack` | `ue_wacom_backpack` | 8130 | writer-eligible |
| `run` | `ue_wacom_run` | 8140 | writer-eligible |
| `integration` | `ue_wacom_integration` | 8190 | read-only |

端点按长期职责命名，不使用 `1171` 一类临时 worktree ID。Role 只决定端口和默认权限；每次启动仍必须显式提供准确 `ProjectRoot` 和 `ExpectedBranch`。

`main` 和 `integration` 默认禁止取得写锁。确需写入时，必须先有用户对本次资产范围的明确授权，再传 `-AllowProtectedRoleWrite`；该参数不代表永久授权。

## 3. 一次性 Codex 配置

运行以下命令生成用户级配置片段：

```powershell
& 'D:\UE_Project\5.7\Wacom\Scripts\Invoke-WacomUnrealMcp.ps1' `
    -Action PrintCodexConfig
```

把输出的 `[mcp_servers.ue_wacom_*]` 段放入 `C:\Users\ahhh\.codex\config.toml`。这些 endpoint 均为 `required = false`，没有启动对应 Editor 时不应阻止 Codex 启动。

首次完成这些用户级配置后重启 Codex 一次，让长生命周期主会话能够发现 named endpoint。以后 Editor 关闭重开不需要重启主会话；只有当前 task 没有加载对应 Unreal 工具时，才为新 Editor 生命周期创建 disposable asset agent。

用户级 `config.toml`、本机进程号、SessionId 和 writer lease 不进入版本控制。仓库只跟踪端点定义、启动工具和本文合同。

## 4. 启动与身份校验

以 card worktree 为例，先固定本轮身份参数：

```powershell
$ProjectRoot = 'D:\UE_Project\5.7\WacomWorktrees\card-presentation\Wacom'
$Branch = 'codex/card-presentation'
$Mcp = Join-Path $ProjectRoot 'Scripts\Invoke-WacomUnrealMcp.ps1'
```

新建 worktree 不共享 `Binaries/`。第一次启动 Editor 前，先确认没有 Editor/端口/写锁占用，再正式编译一次该 worktree：

```powershell
& $Mcp -Action AssertClosedForBuild `
    -Role card `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $Branch

& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
    WacomEditor Win64 Development `
    -Project="$ProjectRoot\Wacom.uproject" `
    -WaitMutex `
    -NoHotReloadFromIDE
```

`Start` 会检查项目和仓库内插件声明的 Editor module DLL。缺失时直接停止并提示先编译，不打开 Unreal 的 “Missing Modules” 重建弹窗。

编译成功后启动：

```powershell
& $Mcp -Action Start `
    -Role card `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $Branch
```

启动器会：

1. 校验 `Wacom.uproject`、Git worktree 根、branch、HEAD、dirty 状态和 `git lfs fsck`。
2. 拒绝被其他进程占用的 role port，以及已经由别的方式打开的同一 `.uproject`。
3. 使用 UE 5.8 参数 `-ModelContextProtocolStartServer` 和 `-ModelContextProtocolPort=<port>` 启动 Editor。
4. 记录唯一 SessionId、Editor PID、进程启动时间、完整 `.uproject`、branch 和 HEAD。
5. 完成 MCP initialize、`tools/list`，并通过 toolset gateway 实际调用 `EditorToolset.EditorAppToolset.IsPIERunning` 后才写入有效 session 记录。

默认拒绝 dirty worktree。只有未提交内容的所有者和边界已经明确时才能传 `-AllowDirty`；该参数不会忽略或清理这些内容。

每轮 MCP 调用前先执行：

```powershell
& $Mcp -Action AssertReady `
    -Role card `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $Branch
```

`AssertReady` 同时校验 PID、进程启动时间、命令行 `.uproject`、SessionId、端口 owner、branch、HEAD 和 MCP 工具健康。仅仅调用 `IsPIERunning` 不能证明连到正确 worktree。

如果 Editor 正常启动但 MCP 工具列表缺失，可在 Editor 控制台执行一次：

```text
ModelContextProtocol.RefreshTools
```

不要把 `RefreshTools` 当成每次启动的固定步骤。

## 5. 单写入者与 Package allowlist

资产 mutation 前必须取得 writer lease。allowlist 使用不带扩展名的完整 `/Game/...` Package 路径：

```powershell
$TaskId = '<current-codex-task-id>'

& $Mcp -Action AcquireWriter `
    -Role card `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $Branch `
    -ThreadId $TaskId `
    -Packages @(
        '/Game/UI/WBP_CardView',
        '/Game/FX/NS_CardDraw'
    )
```

取得写锁后：

- 同一 Editor 同时只允许这个 task 执行 mutation。
- 优先调用 UMGToolSet、BlueprintTools、Niagara Toolsets 等专用工具，不用通用脚本绕过边界。
- 只保存 allowlist 中的 Package；创建的关联资产也必须在取得写锁前列入 allowlist。
- 不在 Editor 打开期间切 branch、更新 HEAD 或执行 C++ 编译。
- 保存后立即检查 `git status --short`，再释放写锁。

默认情况下，Writer 不接管取得租约前已经 dirty 的 allowlist Package。只有这些二进制的来源、当前哈希和本轮所有者都已明确，且任务确实需要继续修改同一路径时，才可显式使用 `-AllowExistingDirtyPackages -Reason '<原因>'`。该模式仍会把接管路径及其租约前 SHA-256 写入 lease/audit，并继续拒绝白名单外变化；它不是忽略 dirty 状态或允许批量覆盖的开关。

```powershell
& $Mcp -Action ReleaseWriter `
    -Role card `
    -ThreadId $TaskId
```

释放时会比较 lease 前后的 dirty 集合和哈希。出现 allowlist 外新文件、已有 dirty 文件被改动、branch/HEAD 改变或 LFS 校验失败时，操作会 fail closed：不清理资产、不删除 lease，等待人工审计。成功时，本机 `%LOCALAPPDATA%\Wacom\UnrealMcp\Audits` 会生成 JSON，包含：

- role、endpoint、SessionId、PID、task ID、branch 和 HEAD；
- `/Game/...` Package 与对应 `.uasset/.umap` 路径；
- mutation 前后 dirty paths；
- `git status`、`git lfs status` 和 `git lfs fsck` 结果；
- allowlist 文件 mutation 前后的 SHA-256。

写锁是本机安全闩，不替代 Git commit、交接报告或集成审计。

如果 Editor 已退出、正常释放因 HEAD 改变或越界资产而无法完成，先人工审计并处理 Git/资产现场。只有确认不再需要该 lease 且用户同意解除门禁后，才能显式归档；此命令只归档本机 lease 和审计记录，不修改或清理任何项目文件：

```powershell
& $Mcp -Action ArchiveStaleWriter `
    -Role card `
    -ThreadId $TaskId `
    -Reason '人工审计结论' `
    -ConfirmStaleWriterArchive
```

Editor、role port 或同一 `.uproject` 仍在运行时，归档会被拒绝。

## 6. 关闭 Editor 与编译

脚本不会强制关闭 Editor。保存、释放 writer lease 后，由用户或当前会话正常关闭对应 Editor，再运行：

```powershell
& $Mcp -Action AssertClosedForBuild `
    -Role card `
    -ProjectRoot $ProjectRoot `
    -ExpectedBranch $Branch
```

只有以下条件全部成立才返回 `ReadyForBuild = true`：

- writer lease 不存在；
- session 对应 PID 已退出；
- role port 不再监听；
- 没有其他 Editor 进程指向同一 `.uproject`。

之后才能运行 `WacomEditor` 编译。编译完成、需要再次编辑资产时，重新启动新的 Editor/MCP 生命周期。

如果启动超时，脚本会保留 Editor 供检查，不会自动结束进程；此时 `Status` 可能显示 `UntrackedPortOwner`。先识别进程与 `.uproject`，正常关闭后再执行编译门禁。

## 7. 二进制资产交付与集成

功能 Agent 的交接报告必须附上：

- role、endpoint、SessionId 和 task ID；
- writer audit JSON 路径；
- mutation Package allowlist；
- 实际变化的 `.uasset/.umap`；
- Blueprint 编译、资产校验或 PIE 结果；
- `git lfs status` 和 `git lfs fsck` 结果。

集成会话按以下规则处理：

1. 根据 commit 而不是运行中的 Editor 取文件。
2. 对照 main 检查同路径 `.uasset/.umap` 是否已经变化。
3. 同路径二进制发生重叠时停止；由用户指定权威版本，或让功能 Agent 基于权威版本重放编辑器操作。
4. 不对 Unreal Package 做文本合并，不从其他 worktree 手工复制来源不明的资产。
5. 重新检查 LFS attributes、pointer/object 完整性和 commit 中的实际路径。

`main` 或 `integration` 上的 MCP 默认只用于只读检查。不得用它“顺手修好”候选提交中的二进制冲突。

## 8. 第一阶段运行清单

- [ ] 用户级 Codex 配置已注册 6 个 named endpoint。
- [ ] Editor 只通过准确 worktree 的启动命令打开。
- [ ] `AssertReady` 在每次 MCP 操作前通过。
- [ ] Mutation 前取得唯一 writer lease 和完整 Package allowlist。
- [ ] 保存后 `ReleaseWriter` 成功并保留 audit JSON。
- [ ] 编译前 `AssertClosedForBuild` 通过。
- [ ] 功能交接包含 MCP provenance、实际二进制路径、LFS 与验证结果。
- [ ] 集成遇到同路径二进制重叠时停止并确定权威版本。

第一阶段不建设常驻 Gateway。只有固定端点仍频繁误连、需要动态发现大量 Editor，或确实需要统一路由和认证时，再单独设计 Gateway。
