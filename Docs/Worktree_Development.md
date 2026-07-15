---
type: development-workflow
scope: wacom-worktrees
status: active
updated: 2026-07-15
tags:
  - wacom/development
  - wacom/worktree
  - wacom/content
---

# Worktree Development

> [!info] 本文职责
> 本文记录 Wacom 多 Agent / Git worktree 的本地依赖、分支隔离、编译和 PIE 验收合同。它不改变 `/Game/Wacom` 的正式资产归档规则。

## 为什么 worktree 需要本地依赖层

Git worktree 会完整检出受 Git 管理的文件，但不会复制被 `.gitignore` 排除的本地内容。Wacom 当前仍有以下本地依赖：

- `Content/Art`
- `Content/Asset`
- `Content/L_TestBattle.umap`

Codex handoff 创建 worktree 时还可能只写入 Git LFS pointer。初始化脚本会先从主仓库共享的 `.git/lfs/objects` 执行 `git lfs checkout`，并逐个拒绝仍为 pointer text 的 `.uasset/.umap`。因此“Git tracked 文件数量相同”不等于 UE 资产已经可以加载。

其中 `Content/Art` 和 `Content/Asset` 作为只读种子使用。`Content/DreamMaterials` 已整体提升为 Git LFS 资产，不再属于本地依赖；其它正式运行依赖长期仍应迁入 `/Game/Wacom`，或登记为有版本的第三方本地依赖包。

不要复制整个项目目录。完整复制会同时复制源码、配置、缓存和无版本资产，形成无法可靠合并的多份工程真相。

## 本地目录结构

Git / Codex worktree 路径保持不变，本地大文件放在 D 盘：

```text
C:/Users/ahhh/.codex/worktrees/1171/Wacom
  Content/Art            -> D:/UE_Project/5.7/WacomWorktreeData/card-presentation/LocalDependencies/Content/Art
  Content/Asset          -> D:/UE_Project/5.7/WacomWorktreeData/card-presentation/LocalDependencies/Content/Asset

C:/Users/ahhh/.codex/worktrees/0b47/Wacom
  Content/Art            -> D:/UE_Project/5.7/WacomWorktreeData/backpack-workspace/LocalDependencies/Content/Art
  Content/Asset          -> D:/UE_Project/5.7/WacomWorktreeData/backpack-workspace/LocalDependencies/Content/Asset
```

每个 worktree 的本地依赖目录彼此独立。不要让两个 worktree 指向同一份可写 Content 目录。`Content/DreamMaterials` 是普通受控目录，全部内容由 Git 管理；不同 worktree 的材质修改通过分支提交与合并同步。

## 初始化

卡牌表现 worktree：

```powershell
& 'D:\UE_Project\5.7\Wacom\Scripts\InitializeWacomWorktree.ps1' `
  -WorktreePath 'C:\Users\ahhh\.codex\worktrees\1171\Wacom' `
  -LocalDataPath 'D:\UE_Project\5.7\WacomWorktreeData\card-presentation' `
  -SeedProjectPath 'D:\UE_Project\5.7\Wacom'
```

背包 worktree：

```powershell
& 'D:\UE_Project\5.7\Wacom\Scripts\InitializeWacomWorktree.ps1' `
  -WorktreePath 'C:\Users\ahhh\.codex\worktrees\0b47\Wacom' `
  -LocalDataPath 'D:\UE_Project\5.7\WacomWorktreeData\backpack-workspace' `
  -SeedProjectPath 'D:\UE_Project\5.7\Wacom'
```

脚本只复制目标中缺失的本地依赖，不覆盖已有文件。若 Content 目标已经是真实目录而不是预期 Junction，脚本会拒绝继续，避免静默覆盖 Agent 或用户资产。

若脚本报告 LFS pointer 仍然存在，先在主仓库执行 `git lfs pull` 获取缺失对象，再重新运行初始化；不要用资源管理器从主工程覆盖 `Content/Wacom`。

## 验证

将初始化命令增加 `-Mode Verify` 即可验证：

- Git tracked 工作区干净。
- 所有 Git LFS 资产已经从 pointer materialize 为真实二进制文件。
- 本地依赖种子文件完整。
- `Content/Art`、`Content/Asset` Junction 指向当前 worktree 的独立 D 盘目录。
- 生成目录存在，并且已有 Junction 没有串到其他 worktree。

验证完成后，使用当前 worktree 自己的 `Wacom.uproject` 编译和 PIE。一次只打开一个 Unreal Editor；切换 worktree 前必须停止 PIE 并关闭编辑器。

## 所有权与合并

- Source、Config、Docs、`Content/Wacom` 和 DShader 真源由 Git 分支管理。
- `Content/Art`、`Content/Asset` 当前只作为本机 PIE 依赖，不在 worktree 间反向同步。
- DreamShader 以 `.dsm`、`.dsh` 和设置脚本为制作真源；`Content/DreamMaterials` 的全部生成 `.uasset` 同时由 Git LFS 管理。
- Agent 完成后只提交自己分支的受控内容，不提交 Junction、缓存或本地依赖副本。
- 集成 Agent 合并分支并完成自动化后，最终 PIE 优先在集成 worktree 中验收。

Ignored Content 的实际引用由 [`Content_Dependency_Audit.md`](./Content_Dependency_Audit.md) 记录。初始化脚本保证本地可运行，审计报告负责迁移决策；两者都不构成第三方资产进入仓库的授权。
