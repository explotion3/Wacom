# Implementation Plan: 击倒分支奖励合同基线

**Branch**: `codex/knockdown-branch-reward-baseline` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/012-knockdown-branch-reward-baseline/spec.md`

## Summary

把单一 `KnockdownRewardCard` 扩展为明确的 Aid/Destroy 分支配置，同时保留现有二进制资产的 legacy fallback。`WacomData` 提供唯一查询，`WacomBattle` 在既有击倒选择事务中消费并生成只读奖励摘要，`WacomApp` 只显示摘要，`WacomEditor` 提供 General/FormalProduction 两档校验。Run 结果包、获得卡记录、存档和模块依赖保持不变。

## Wacom Domain Context

**Primary Domain**: Battle rules / Data-card authoring / Battle UI

**Required Docs Read**:
- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`、`Docs/TechDebt.md`
- [x] Spec 011 的 Production manifest/readiness 合同及 live headers/resolvers/tests

**Docs To Update**:
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`、`Docs/TechDebt.md`

**Owning Module(s)**: `WacomData`、`WacomBattle`、`WacomApp`、`WacomEditor`、`WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomData/WacomBattle/WacomApp for editor validation/builders
WacomTests  -> runtime/app/editor validation as test harness
```

不修改任何 Build.cs；`EKnockdownChoice` 已位于 `WacomCore`，因此 `WacomData` 可安全引用。

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: `UPrimaryDataAsset`、UPROPERTY deprecation metadata、UMG/CommonUI fallback、Automation Tests、AssetRegistry

**Storage/State**: 静态 `UEnemyPartDefinition` 字段与瞬时 `FKnockdownChoiceOptionView`；不新增 BattleState/RunState/SaveGame 字段

**Runtime Contracts**: DataAsset field/query、Battle ViewData、现有 `FBattleGainedCard.SourceChoice`、Editor validation profile

**Testing**: `Wacom.Data.EnemyPart`、`Wacom.Data.Enemy.TrainingWarrior`、`Wacom.Battle.Knockdown`、`Wacom.Battle.KnockdownReward`、Run reward/notification、Knockdown Dialog/Presentation；每个 C++ checkpoint 默认 Unity 编译

**Target Platform**: Windows editor

**Performance Goals**: 查询为常数时间字段选择；面板只在 `SetContext`/construct 刷新，不引入 Tick、加载循环或额外异步流程

**Constraints**: passive UI；缺奖励不禁用；旧资产不重存；不运行 builder；不得改变 ResultPacket/Run/Save/Tag/Build 合同

**Scale/Scope**: 3 个生产模块、1 个 Editor validator/builder 源码切片、2 个新小型测试文件、现有定向测试与 10 个长期文档

**Blueprint Exposure Strategy**: 两个奖励字段需要反射以供 DataAsset 制作；统一查询保持非反射 C++，避免 Blueprint 绕过分支/legacy 规则；ViewData 继续 BlueprintReadOnly；Dialog 仅新增可选文本锚点

**Data/GameplayTag Impact**: 新增 `AidRewardCard`、`DestroyRewardCard`，旧字段标记 deprecated；不新增 GameplayTag，不修改或保存任何 DataAsset

**Save/Load Impact**: 无。奖励卡仍经现有 `FBattleResultPacket.GainedCards` 进入 Run，SaveGame schema 不变

**UI/App Lifecycle Impact**: Dialog push、Modal focus、Back 拦截和命令提交流程不变；`SetContext` 与 `NativeConstruct` 统一刷新文本和可用性；无订阅需要解绑

## Constitution Check

*GATE: PASS before research; PASS after design.*

- **Docs and AGENTS Are the Rule Truth**: 已读取 live Docs/源码并列出所有长期文档同步点。
- **Wacom Module Boundaries Are Mandatory**: 静态合同、规则、表现、Editor 校验和测试分别归属正确模块；依赖方向与 Build.cs 不变。
- **Domain Rules Before Presentation**: UI 只消费 `FKnockdownChoiceOptionView`，不持有卡牌定义、不判断奖励合法性。
- **Data, GameplayTags, and Authoring Stay Explicit**: 字段、deprecated 兼容、两档 validator、builder 源码迁移及零资产写入均显式记录。
- **Reusable Systems Over One-Off Work**: 单一 Data 查询同时服务 resolver 与 ViewData，不建立敌人/关卡/UI 特例。
- **Validation Is Part of the Slice**: 每个 checkpoint 有编译和定向测试，最终包含 AssetRegistry、Blueprint、哈希和 LFS 审计。

## Phase 0: Research

结论记录于 [research.md](./research.md)：

1. live `WacomData` 字段、Battle resolver/availability、Run settlement、Dialog 和 validator 当前流向。
2. `EKnockdownChoice` 的底层模块归属与非反射查询可行性。
3. General/FormalProduction 校验边界与 legacy 资产不重存策略。
4. ViewData 的最小字段和显示名 fallback。
5. Spec 011 的 38 核心资产如何扩展为额外 8 张奖励卡，而不改原 manifest 写集合。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：字段、ViewData、验证 profile、迁移和状态转换。
- [contracts/data-authoring-contract.md](./contracts/data-authoring-contract.md)：查询和制作校验。
- [contracts/battle-resolution-contract.md](./contracts/battle-resolution-contract.md)：原子选择与 Run 保持项。
- [contracts/ui-preview-contract.md](./contracts/ui-preview-contract.md)：被动 UI 与生命周期。
- [contracts/production-migration-contract.md](./contracts/production-migration-contract.md)：Floor 1 八张卡与 legacy 删除门槛。
- [quickstart.md](./quickstart.md)：checkpoint、最终命令、哈希和跳过项记录。

## Project Structure

### Documentation (this feature)

```text
specs/012-knockdown-branch-reward-baseline/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── data-authoring-contract.md
│   ├── battle-resolution-contract.md
│   ├── ui-preview-contract.md
│   └── production-migration-contract.md
├── checklists/
└── tasks.md
```

### Source Code (repository root)

```text
Source/WacomData/Public/Enemies/EnemyPartDefinition.h
Source/WacomBattle/Public/Session/BattleSession.h
Source/WacomBattle/Private/Commands/KnockdownChoiceAvailability.cpp
Source/WacomBattle/Private/Commands/KnockdownChoiceResolver.cpp
Source/WacomApp/Public/UI/Battle/WacomKnockdownChoiceDialog.h
Source/WacomApp/Private/UI/Battle/WacomKnockdownChoiceDialog.cpp
Source/WacomEditor/Public/Validation/EnemyPartDefinitionValidation.h
Source/WacomEditor/Private/Validation/EnemyPartDefinitionValidation.cpp
Source/WacomEditor/Private/ContentBuilders/TrainingWarriorBuilder.cpp
Source/WacomEditor/Private/ContentBuilders/SnakeBuilder.cpp
Source/WacomTests/Private/Battle/KnockdownBranchRewardSpec.cpp
Source/WacomTests/Private/UI/KnockdownChoiceDialogSpec.cpp
Source/WacomTests/Private/UI/WacomUITestAccess.h
Source/WacomTests/Private/Data/TrainingWarriorContentSpec.cpp
Source/WacomTests/Private/Run/DataDefinitionValidationSpec.cpp
```

**Structure Decision**: 公共面只增加 DataAsset 查询和 Battle ViewData 字段；resolver/availability 继续 private；Dialog 继续现有 public Blueprintable 制作面；test probe 只存在 `WacomTests/Private`。

## Implementation Checkpoints

### Checkpoint 1 — Data/authoring contract

- 增加字段、查询、deprecation metadata 与 validation profile。
- 修改 TrainingWarrior/Snake builder 的未来写入，不运行 builder。
- 扩展 EnemyPart validation 和 TrainingWarrior legacy fallback 测试。
- 编译并运行 `Wacom.Data.EnemyPart`、`Wacom.Data.Enemy.TrainingWarrior`。

### Checkpoint 2 — Battle atomic settlement

- Resolver 使用统一查询；Availability 同一查询构造奖励 ViewData。
- 新建 `KnockdownBranchRewardSpec.cpp` 覆盖显式分支、fallback、空奖励和 Withdraw。
- 编译并运行新旧 Knockdown 及 Run reward/notification 回归。

### Checkpoint 3 — Passive UI preview

- Dialog 新增 Aid/Destroy 奖励文本锚点与原生 fallback。
- 新建独立 Dialog 测试；不扩张 `BattleWidgetSpec.cpp`。
- 编译并运行 Knockdown Dialog、presentation queue/plan 回归。

### Checkpoint 4 — Docs/migration/final validation

- 同步长期 Docs、Spec tasks/checklists/quickstart。
- 只读审计资产、Blueprint、failed-load 与哈希。
- 最终默认 Unity 编译和全部聚焦测试。

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development `
  -Project='D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'D:\UE_Project\5.7\WacomWorktrees\knockdown-branch-reward-baseline\Wacom\Wacom.uproject' `
  -ExecCmds='Automation RunTests Wacom.Data.EnemyPart; Automation RunTests Wacom.Data.Enemy.TrainingWarrior; Automation RunTests Wacom.Battle.Knockdown; Automation RunTests Wacom.Battle.KnockdownReward; Automation RunTests Wacom.Run.BattleRewardCardsAddedToBackpack; Automation RunTests Wacom.Run.NotificationCoalescing; Automation RunTests Wacom.UI.Battle.KnockdownChoice; Automation RunTests Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded; Automation RunTests Wacom.UI.Battle.PresentationPlan.Gained; Quit' `
  -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

**Manual/Editor Validation**: 不做正式 PIE；执行只读 AssetRegistry/failed-load、Blueprint compile，且前后检查 TrainingWarrior Part 与 BrokenCleave SHA-256。无二进制内容可展示不同分支卡，因此 PIE 不增加本轮证据。

## Complexity Tracking

无 Constitution 违例。legacy 字段是受控迁移债，不是新增临时分支；其删除触发条件在 Production migration contract 与 `Docs/TechDebt.md` 中固定。
