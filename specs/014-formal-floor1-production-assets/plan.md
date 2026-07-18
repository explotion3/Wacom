# Implementation Plan: Floor 1 Production 46 DataAsset 播种与校验

**Branch**: `codex/formal-floor1-production-assets` | **Date**: 2026-07-18 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/014-formal-floor1-production-assets/spec.md`

## Summary

在最新干净 main 后继 `d7c6b70b1dc1006f08fff8c598d58e65f53a5813` 上，为 Spec 011 的 38 个 Floor 1 核心 DataAsset 与 Spec 013 的 8 张击倒分支卡提供一个 manifest-driven、seed-only 的 `WacomEditor` 制作服务。服务默认只读检查，只有显式 `SeedMissing` 才创建缺失 package；已有正确 class 资产不保存、不回写人工调参。默认校验守稳定身份和结构，`CompareSeedDefaults` 仅用于初次验收。实际 46 个二进制 Package 在 run/8140 Unreal MCP 会话中按 `12/19/15` 三组取得 writer lease 后串行创建和审计。

## Wacom Domain Context

**Primary Domain**: Data/card authoring / Battle content / Run content / Editor tooling / Tests

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`
- [x] Spec 011/012/013 的规格、计划、数据模型、合同、任务与验证台账
- [x] live Card/Enemy/Behavior/Intent/Encounter/Event/Pickup/Shop schema、validator、builder 和测试

**Docs To Update**:

- [ ] `Docs/WacomData.md`：记录 46 资产已实现、首次 seed 与后续结构权威边界。
- [ ] `Docs/WacomDataAuthoring.md`：记录命令、三组、writer lease、report、double-run、禁止覆盖与人工调优合同。
- [ ] `Docs/WacomBattle.md`：记录 11 Part 的正式双分支引用已落盘并通过 FormalProduction。
- [ ] `Docs/WacomRun.md`：记录正式 Event/Pickup/Shop Definition 已存在，但尚未绑定 Production Floor/map。
- [ ] `Docs/Architecture.md`：记录 seed-only Editor service、commandlet/Editor bridge 共用单一 manifest，runtime 依赖不变。
- [ ] `Docs/TODO.md`：关闭 46 DataAsset 实现任务，保留正式 Journey/Floor/map/Host/PIE。
- [ ] `Docs/Questions.md`：把世界资产权威问题前置条件更新为 46 资产已完成审计，不替用户选择地图权威。
- [ ] `Docs/Roadmap.md`：将 Floor 1 击倒奖励卡从待制作改为待平衡/场景接入。

