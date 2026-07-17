# Implementation Plan: Journey 成功结算与终局交接基线

**Branch**: `codex/run-level-authoring-baseline` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: 在基线 `70023f91` 上，以 `WacomData → WacomRun → WacomApp` 的单向合同实现可复用的 Journey 成功终局。

## Summary

`UWacomJourneyDefinition` 声明唯一成功终局；Editor 与 Run 初始化共同验证终局静态合同。`WacomRun` 在终局 Encounter 的有效非撤离 Victory 事务中原子提交奖励、节点、AP、`ERunOutcome::Succeeded`、完成摘要与末尾事件；SaveGame v5 保存 Outcome/独立摘要但拒绝把终态档恢复为活动 Run。`WacomApp` 在现有 Return-to-Run 镜头 staging 后展示被动原生总结页，并由 GameMode 在确认或 Back 后下一帧返回 `L_MainMenu`。

## Wacom Domain Context

**Primary Domain**: Data authoring / Run-exploration / Save-load / UI-App / Testing

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `specs/009-formal-floor23-journey-pacing-freeze/`
- [x] live Journey definition/validator、Run Session/settlement/snapshot、SaveGame、GameMode/CommonUI 源码与既有 tests

**Docs To Update**:

- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`

**Owning Module(s)**: `WacomData`, `WacomRun`, `WacomApp`, `WacomEditor`, `WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

不修改 Build.cs 或依赖；App 只读取 Run event/snapshot，Data 不依赖 Run，Widget 不写 RunState。

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: DataAsset、Editor Data Validation、Automation Tests、CommonUI/UMG、GameMode travel、USaveGame

**Storage/State**: `FRunState` 单玩家本地权威状态；SaveGame v5 持久 Outcome 与最近成功摘要；UI ViewData 为 transient projection

**Runtime Contracts**: `UWacomJourneyDefinition` fields、`ERunOutcome`、`FRunCompletionSummary`、`FRunExplorationSnapshot`、`ERunExplorationEventType::JourneySucceeded`、`FWacomJourneySummaryViewData`

**Testing**: `Wacom.Data.Map.Validation`, `Wacom.Run.JourneyCompletion`, 既有 Encounter/Result/notification, `Wacom.Run.Save`, `Wacom.UI.JourneySummary`, GameMode handoff/return staging/input-focus

**Target Platform**: Windows editor / packaged Windows single-player

**Performance Goals**: 事件驱动、无 Tick；成功事务保持单次版本递增和单次状态广播；总结页单实例/单次 travel

**Constraints**: 不修改二进制资产、GameplayTag、Build.cs、模块依赖、现有 Command/Resolution schema、AP/Camp/跨层规则；不运行 builder；Save 总开关保持关闭

**Scale/Scope**: 5 modules + tests；1 DataAsset contract、1 runtime outcome/summary contract、1 Save schema version、1 native fallback screen、约 7 长期 Docs

**Blueprint Exposure Strategy**: Journey 字段、Outcome/摘要/Snapshot/ViewData 使用反射以服务 DataAsset、SaveGame 与 Blueprint 表现；规则 helper、事务判定、GameMode flow 和 test access 保持 C++ private/protected

**Data/GameplayTag Impact**: Journey 新增两个字段与 validator；零 GameplayTag、零生成资产、零 Content 修改

**Save/Load Impact**: v5 显式迁移；v4 `bRunActive` 只用于迁移；Succeeded 必须带摘要；InProgress 禁止摘要；Succeeded/Failed 不可恢复为活动 Run

**UI/App Lifecycle Impact**: GameMode 在 return staging 双 barrier 完成后 push；Screen activate 获得 UI focus，deactivate/destruct 清委托；确认/Back 去重；PrimaryLayout teardown 后次帧 travel；push failure 同路 fallback

**Ownership/Lifetime/Network**: Journey/RunSession 由现有 GameMode/GameInstance 流程拥有；Summary Screen 由 PrimaryLayout/CommonUI layer 拥有，GameMode 持 transient 引用并在 teardown 前解绑；功能明确为单玩家，不新增 replication/RPC。

## Constitution Check

### Pre-research gate

- **Docs and AGENTS Are the Rule Truth — PASS**: 已读取全部相关长期文档与 live 源码，长期事实将在 checkpoint 5 回写。
- **Wacom Module Boundaries Are Mandatory — PASS**: Data/Run/App/Editor/Tests 职责明确且不改依赖。
- **Domain Rules Before Presentation — PASS**: Outcome/摘要/事件由 Run 事务生成；Screen 只展示 ViewData 并发 intent。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 只新增 Journey 字段/validator；零 tag/二进制写入。
- **Reusable Systems Over One-Off Work — PASS**: 使用 Journey handle、通用 Outcome/摘要和原生 fallback，不依赖 label、关卡蓝图或一次性 Actor。
- **Validation Is Part of the Slice — PASS**: 四个可编译 checkpoint 各自编译、定向测试并持续记入 quickstart。

