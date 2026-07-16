# Implementation Plan: Run 持久任务凭证

**Branch**: `codex/run-level-authoring-baseline` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: 将表现卡与不可丢失的 Run 通行资格解耦，并以可复用、可存档、可静态校验的 Credential 合同解除正式 Floor 1 蛇印软锁。

## Summary

在 `FRunState` 中增加只增不减的稳定 `FName` Credential 集合，由 `UWacomRunPickupDefinition` 声明授予，由 `URunSession` 在数据驱动 Pickup 的同一 working-state 事务中原子提交。FloorEntrance 新增 `RequiredCredentialIds`，与既有 `OwnedCardRequirements` 采用 AND；入口预览、请求和确认只读取权威 Run 状态。Credential 进入 SaveGame v4，v3 迁移为空集合，不从实体卡反推。编辑器校验必须证明入口要求的每个 Credential 在其前方存在支配入口的固定 Pickup 来源。

稳定蛇印身份冻结为 `Credential.Run.SerpentSigil`；`Card.Run.SerpentSigil` 只负责可见表现。当前轮不创建正式资产、不启用存档、不修改 GameplayTag、Build.cs、模块依赖或 Battle/UI 合同。

## Wacom Domain Context

**Primary Domain**: Run-exploration / Save-load / Data authoring / Editor validation / Tests

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `specs/007-formal-floor1-content-freeze/`
- [x] 当前 `RunState`、Pickup、FloorTransition、SaveGame、validator 与定向测试源码

**Docs To Update**:

- [ ] `Docs/WacomRun.md`
- [ ] `Docs/WacomMap.md`
- [ ] `Docs/WacomData.md`
- [ ] `Docs/WacomDataAuthoring.md`
- [ ] `Docs/TODO.md`
- [ ] `Docs/Questions.md`
- [ ] `Docs/Architecture.md` 仅同步 SaveGame v4 的长期 schema 事实；模块方向不变

**Owning Module(s)**: `WacomData`、`WacomRun`、`WacomApp`、`WacomEditor`、`WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomData / WacomRun for editor-only validation
WacomTests  -> runtime/app/editor modules as test harness
```

现有依赖已覆盖本轮调用，不修改任何 `.Build.cs`。

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: `UPrimaryDataAsset`、`USaveGame`、Automation Tests、Editor Data Validation

**Storage/State**: `FRunState::GrantedCredentialIds` 为运行时权威集合；`UWacomSaveGame::GrantedCredentialIds` 为 v4 确定排序磁盘数组

**Runtime Contracts**: Pickup Definition grant field、`URunSession` C++ 数据驱动 Pickup 入口、只读 `HasCredential`、FloorEntrance requirement field、SaveGame v4

**Testing**: `Wacom.Run.Credential`、`Wacom.Run.Pickup`、`Wacom.Run.FloorTransition`、`Wacom.Run.Save`、`Wacom.Data.RunPickup.Validation`、`Wacom.Data.Map.Validation` 与受影响 RewardPickup 定向 UI 测试

**Target Platform**: Windows editor / packaged Windows build

**Performance Goals**: Credential 查询为集合查找；序列化只在保存边界排序；validator 复用当前图支配分析，不引入 Tick 或运行时资产扫描

**Constraints**: 原子 working-state 提交、失败零修改/零广播、稳定 FName 非空唯一、无卡牌反推、无 Floor 1 特判、不开存档总开关、不保存资产

**Scale/Scope**: 2 个反射字段、1 个 RunState 集合、1 个私有 Credential 规则模块、1 个窄数据驱动 Pickup API、1 个只读查询、1 次 SaveGame 版本升级、现有 validator 扩展、小型定向测试

**Blueprint Exposure Strategy**: 静态资产字段使用 `UPROPERTY(EditAnywhere)`；`HasCredential` 保持 C++ 只读 API，数据驱动 Pickup 入口保持 C++ 调用，避免向 Blueprint 暴露任意 Credential 写入口

**Data/GameplayTag Impact**: `UWacomRunPickupDefinition::GrantedCredentialIds`、`FWacomMapFloorEntrancePayload::RequiredCredentialIds`；均为稳定 FName，不新增 GameplayTag、不写资产

**Save/Load Impact**: `CurrentSaveVersion` 3→4；v3→v4 初始化空集合；写入排序，读取拒绝 `NAME_None`/重复并保持原子性；SaveGame 总开关继续关闭

**UI/App Lifecycle Impact**: 无新 UI、焦点或输入合同；`AWacomRunRewardPickupActor` 只改为提交完整 Definition，现有 Card/Gold toast 保持不变

## Constitution Check

### Pre-research gate

- **Docs and AGENTS Are the Rule Truth — PASS**: 已读取长期 Docs、上一轮冻结和 live 源码；已决规则将同步回领域 Docs。
- **Wacom Module Boundaries Are Mandatory — PASS**: 静态字段在 Data，权威状态和事务在 Run，Actor 仅转发 Definition，校验在 Editor。
- **Domain Rules Before Presentation — PASS**: UI/Actor 不持有、推导或撤销 Credential。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 明确两个 FName 字段、v4 schema、validator 与零资产修改。
- **Reusable Systems Over One-Off Work — PASS**: 多 Credential 集合与数据化授予/要求，无蛇印或 Floor 1 条件分支。
- **Validation Is Part of the Slice — PASS**: 每个可编译 checkpoint 都有 WacomEditor 编译和定向 Automation。