**Owning Module(s)**: `WacomEditor`、`WacomTests`、Content packages。`WacomData/WacomBattle/WacomRun/WacomApp` 源码不修改。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomCore/WacomData/WacomBattle (existing dependencies only)
WacomTests  -> WacomEditor/runtime modules as test harness
```

不修改 `Build.cs`，不引入 ToolsetRegistry 或 MCP runtime 依赖。Editor 内触发通过 WacomEditor 自身控制台命令复用构建服务；MCP 只负责准确 Editor 身份、UI/console 调用和 writer audit。

## Technical Context

**Language/Engine**: C++17、Unreal Engine 5.8、Markdown、JSON

**Primary UE Systems**: `UPrimaryDataAsset`、AssetRegistry、`UPackage::SavePackage`、Editor commandlet、`IConsoleManager` editor-only command、Data Validation、Automation Tests、Git LFS、Unreal MCP

**Storage/State**: 46 个静态 DataAsset；无运行时/SaveGame 状态

**Runtime Contracts**: 完全复用现有 Card/Enemy/Behavior/Encounter/Event/Pickup/Shop schema 和 Battle/Run resolver

**Testing**: transient manifest/结构/参数测试；真实 46 资产 smoke；通用 Data validators；Battle RuleContent/KnockdownReward；Run Event/Pickup/Shop/reward；AssetRegistry/failed-load；双跑、hash、LFS/MCP audit

**Target Platform**: Windows Editor，默认 Unity `WacomEditor Win64 Development`

**Performance Goals**: 46 条线性 manifest；inspect 不产生 package dirty；无 Tick、runtime load 或游戏启动成本

**Constraints**: seed-only；existing assets never saved；no force/delete；三组串行；所有 Unreal 命令 `-NoDreamShaderEditorBridge`；Editor 生命周期内不编译/切 branch/更新 HEAD

**Scale/Scope**: 46 new packages、4 read-only dependencies、1 shared service、1 commandlet、1 editor console bridge、2 small test specs、8 durable docs

**Blueprint Exposure Strategy**: 无 Blueprint API。构建服务和 console bridge 位于 `WacomEditor/Private`；必要的 tests-only view 仅暴露非反射 editor automation API。

**Data/GameplayTag Impact**: 创建现有 DataAsset class 实例；0 schema/tag change

**Save/Load Impact**: 无

**UI/App Lifecycle Impact**: 无游戏 UI。Editor console bridge 只在 WacomEditor module 生命周期注册。

## Constitution Check

*GATE: PASS before research; PASS after design.*

- **Docs and AGENTS Are the Rule Truth**: live Docs、Spec 011/012/013 与 schema 已读取；完成事实同步到八份长期文档。
- **Wacom Module Boundaries Are Mandatory**: 静态内容不进入 runtime state；制作逻辑只在 Editor；测试只在 WacomTests；Build.cs 不变。
- **Domain Rules Before Presentation**: 无 UI 规则；46 个资产只配置现有领域合同。
- **Data, GameplayTags, and Authoring Stay Explicit**: exact package/class/ID/group/dependency、稳定/可调字段、writer allowlist 全部枚举；0 tag/schema change。
- **Reusable Systems Over One-Off Work**: 一个 manifest/expected-object/comparator 服务同时服务 commandlet、Editor bridge 和 tests；没有 46 份散落脚本。
- **Validation Is Part of the Slice**: 编译、Automation、Data Validation、AssetRegistry、failed-load、double-run、hash、LFS 和 MCP audit 均为完成条件。

## Phase 0: Research

结论见 [research.md](./research.md)：

1. 资产权威是“仅首次播种”，不是可重复重建器。
2. 默认结构校验与 `CompareSeedDefaults` 必须分层。
3. 46 条 manifest 是 package、配置、依赖和 comparator 的单一真相。
4. commandlet 与 Editor console bridge 复用同一服务，以满足 MCP writer lease。
5. 三组创建顺序固定为 Cards → EnemyGraph → NodeDefinitions。
6. 既有通用 validator 与 Floor1 exact comparator 分工明确。
7. 报告、退出码、失败现场与 hash/LFS 证据必须机器可读。
8. Spec 013 的 Guardian Destroy 列错位按 live 长期 Docs 纠正为 AllEnemyParts。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：manifest entry、build options/report、稳定与可调字段、状态转换和 seed 文本默认值。
- [contracts/asset-manifest.md](./contracts/asset-manifest.md)：46 条 package/class/ID/group/依赖。
- [contracts/seeder-command-contract.md](./contracts/seeder-command-contract.md)：CLI/editor 参数、退出码、报告与 no-overwrite。
- [contracts/validation-contract.md](./contracts/validation-contract.md)：结构、strict、通用 validator、forbidden closure。
- [contracts/mcp-write-contract.md](./contracts/mcp-write-contract.md)：run/8140、三组 writer lease、audit/close/build 门禁。
- [quickstart.md](./quickstart.md)：工作区、命令、checkpoint、hash、跳过项和最终结果台账。

## Project Structure

### Documentation

```text
specs/014-formal-floor1-production-assets/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── tasks.md
├── quickstart.md
├── contracts/
│   ├── asset-manifest.md
│   ├── seeder-command-contract.md
│   ├── validation-contract.md
│   └── mcp-write-contract.md
└── checklists/
    ├── requirements.md
    └── production-assets.md
