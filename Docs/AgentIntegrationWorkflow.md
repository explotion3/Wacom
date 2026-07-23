---
type: workflow
scope: wacom-agent-integration
status: active
updated: 2026-07-18
tags:
  - wacom/workflow
  - wacom/git
  - wacom/agents
  - wacom/integration
---

# Agent Integration Workflow

> [!info] 本文职责
> 本文定义多个 Codex Agent 在不同 branch / worktree 中开发后，如何把已提交成果安全集成到 `main`。项目架构和实现约束仍以 `AGENTS.md` 与对应 `Docs/` 领域文档为准。

> [!warning] 集成不是二次开发
> 集成会话只审计、排序、应用提交、解决必要冲突和验证结果。它不得借合并之机扩大功能、顺手重构或重新解释策划规则。

## 1. 目标与适用范围

本工作流解决以下问题：

- 多个 Agent 同时修改同一 UE 项目时隔离工作区。
- 明确每个 Agent 的文件和资产所有权，减少互相覆盖。
- 让功能会话消失后仍能依靠 Git commit 完成交接。
- 安全处理 C++、文档、`.uasset`、`.umap`、DreamShader 和 Git LFS 内容。
- 在进入 `main` 前形成可复查的集成结果和验证记录。

本工作流不负责：

- 决定功能设计、规则口径或架构方案。
- 替代各功能 Agent 自己的编译和定向测试。
- 自动 push、删除分支、删除 worktree 或清理用户文件。
- 维护一个容易过期的仓库内“待合并队列”。待集成批次由用户在集成会话中显式提供。

## 2. 核心原则

### 2.1 Commit 是交付单位

会话 ID 只用于找回上下文，不是集成身份。集成所需的稳定信息是：

- branch；
- worktree 路径；
- base commit；
- final commit 或有序 commit 列表；
- 依赖的其他 commit；
- changed files、测试结果和剩余风险。

功能 Agent 未提交或工作区不干净时，不视为可集成成果。

### 2.2 `main` 只接收已验证批次

推荐先在独立 `codex/integration-*` 分支完成 cherry-pick、冲突处理和验证，再让 `main` fast-forward 到该结果。不要把 `main` 当作冲突试验场。

### 2.3 不隐藏工作区状态

开始集成前必须检查：

- `git status --short --branch`；
- `git worktree list`；
- 当前 `main`、集成分支和候选分支的 HEAD；
- `git lfs status`；
- 是否有编辑器实例占用待修改的 Package。

发现未提交内容时必须识别所有者和用途。不得通过 `reset --hard`、`checkout --`、`clean` 或未经授权的 stash 抹去现场。

### 2.4 二进制资产不做文本式合并

`.uasset`、`.umap` 以及其他 UE 二进制 Package 不能可靠地逐行合并。发生同路径冲突时，必须确定权威版本，或让其中一个功能 Agent 基于最新集成结果在编辑器中重新应用改动。

通过 Unreal MCP 修改资产时，还必须遵循 `Docs/UnrealMCPWorkflow.md`。MCP endpoint 可达只证明服务在线，不证明连接的是目标 worktree；必须保留经过身份校验的 session provenance、Package allowlist 和 writer audit。Unreal MCP mutation 必须由该功能任务的主会话直接执行；subagent/disposable asset agent 产出的二进制默认不进入正式集成。

### 2.5 验证分层但不重复造假

功能 Agent 提供定向验证；集成会话验证跨提交组合；用户在主工程执行最终 PIE 手感与视觉验收。任何未执行项目必须明确标记为未验证，不能以“预计通过”代替结果。

## 3. 角色与责任

### 3.1 用户 / 主协调者

- 为每个功能 Agent 指定任务、branch/worktree 和文件边界。
- 决定有策划或二进制资产冲突时哪个版本是权威版本。
- 向集成会话提供待集成 commit 及其交接报告。
- 对 push、分支/worktree 删除和破坏性操作提供单独授权。
- 在 `main` 完成最终 PIE 验收。