## Phase 0: Research

结论见 [research.md](./research.md)：

1. 静态终局身份必须是 Floor-qualified handle，且 validator/runtime 使用同一语义。
2. 原始 `bRunActive` 无法区分成功/失败，Outcome 必须成为权威状态；兼容查询继续保留。
3. 成功判定属于 Encounter settlement transaction，不能由 GameMode 或 Snapshot 后处理推断。
4. 终局 Victory 优先于同一战斗导致的压力失败，原始 Outcome 为 Succeeded 时兼容失败查询不得覆盖。
5. Save runtime struct 与磁盘 struct 分离，迁移/验证在应用前完成，终态恢复必须零修改。
6. App 只消费事件并等待既有镜头 staging；成功路径不恢复 exploration-only presentation。
7. CommonUI push failure 与正常 continue 共用一个 idempotent main-menu handoff。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：Journey terminal、Outcome 状态机、摘要、事件、Save v5 和 ViewData。
- [contracts/journey-terminal-definition.md](./contracts/journey-terminal-definition.md)：静态终局与 validator/runtime 合同。
- [contracts/run-success-settlement.md](./contracts/run-success-settlement.md)：原子成功事务、优先级、事件顺序与终态拒绝。
- [contracts/savegame-v5-completion-summary.md](./contracts/savegame-v5-completion-summary.md)：磁盘结构、迁移、合法性与恢复边界。
- [contracts/app-summary-handoff.md](./contracts/app-summary-handoff.md)：staging、被动 UI、焦点/生命周期与 travel fallback。
- [quickstart.md](./quickstart.md)：checkpoint 命令、实测结果、哈希、跳过项和风险。

### Post-design Constitution Re-check

PASS。设计没有新增依赖、GameplayTag、资产或 UI 规则副本；SaveGame 有显式版本/迁移，UI 有数据源、刷新、焦点、解绑与 fallback，所有规则写入保持 Run-owned transaction。

## Project Structure

### Documentation (this feature)

```text
specs/010-journey-success-settlement-baseline/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/
│   ├── requirements.md
│   └── journey-success.md
├── contracts/
│   ├── journey-terminal-definition.md
│   ├── run-success-settlement.md
│   ├── savegame-v5-completion-summary.md
│   └── app-summary-handoff.md
└── tasks.md
```

### Source Code (expected)

```text
Source/WacomData/Public/Map/WacomJourneyDefinition.h
Source/WacomEditor/Private/Validation/WacomMapDefinitionValidation.cpp
Source/WacomRun/Public/RunOutcomeTypes.h
Source/WacomRun/Private/RunOutcomeTypes.cpp
Source/WacomRun/Public/RunState.h
Source/WacomRun/Public/Exploration/RunExplorationTypes.h
Source/WacomRun/Public/RunSession.h
Source/WacomRun/Private/RunSession.cpp
Source/WacomRun/Private/Exploration/*
Source/WacomRun/Public/Save/WacomSaveGame.h
Source/WacomRun/Private/Save/WacomSaveGame.cpp
Source/WacomRun/Private/Save/WacomRunSaveGameSerializer.cpp
Source/WacomApp/Public/WacomGameFlowTypes.h
Source/WacomApp/Public/UI/Menus/WacomJourneySummaryScreen.h
Source/WacomApp/Private/UI/Menus/WacomJourneySummaryScreen.cpp
Source/WacomApp/Public/WacomGameMode.h
Source/WacomApp/Private/WacomGameMode.cpp
Source/WacomTests/Private/Data|Run|UI/*
```

**Structure Decision**: 公共文件只承载跨模块反射合同；终局验证 helper、success transaction helper、Save conversion 和 App flow 保持 Private。新增测试拆成小型 spec，不扩张既有巨型文件。

## Validation Plan

每个 Unreal 命令都附加 `-NoDreamShaderEditorBridge`。

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.JourneyCompletion; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

**Checkpoint prefixes**:

1. `Wacom.Data.Map.Validation`
2. `Wacom.Run.JourneyCompletion` + affected Encounter/Result/notification
3. `Wacom.Run.Save`
4. `Wacom.UI.JourneySummary` + GameMode handoff/return staging/input-focus

**Manual/Editor Validation**: 最终只读 AssetRegistry/failed-load audit；不做真实 Floor 3 PIE，因为当前没有 Production DataAsset/场景。该 skip 是内容缺口，不是规则自动化缺口。

## Complexity Tracking

无 Constitution 违规。原生 fallback Screen 是正式容错层，不是临时 WBP 替代债务；失败总结流程明确留到下一功能，不在本轮伪实现。