## Phase 0: Research

研究结论见 [research.md](./research.md)：

1. Credential 必须是 Run 权威状态，不能是卡牌派生属性或 UI 标记。
2. 数据驱动 RewardPickup 需要一次完整 Definition 结算入口，不能在旧 Card Pickup 成功后补发 Credential。
3. Credential 只增不减，因此既有销毁、卖卡、支付和世界交互消耗路径无需耦合修改。
4. FloorEntrance 继续支持 OwnedCardRequirements；两组条件采用 AND，已解锁入口继续沿用现有永久解锁语义。
5. SaveGame v4 使用排序数组映射运行时集合；v3 没有合法 Production 蛇印历史，迁移为空最安全。
6. 保证来源必须使用图支配关系，而不是“某条分支上存在 Pickup”。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：静态 grant/requirement、运行时集合、v4 schema、事务状态变化和非法状态。
- [contracts/credential-state-contract.md](./contracts/credential-state-contract.md)：稳定身份、只增不减、只读查询和禁止反推。
- [contracts/pickup-grant-contract.md](./contracts/pickup-grant-contract.md)：数据驱动 Pickup 原子提交合同。
- [contracts/floor-entrance-credential-contract.md](./contracts/floor-entrance-credential-contract.md)：入口 AND 求值与静态保证来源。
- [contracts/savegame-v4-contract.md](./contracts/savegame-v4-contract.md)：确定性序列化、v3 迁移和失败原子性。
- [quickstart.md](./quickstart.md)：checkpoint 编译/测试、跳过项、哈希与最终交接记录。

### Post-design Constitution Re-check

PASS。设计没有新增任意 Blueprint 写入口、GameplayTag、模块依赖、硬编码蛇印分支或资产迁移；Credential 的唯一写路径是 Run 私有规则，所有外部输入均经 Definition/SaveGame 校验。

## Project Structure

### Documentation (this feature)

```text
specs/008-run-credential/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── credential-state-contract.md
│   ├── pickup-grant-contract.md
│   ├── floor-entrance-credential-contract.md
│   └── savegame-v4-contract.md
└── tasks.md
```

### Expected source changes

```text
Source/WacomData/
├── Public/Pickups/RunPickupDefinition.h
└── Public/Map/WacomMapTypes.h

Source/WacomRun/
├── Public/RunState.h
├── Public/RunSession.h
├── Public/WacomSaveGame.h
├── Private/RunSession.cpp
├── Private/WacomSaveGame.cpp
├── Private/Credential/RunCredentialModule.{h,cpp}
├── Private/Exploration/RunFloorTransitionModule.cpp
└── Private/Save/RunSaveGameSerializer.cpp

Source/WacomApp/Private/Actors/WacomRunRewardPickupActor.cpp

Source/WacomEditor/Private/Validation/
├── RunPickupDefinitionValidation.cpp
└── WacomMapDefinitionValidation.cpp

Source/WacomTests/Private/
├── Run/RunCredentialSpec.cpp
├── Run/RunCredentialSaveSpec.cpp
├── Run/RunFloorTransitionSpec.cpp
├── Run/SaveGameRoundtripSpec.cpp
├── Run/RunPickupDefinitionValidationSpec.cpp
├── Data/MapDefinitionValidationSpec.cpp
└── UI/RunRewardPickupCredentialSpec.cpp
```

**Structure Decision**: 不增加新模块。Credential 算法放 `WacomRun/Private/Credential`；公共面只保留查询与已存在 DataAsset 反射字段。若 live 实现证明某个列出的文件无需改动，可在 tasks/quickstart 记录，不为满足计划机械触碰。

## Validation Plan

所有 Unreal 命令统一加 `-NoDreamShaderEditorBridge`，不运行 builder。

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -NoDreamShaderEditorBridge
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests <PREFIX>; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

**Checkpoint prefixes**:

1. Data fields/validation: `Wacom.Data.RunPickup.Validation`
2. Credential state + atomic Pickup: `Wacom.Run.Credential`, `Wacom.Run.Pickup`
3. FloorEntrance + graph validator: `Wacom.Run.FloorTransition`, `Wacom.Data.Map.Validation`
4. SaveGame v4: `Wacom.Run.Save`
5. App integration/regression: `Wacom.UI.WorldInteraction.RunRewardPickupCredential` plus all prefixes above

**Manual/Editor Validation**: 不做 PIE；本轮没有可配置 `Credential.Run.SerpentSigil` 的 Production Pickup/Floor/Map 资产。运行现有 Debug DataAsset 加载/validator 回归，确认新增数组默认为空且 Blueprint/Asset load 无失败。PIE 风险与跳过理由写入 quickstart。

## Complexity Tracking

无 Constitution 违规或临时实现。