### 3.2 功能 Agent

- 只修改被分配的功能范围，保护其他 Agent 和用户的工作。
- 开工前记录 base commit；需要依赖其他功能时显式报告。
- 完成编译、定向测试及适用的 Blueprint/资产验证。
- 使用 Unreal MCP mutation 时，由当前功能任务主会话直接取得 writer lease 和调用工具，交付主会话 task/thread ID、session provenance、Package allowlist 和 writer audit；不得委托 subagent 制作或保存 Unreal 二进制资产。
- 把自身成果整理为一个或一组有序、可解释的 commit。
- 交付前确认 worktree 干净。
- 不 merge `main`、不 push、不删除自己的 branch/worktree，除非用户明确要求。

### 3.3 集成 Agent

- 读取 `AGENTS.md`、本文以及候选提交涉及的领域文档。
- 审计 commit、base、依赖、文件重叠、LFS 状态和测试证据。
- 选择集成顺序并在独立 integration branch 应用提交。
- 只解决合并所必需的冲突；需要改变规则或资产权威版本时停止并请求用户决定。
- 编译并运行覆盖跨提交交互的定向回归。
- `main` 不干净或已前进时，不强行落入 `main`。
- 最终报告集成结果、冲突、验证和剩余风险。

## 4. Worktree 与 Branch 约定

推荐结构：

```text
D:\UE_Project\5.7\Wacom                         main / 最终 PIE
C:\Users\ahhh\.codex\worktrees\<id>\Wacom     功能 Agent worktree
C:\Users\ahhh\.codex\worktrees\integration\Wacom  可选集成 worktree
```

推荐 branch 命名：

```text
codex/main-menu
codex/card-presentation
codex/backpack-workspace
codex/run-level-authoring
codex/integration-20260716
```

约束：

- 每个功能 worktree 使用独立 branch。
- 一个 branch 同一时间只由一个功能 Agent 负责写入。
- 不在不同 worktree 同时 checkout 同一 branch。
- main worktree 用于最终集成和 PIE，不作为多个 Agent 的共享写目录。
- `Binaries/`、`Intermediate/`、`Saved/`、DDC 等未跟踪内容不会自动跨 worktree 同步；它们不是判断源代码是否完整的依据。
- 正式运行时依赖但被 Git 跟踪的 Content 资产应随 branch/worktree 出现；缺失时先检查 LFS，而不是从 main 目录手工复制覆盖。

## 5. 开发前的所有权分配

启动多个 Agent 前，为每个任务明确：

- 允许修改的模块和目录；
- 独占的 `.uasset/.umap`；
- 可能共享的 C++ header、配置和文档；
- 不允许触碰的正式资产；
- 依赖的其他 branch/commit；
- 最终验证入口。

高冲突资产应尽量只分给一个 Agent，例如：

- `L_Exploration.umap`；
- 同一个 WBP；
- `GM_Wacom.uasset`；
- `BP_WacomPlayerCharacter.uasset`；
- 同一个 DreamShader 父材质或默认 MI；
- `DefaultGame.ini`、`DefaultEngine.ini` 中同一配置段。

如果两个任务都必须修改同一个二进制资产，优先串行：先集成第一个提交，再让第二个 Agent 基于新 `main` 完成自己的资产修改。

## 6. 功能 Agent 交接合同

每个功能 Agent 使用以下模板交付：

```text
任务：
工作目录：
分支：
基线提交：
最终提交：
提交信息：

依赖提交：
- 无 / <commit>

改动范围：
- 代码模块和主要文件
- uasset/umap/材质
- 配置、文档和测试

验证结果：
- WacomEditor 编译：
- 定向自动化：
- Blueprint 编译：
- 资产构建/校验：
- PIE：

资产说明：
- Git LFS 文件：
- Unreal MCP role / endpoint / SessionId / task ID：
- Unreal MCP writer audit 路径与 Package allowlist：
- DreamShader/Niagara/WBP 等生成步骤：
- 是否需要在集成环境重新生成：
- 是否修改正式人工资产：

已知问题：
可能冲突：
工作区是否干净：

状态：
- 未 merge main
- 未 push
```