```

### Source and content

```text
Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.h
Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentBuilder.cpp
Source/WacomEditor/Private/Commandlets/WacomBuildFormalFloor1ContentCommandlet.h
Source/WacomEditor/Private/Commandlets/WacomBuildFormalFloor1ContentCommandlet.cpp
Source/WacomEditor/Private/ContentBuilders/FormalFloor1ContentEditorCommand.cpp
Source/WacomEditor/Public/Testing/WacomFormalFloor1ContentAutomationTestView.h   # only if required
Source/WacomEditor/Private/Testing/WacomFormalFloor1ContentAutomationTestView.cpp
Source/WacomTests/Private/Editor/FormalFloor1ContentManifestSpec.cpp
Source/WacomTests/Private/Data/FormalFloor1ContentAssetSpec.cpp
Content/Wacom/Data/.../SerpentWood/*.uasset                                 # exact 46 only
```

**Structure Decision**: 不改共享 `BuildDataAsset()` 的覆盖语义，避免影响 Snake/TrainingWarrior。FormalFloor1 service 使用自己的“load existing or create missing”路径：existing asset 只读；missing asset 用 transient expected 配置后创建并保存。Exact comparator 与 configurator 和 manifest 同文件私有实现，测试通过最小 automation view 读取结果。

## Implementation Checkpoints

### Checkpoint 1 — Spec、manifest 与无写入验证

- 完成 Spec 014 全套工件、46 条表、稳定/可调边界、MCP 计划与检查清单。
- 实现 manifest、参数解析、JSON report、inspect-only 和 transient expected/config comparator。
- 新增小型 `FormalFloor1ContentManifestSpec.cpp`，不依赖目标资产存在。
- `AssertClosedForBuild` 后编译 WacomEditor；运行 `Wacom.Editor.FormalFloor1Content.Manifest`。
- 此 checkpoint 不启动 Editor、不创建 `.uasset`；提交第一笔 `feat(editor): add formal floor1 content seeder`。

### Checkpoint 2 — Seed-only 创建服务与通用校验

- 实现 `SeedMissing`：预检整组依赖、只创建 missing、existing never save、wrong class fail。
- 实现 Card/Behavior/Part/Enemy/Encounter/Event/Pickup/Shop expected configurator。
- 默认结构 comparator 调用通用 validator，再检查 Floor1 exact stable structure；strict 比较所有 editable seed 属性。
- commandlet 与 `Wacom.BuildFormalFloor1Content` Editor console command 共用参数/服务。
- 重新编译并运行 manifest、validator、RuleContent transient tests；仍不创建目标资产。

### Checkpoint 3 — MCP Cards 组（12）

- 启动前 `AssertClosedForBuild` 和编译结果已通过；用脚本 `Start -Role run`。
- `AssertReady`，取得 Cards 12-package writer lease。
- 通过 MCP Editor console bridge 执行 `SeedMissing Group=Cards CompareSeedDefaults`，保存 12 个 package。
- 检查 Git status、报告与 hashes；`ReleaseWriter`；运行只读 inspect/strict 和 Cards 真实资产测试。

### Checkpoint 4 — MCP EnemyGraph 组（19）

- `AssertReady`，取得 Behavior 4 + Part 11 + Enemy 4 writer lease。
- 执行 `SeedMissing Group=EnemyGraph CompareSeedDefaults`，验证 FormalProduction、24 Intent、4 Enemy graph。
- 检查 Git/report/hash；`ReleaseWriter`；运行 Enemy/Behavior/Part 与 KnockdownReward 测试。

### Checkpoint 5 — MCP NodeDefinitions 组（15）

- `AssertReady`，取得 Encounter 6 + Event 4 + Pickup 4 + Shop 1 writer lease。
- 预检四张只读 Shop card hash；执行 `SeedMissing Group=NodeDefinitions CompareSeedDefaults`。
- 检查 Git/report/hash；`ReleaseWriter`；运行 Encounter/Event/Pickup/Shop/Run 测试。

### Checkpoint 6 — Double-run、关闭 Editor 与最终验证

- 仍在同一 Editor 生命周期中对三组再次执行 `SeedMissing`；均应 0 create/0 save，hash 不变。
- 最终 `AssertReady` 只读检查、收集 session/audit/report；正常关闭 Editor。
- `AssertClosedForBuild` 后最终默认 Unity 编译。
- 运行全部聚焦 Automation、AssetRegistry/failed-load/forbidden reference、Git LFS fsck、`git diff --check`。
- 同步长期 Docs，提交第二笔 `feat(content): seed formal floor1 production assets`，确认 worktree 干净。

## Validation Plan

### Build gate

```powershell
& '<ProjectRoot>\Scripts\Invoke-WacomUnrealMcp.ps1' -Action AssertClosedForBuild `
  -Role run -ProjectRoot '<ProjectRoot>' `
  -ExpectedBranch 'codex/formal-floor1-production-assets'

& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  WacomEditor Win64 Development -Project='<ProjectRoot>\Wacom.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

### Commandlet read-only/CI entry

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  '<ProjectRoot>\Wacom.uproject' `
  -run=WacomBuildFormalFloor1Content -Group=All `
  -CompareSeedDefaults -Report='<ProjectRoot>\Saved\FormalFloor1\inspect.json' `
  -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
```

正式资产 mutation 不用 standalone commandlet；在 run/8140 writer lease 内通过 MCP 调用 Editor console bridge。commandlet 的 `SeedMissing` 仅作为受控离线备用，未取得等价资产所有权授权时不得使用。

### Focused Automation

```text
Wacom.Editor.FormalFloor1Content
Wacom.Data.FormalFloor1Content
Wacom.Data.Card.Validation
Wacom.Data.EnemyPart
Wacom.Data.EnemyValidation
Wacom.Data.EnemyBehavior
Wacom.Data.Encounter
Wacom.Data.RunEvent.Validation
Wacom.Data.Shop.Validation
Wacom.Data.Pickup
Wacom.Battle.RuleContentMatrix
Wacom.Battle.KnockdownReward
Wacom.Run.BattleRewardCardsAddedToBackpack
Wacom.Run.NotificationCoalescing
Wacom.Run.Event
Wacom.Run.Pickup
Wacom.Run.Shop
```

实际前缀以 live test discovery 为准，记录发现数量和 pass/fail；不以不存在的别名伪造覆盖。

### Asset evidence

- 46/46 AssetRegistry load、class、stable ID、结构、strict seed defaults。
- 0 forbidden dependency、0 failed load、0 unexpected package。
- 三组双跑第二次 0 save；全部新增资产 hash 不变。
- 四张只读 Shop 卡前后 SHA-256 相同。
- 三个 writer audit JSON 与实际 Git/LFS paths 一致。

### Explicit skips

- **Blueprint compile**: skipped；本轮没有 Blueprint。
- **PIE**: skipped；没有 Production Journey/Floor/map/Host，规则与静态内容由 Automation 覆盖。
- **Card art/readability/balance**: not claimed；卡牌没有专用美术，数值和背包膨胀需要正式可玩路径后的 PIE/平衡轮。

## Commit Strategy

1. `feat(editor): add formal floor1 content seeder`
   - Spec 014、指针、Editor service/commandlet/console bridge、transient tests，以及已编译但在资产存在前不执行的只读 real-asset spec。
   - 0 `.uasset/.umap`。
2. `feat(content): seed formal floor1 production assets`
   - 46 `.uasset`、长期 Docs、最终 ledger；执行首笔提交中已编译的 real-asset spec。

不 merge main、不 push、不删除 branch/worktree。

## Complexity Tracking

无 Constitution 违例。保留 commandlet 与 Editor console bridge 是同一服务的两个适配器：前者用于 CI/read-only inspect，后者使资产写入能够被 Unreal MCP writer lease 审计。它们不复制配置逻辑，也不进入 runtime。seed-only 限制是长期防覆盖合同，不是临时绕过。