不完整交接的处理：

- 缺 commit：退回功能 Agent 先提交。
- 缺 base：由集成 Agent审计 merge-base，但标记交接不完整。
- worktree 不干净：只集成明确 commit，不从其工作区直接捞文件。
- 测试失败：区分本提交新增失败、已知基线失败和未运行；不能笼统写“有几个既有失败”。

## 7. 集成会话启动检查

集成 Agent 收到一批提交后，按以下顺序执行只读审计：

1. 确认仓库与 worktree 状态。
2. 确认每个 commit 在本地对象库中存在。
3. 查看 commit subject、parents、changed files、二进制/LFS 文件和 diff summary。
   如果资产由 Unreal MCP 产生，同时核对 writer audit 中的 branch、HEAD、Package allowlist、实际 dirty paths 和文件哈希。
4. 计算每个候选提交相对 `main` 的 merge-base。
5. 判断候选提交之间是否有依赖或文件重叠。
6. 判断候选提交是否已经进入 `main`，避免重复应用。
7. 检查提交是否夹带其他 Agent、缓存、生成中间物或无关文档。
8. 根据模块依赖和提交依赖确定集成顺序。

建议的审计信息包括：

```text
commit 是否存在
commit 是否为 merge commit
base 距离 main 多远
是否已被 main 包含
文件重叠集合
LFS pointer 是否完整
是否包含被 .gitignore 意外排除的运行时资产
测试证据是否足够
```

## 8. 集成策略

### 8.1 默认使用 integration branch

推荐流程：

```text
main 当前 HEAD
    -> 创建 codex/integration-<date-or-batch>
    -> 按依赖顺序应用候选提交
    -> 解决允许解决的冲突
    -> 编译与回归
    -> 确认 main 未前进且工作区干净
    -> main fast-forward 到 integration branch
```

如果 `main` 在验证期间前进：

- 不强制覆盖 main。
- 重新审计新提交与当前批次重叠。
- 对大量二进制资产批次，优先从新 main 重建 integration branch 并按原顺序重新应用，而不是盲目 rebase 冲突结果。

### 8.2 Cherry-pick 与 merge 的选择

优先 cherry-pick：

- 用户提供一个或少量明确 commit；
- branch 中夹有实验历史；
- 只需要某个完整功能切片；
- 希望逐个定位冲突和失败来源。

可以 merge 完整 branch：

- branch 的所有提交都属于同一已审阅功能；
- 提交历史本身需要保留；
- branch 没有夹带其他工作；
- 用户明确同意合并该 branch，而不是只合并某个 commit。

禁止直接 cherry-pick 一个不明 parent 语义的 merge commit。应先审计其 parents，并选择合并完整 branch 或明确的非 merge commits。

### 8.3 推荐集成顺序

实际顺序以依赖为准，通常为：

1. 底层公共 contract、数据定义和 GameplayTag。
2. 规则模块实现与迁移。
3. WacomApp 接入、输入、HUD/Screen flow。
4. Editor builder、validator 和生成脚本。
5. WBP、地图、材质及其他 Content 资产。
6. 测试与长期文档。

如果一个提交内部已经完整包含上述切片，不拆散它；只调整不同提交之间的顺序。

## 9. 冲突处理规则

### 9.1 C++、配置和 Markdown

可以由集成 Agent 解决，但必须：

- 读取 live 文件和双方 diff，不只选 `ours` 或 `theirs`；
- 保持 `AGENTS.md` 的模块与 UI/规则边界；
- 保留双方不冲突的行为、测试和文档事实；
- 修改后重新编译受影响目标；
- 如果冲突实际代表不同策划口径，停止并请求用户选择。

### 9.2 `.uasset` 和 `.umap`

不能进行逻辑三方文本合并。处理顺序：

1. 确定哪个提交拥有该资产的主要功能。
2. 审计另一个提交对同一资产的目的。
3. 选择权威资产版本。
4. 让另一项修改在权威版本上通过编辑器或定向构建脚本重新应用。
5. 重新执行 Blueprint/AssetRegistry/PIE 验证。

未经用户确认，不因“提交较新”自动判定二进制资产权威版本。

若修改来自 Unreal MCP，writer audit 只用于证明“哪个 Editor/branch/HEAD、哪个主会话 task、允许保存哪些 Package、最终产生哪些 dirty paths”；它不能自动解决同路径冲突，也不能替代用户对权威资产的决定。集成会话应先确认 `ThreadId` 对应该功能任务的主会话，再把 audit 中的 SHA-256、实际 commit blob/LFS object 和当前 main 同路径资产三者一起核对。若 audit 或交接表明 mutation 来自 subagent/disposable asset agent，默认停止并要求主会话基于权威资产重做；用户可针对既有历史资产逐次决定是否接受，但该决定不形成后续授权。

### 9.3 DreamShader 与生成材质

Wacom 正式运行时材质需要同时考虑：

- `DShader/Material/**/*.dsm`；
- `DShader/Shared/**/*.dsh`；
- 正式运行时引用的 `Content/DreamMaterials/**/*.uasset`；
- 定向 Setup 脚本和 DeveloperSettings/DataAsset 引用。

规则：

- `Content/DreamMaterials` 中正式运行时资产必须通过 Git LFS 进入版本控制，不能只提交 `.dsm/.dsh` 后假设其他 worktree 自动生成。
- 不运行可能覆盖人工调参的全量 regenerate；只执行交接报告明确要求的定向生成。
- 父材质 Settings、usage flags 和 MI 固定路径必须一起审计。
- 若提交只包含生成真源但缺运行时必需 Package，先补齐资产合同再进入 main。

### 9.4 测试和文档冲突

- 不删除另一提交新增的有效回归覆盖来换取编译通过。
- 不把新失败随意归类为“既有失败”；必须用 merge 前基线证明。
- 长期事实回写对应 `Docs/`；阶段性 Spec Kit 工件不能替代长期文档。
- `specs/` 当前可能被 `.gitignore` 忽略。提交 Spec Kit 工件时必须确认它们被显式纳入，避免只提交 `AGENTS.md` 中指向一个不存在的 plan。

## 10. Git LFS 与资产完整性

集成含 UE 资产的提交时必须确认：

- `.uasset`、`.umap` 和项目配置的其他二进制类型匹配 LFS attributes；
- commit 中保存的是 LFS pointer，所指对象在本地可用；
- Unreal MCP mutation 的 writer audit `ThreadId` 对应功能任务主会话，不是 subagent/disposable asset agent；
- Unreal MCP writer audit 报告的 Package、dirty path 和哈希与实际提交一致；
- checkout 后文件不是未解析 pointer 文本；
- AssetRegistry 能加载正式引用；
- 新 worktree 不依赖主 worktree 的未跟踪本地副本才能运行。

如果 LFS 对象缺失：

- 停止资产验证和 main 集成；
- 报告具体 path 和 object；
- 不用另一个 worktree 中来源不明的文件覆盖 pointer；
- 由提交所有者补齐 LFS 对象或重新提交。

## 11. 验证层级

### 11.1 每个功能 Agent 的最低验证

- C++ 改动：至少一次 `WacomEditor` 编译。
- 规则、生命周期、绑定或交易：对应小型自动化测试。
- WBP/Blueprint：Blueprint 编译。
- Editor builder：至少两次幂等运行和引用审计。
- 表现、材质、地图：明确 PIE 验收清单。

### 11.2 集成会话的验证

按组合风险选择：

- 完整 `WacomEditor` 编译。
- 每个候选提交自己的定向测试。
- 跨提交共享区域的回归测试。
- Blueprint 全量编译和 failed-load 检查。
- AssetRegistry、Git LFS、builder 幂等和 `git diff --check`。
- 对无法自动化的视觉/手感列出主工程 PIE 检查点。

集成会话不必机械重复所有昂贵测试，但不能跳过覆盖提交交叉影响的验证。

### 11.3 进入 main 后的验证

- 确认 `main` HEAD 包含全部预期原始 commits。
- 确认 `git status` 干净，或只剩已明确归属的用户改动。
- 在 `D:\UE_Project\5.7\Wacom` 主工程执行最终 PIE。
- PIE 失败时先在 integration branch 复现和修复；不要直接在 main 形成无法解释的散改。

## 12. Main 准入条件

只有同时满足以下条件才允许把 integration branch 落入 `main`：

- 候选 commit、依赖和集成顺序已审计。
- 所有冲突有明确解决依据。
- 二进制资产权威版本已经确认。
- Git LFS 对象完整。
- 必要编译与定向测试通过，或已有用户明确接受的已知失败基线。
- 没有新增 failed load、Blueprint error 或 Package corruption。
- main worktree 干净，或未提交内容与本批次完全不重叠且用户明确允许继续。
- main 在集成验证期间没有产生未重新审计的新提交。

默认只允许 fast-forward。需要 merge commit、非 fast-forward、历史改写或回退 main 时必须单独向用户说明理由并获得授权。

## 13. 阻塞与失败处理

以下情况必须停止自动集成并报告：

- 无法确定 `.uasset/.umap` 权威版本。
- 提交包含未说明的其他 Agent 改动。
- main 或 integration worktree 有来源不明的未提交文件。
- 候选提交依赖一个未提供的 commit。
- LFS object 缺失或资产只能依赖未跟踪文件运行。
- 冲突会改变规则、SaveGame schema、资产语义或模块依赖。
- 自动化出现无法证明为既有基线的新失败。
- 编辑器正在占用需要替换或保存的正式 Package。

集成 Agent应保留现场、说明已完成步骤和安全的下一步，不通过破坏性命令“恢复干净”。

## 14. 集成完成报告

每批集成使用以下格式：

```text
集成分支：
集成前 main：
集成后 HEAD：

已包含提交：
- <commit> <subject>

未包含提交：
- 无 / <commit + 原因>

冲突与处理：
- 文件
- 双方意图
- 采用结果和依据

验证结果：
- WacomEditor：
- 定向自动化：
- Blueprint：
- AssetRegistry / builders：
- Git LFS：
- git diff --check：
- PIE：

工作区状态：
- main：
- integration：
- 原功能 worktree：

剩余风险：
需要用户操作：
是否已进入 main：
是否 push：否，除非用户明确授权
```

## 15. 新集成会话的最小提示词

```text
你是 Wacom 项目的专用 Git 集成会话。

项目主目录：D:\UE_Project\5.7\Wacom

先完整读取：
- AGENTS.md
- Docs/AgentIntegrationWorkflow.md

只审计和集成我提供的 commits，不新增功能、不顺手重构。保护所有未提交内容；不 reset/clean，不 push，不删除 branch/worktree。main 不干净时先在 codex/integration-* 分支准备结果，只有通过审计和验证后才允许 main fast-forward。

待集成交接报告：
[粘贴功能 Agent 的标准交接报告]
```

## 16. 推荐日常流程

```text
用户划分任务与资产所有权
  -> 功能 Agent 在独立 branch/worktree 开发
  -> 功能 Agent 编译、测试、提交并给出标准交接
  -> 集成 Agent 只读审计 commits 和重叠
  -> integration branch 按依赖顺序应用
  -> 解决允许解决的冲突并验证组合
  -> main 干净且未前进时 fast-forward
  -> 用户在主工程完成 PIE
  -> 用户授权后再清理 branch/worktree 或 push
```

这条链路中，任何 Agent 的会话上下文都不是唯一真相。Git commit、长期 `Docs/`、自动化结果和可复现资产构建合同才是可持续交接依据。
