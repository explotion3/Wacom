---
description: "Run exploration core rewrite implementation tasks"
---

# Tasks: Run 探索规则核心重构

**Input**: Design documents from `/specs/004-run-exploration-core-rewrite/`

**Prerequisites**: [plan.md](./plan.md), [spec.md](./spec.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/](./contracts/), [quickstart.md](./quickstart.md)

**Tests**: 规则、DataAsset、反射迁移和跨模块 contract 均要求 focused automation；Spline/镜头/卡牌手感增加 PIE 验收但不能替代事务测试。

**Organization**: 每个用户故事均可在后续故事未完成时独立自动化验证；正式调试资产和旧路径删除只在所有新合同稳定后执行。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 可在不同文件、且不依赖未完成公共合同的情况下并行。
- **[Story]**: 对应 spec.md 中的 US1–US5。
- 每个任务包含明确文件路径；生产代码与 WacomTests 分开。

## Phase 1: Setup、影响基线与规则文档对齐

**Purpose**: 在修改反射类型前冻结可比较的源码、资产和自动化基线。

- [x] T001 检查 `git status`、当前文档 diff 和其它 agent 改动，记录本轮允许修改边界到 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T002 使用 CodeGraph 与 `rg` 复核 `ConsumeNode`、`RemainingNodeCount`、Run Tunnel classes、Battle/Shop/RunEvent/Pickup callers，并把新增影响写回 `specs/004-run-exploration-core-rewrite/plan.md`
- [x] T003 使用 AssetRegistry/二进制 token 审计旧 Run Tunnel 类型、RunEvent enum 与 `L_Exploration` 引用，将迁移清单写入 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T004 运行迁移前 `WacomEditor` 编译及 Run/RunTunnel/RunEvent/Shop/BattleResult/first-person focused tests，并把基线结果写入 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T005 定向修正已确认规则在 `Docs/WacomMap.md`、`Docs/WacomRun.md` 与 `Docs/Game_Design.md` 中的术语冲突，保留未决正式内容问题到 `Docs/Questions.md`

---

## Phase 2: Foundational Contracts

**Purpose**: 先稳定所有故事共享的静态身份、Snapshot/Result 和测试 fixture；此阶段不切换现有运行路径。

**CRITICAL**: 完成并编译本阶段后才能开始 US1–US5。

- [x] T006 [P] 在 `Source/WacomData/Public/Map/WacomMapTypes.h` 定义 MapNodeType、MapNodeHandle、MapEdgeHandle、OwnedCardRequirement、Node/Edge typed payload reflected contracts；EdgeId 仅在 Floor 内唯一，跨层身份必须携带 FloorId
- [x] T007 [P] 在 `Source/WacomData/Public/Map/WacomJourneyDefinition.h` 定义 JourneyId、支持角色、Floor 顺序、时段预算和 Decay curves
- [x] T008 [P] 在 `Source/WacomData/Public/Map/WacomFloorMapDefinition.h` 定义 FloorId、EntryNodeId、Nodes 和 directed Edges
- [x] T009 在 `Source/WacomData/Private/Map/WacomJourneyDefinition.cpp` 与 `Source/WacomData/Private/Map/WacomFloorMapDefinition.cpp` 实现只读查找和安全默认值，不加入运行时生命周期规则
- [x] T010 [P] 在 `Source/WacomRun/Public/Exploration/RunExplorationTypes.h` 定义 lifecycle、time/exploration snapshot、event 与 opaque token public contracts
- [x] T011 [P] 在 `Source/WacomRun/Public/Exploration/RunExplorationCommand.h` 和 `RunExplorationResolution.h` 定义 C++-only command/result/status contracts
- [x] T012 在 `Source/WacomRun/Public/RunState.h` 添加组合式 `FRunTimeState`、`FRunExplorationState` 和 exploration version，暂时保留旧字段供迁移期编译
- [x] T013 在 `Source/WacomRun/Public/RunSession.h` 声明 `FRunInitializationParams/Result` 与 `ResolveExplorationCommand`；现有 `Initialize(UCharacterDefinition*)` 仅暂存原实现供资产迁移且禁止新增调用，不得包装新 API，最终在 T083 删除
- [x] T014 [P] 在 `Source/WacomTests/Public/Fixtures/WacomRunExplorationFixture.h` 和 `Source/WacomTests/Private/Fixtures/WacomRunExplorationFixture.cpp` 创建 transient Journey/Floor 与 initialized session fixture
- [x] T015 [P] 在 `Source/WacomEditor/Public/Validation/WacomMapDefinitionValidation.h` 建立 Journey/Floor validation report contract
- [x] T016 编译 `WacomEditor` 并运行新 public contracts 的 header/fixture smoke tests，将结果写入 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T017 在 `Docs/WacomData.md`、`Docs/WacomDataAuthoring.md` 和 `Docs/Architecture.md` 记录静态图合同、反射边界与不新增 UE Module 的决定

**Checkpoint**: 新数据/结果合同可编译，旧 Run 仍可运行；所有后续故事可以针对稳定身份开发。

---

## Phase 3: User Story 1 - 在非连续有向地图中可靠探索 (Priority: P1) MVP

**Goal**: 规则层完成初始化、节点生命周期、两阶段通道和同层 MapTravel；App 新 Path Traversal 可在 transient scene 中独立验证。

**Independent Test**: `Wacom.Run.Map` + `Wacom.UI.RunPathTraversal` 使用 transient Floor/actors 验证 begin/complete/cancel/dead-end/travel，不依赖节点内容结算。

### Tests and Validation for User Story 1

- [x] T018 [P] [US1] 在 `Source/WacomTests/Private/Run/RunInitializationResultSpec.cpp` 覆盖 working-state 成功、失败保留旧状态、版本 1 和初始 reveal
- [x] T019 [P] [US1] 在 `Source/WacomTests/Private/Run/RunMapLifecycleSpec.cpp` 覆盖 Hidden→Revealed→Visited→Resolved、非法逆转、directed adjacency，以及 Floor Entrance/Boss 远景轮廓不推进 lifecycle
- [x] T020 [P] [US1] 在 `Source/WacomTests/Private/Run/RunMapTraversalSpec.cpp` 覆盖 begin/complete/cancel、重复/过期 ticket、active transaction 与 MapTravel 拒绝
- [x] T021 [P] [US1] 在 `Source/WacomTests/Private/UI/RunPathTraversalSpec.cpp` 和私有 test access wrapper 覆盖 W/S、start/end one-shot、状态枚举和 Suspend/Resume
- [x] T022 [P] [US1] 在 `Source/WacomTests/Private/UI/RunExplorationPresentationCoordinatorSpec.cpp` 覆盖 source/target/host 双阶段 preflight、result version 幂等、start failure cancel、终点绑定失效先 Cancel、commit failure source recovery，以及成功 commit 后禁止 source fallback

### Implementation for User Story 1

- [x] T023 [US1] 在 `Source/WacomRun/Private/Exploration/RunMapModule.h` 和 `.cpp` 实现图查找、lifecycle/reveal、directed reachability 和 free same-floor travel
- [x] T024 [US1] 在 `Source/WacomRun/Private/Exploration/RunExplorationCommandResolver.h` 和 `.cpp` 实现 traversal ticket 与 exploration command working-state pipeline
- [x] T025 [US1] 在 `Source/WacomRun/Private/RunSession.cpp` 将新初始化改为完整 working state，并在每次成功探索事务只广播一次
- [x] T026 [P] [US1] 在 `Source/WacomApp/Public/Actors/WacomRunPathSegmentActor.h` 和 `Private/Actors/WacomRunPathSegmentActor.cpp` 实现 EdgeId + PathSpline actor，无 BeginPlay 自动激活
- [x] T027 [P] [US1] 在 `Source/WacomApp/Public/Actors/WacomRunPathBranchTargetActor.h` 和 `.cpp` 实现只上报 EdgeId 的点击目标
- [x] T028 [P] [US1] 在 `Source/WacomApp/Public/Actors/WacomRunMapNodeAnchorActor.h` 和 `.cpp` 实现 NodeId 场景落点
- [x] T029 [US1] 在 `Source/WacomApp/Public/Components/WacomRunPathTraversalComponent.h` 和 `Private/Components/WacomRunPathTraversalComponent.cpp` 实现 Inactive/Anchored/Traversing/Suspended、Spline movement、边界 latch、View transform 和 CameraShake
- [x] T030 [P] [US1] 在 `Source/WacomApp/Private/GameFramework/WacomRunSceneBindingRegistry.h` 和 `.cpp` 实现当前 Floor scoped Edge/Node/Host preflight registry，并缓存 target transform 供成功提交后的安全定位
- [x] T031 [US1] 在 `Source/WacomApp/Private/GameFramework/WacomRunExplorationPresentationCoordinator.h` 和 `.cpp` 实现 branch intent、ticket lifecycle、version 幂等和失败恢复
- [x] T032 [US1] 在 `Source/WacomApp/Public/GameFramework/WacomPlayerCharacter.h`、`Private/GameFramework/WacomPlayerCharacter.cpp` 和 `WacomPlayerController.cpp` 接入新 Traversal/Coordinator，同时保留旧组件仅供后续资产迁移
- [x] T033 [US1] 将 `Source/WacomApp/Private/Components/WacomFirstPersonCardAnchorComponent.cpp` 与 `Source/WacomApp/Private/Camera/WacomFirstPersonViewStageReturnFlow.cpp` 的 View Source 切换到新 Traversal contract
- [x] T034 [US1] 编译 `WacomEditor`，运行 `Wacom.Run.Map`、`Wacom.UI.RunPathTraversal`、ViewStage 与 first-person focused tests，并更新 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T035 [US1] 更新 `Docs/WacomMap.md` 与 `Docs/WacomApp.md` 的 traversal ticket、scene registry 和局部后退合同

**Checkpoint**: 纯规则和 transient App scene 可完成有向图移动；正式 `L_Exploration` 尚未迁移，旧反射类尚未删除。

---

## Phase 4: User Story 2 - 通过内容行动推进时段与压力 (Priority: P1)

**Goal**: Action Point、phase gate 和 Floor Exposure 完全替代旧 NodeCount/ConsumeNode 规则。

**Independent Test**: `Wacom.Run.Time` 在 transient Journey 中验证 2/6/2/2/1、Morning Planning、Night gate、Camp special advance 和 FloorDay 1–8 Decay。

### Tests and Validation for User Story 2

- [x] T036 [P] [US2] 在 `Source/WacomTests/Private/Run/RunActionPointSpec.cpp` 覆盖原子消费、不足无副作用、恰好耗尽和 phase budget reset
- [x] T037 [P] [US2] 在 `Source/WacomTests/Private/Run/RunTimePhaseGateSpec.cpp` 覆盖 Morning Planning 自动占用、Dusk 可选 Picnic、Night choice 和 Sunrise 路径
- [x] T038 [P] [US2] 在 `Source/WacomTests/Private/Run/RunFloorExposureSpec.cpp` 覆盖初始日不加 Decay、Base/Overstay curves、Camp skip Sunrise 和 Floor 中途进入

### Implementation for User Story 2

- [x] T039 [US2] 在 `Source/WacomRun/Private/Exploration/RunTimeModule.h` 和 `.cpp` 实现 private `TrySpendActionPoints`、phase advancement、Morning/Night gates
- [x] T040 [P] [US2] 在 `Source/WacomRun/Private/Exploration/RunFloorExposureModule.h` 和 `.cpp` 实现一次性 Morning daily decay 与 FloorDay 计算
- [x] T041 [US2] 在 `Source/WacomRun/Private/Exploration/RunExplorationCommandResolver.cpp` 接入 ChooseNightExploration 和 time/exposure events
- [x] T042 [US2] 在 `Source/WacomRun/Public/RunSession.h`、`Private/RunSession.cpp`、`Source/WacomApp/Public/UI/ViewModels/WacomRunViewModel.h`、`Private/UI/ViewModels/WacomRunViewModelProvider.cpp` 和 `Private/UI/Foundation/WacomExplorationHUD.cpp` 将只读展示迁为 RemainingActionPoints
- [x] T043 [US2] 迁移 `Source/WacomTests/Private/Run/RunStateSpec.cpp`、`SnapshotRevisionSpec.cpp` 与相关 UI ViewModel tests 的旧 NodeCount 断言到新语义，并明确 exploration result version 与现有 Backpack/Shop/Economy revision 的职责边界
- [x] T044 [US2] 删除 `Source/WacomRun/Private/Time/RunTimeRules.h/.cpp` 的旧 ConsumeNode/Advance public path，并从 `FRunState` 删除 Remaining/InitialNodeCount 字段
- [x] T045 [US2] 编译 `WacomEditor`，运行 `Wacom.Run.Time`、Pressure、SnapshotRevision 和 ExplorationHUD tests
- [x] T046 [US2] 更新 `Docs/WacomRun.md`、`Docs/Game_Design.md`、`CONTEXT.md`，全仓库源码中旧 NodeCount/ConsumeNode 除迁移待办外零引用

**Checkpoint**: 时间和压力核心独立可用，GameMode/内容系统仍待 US3 迁移到节点活动事务。

---

## Phase 5: User Story 3 - 原子结算战斗、事件、商店与宝藏 (Priority: P1)

**Goal**: 四类节点内容通过单一 active activity 所有权结算，不再由 GameMode、Screen 或 EndVisit 手工扣点。

**Independent Test**: `Wacom.Run.NodeActivity` 与各现有领域 focused tests 验证成本矩阵、预留、rollback、撤离和节点完成。

### Tests and Validation for User Story 3

- [x] T047 [P] [US3] 在 `Source/WacomTests/Private/Run/RunNodeActivityTransactionSpec.cpp` 覆盖互斥 token、预留、cancel、重复/过期 completion 和组合广播
- [x] T048 [P] [US3] 在 `Source/WacomTests/Private/Run/RunEncounterNodeSettlementSpec.cpp` 覆盖 Victory 1、Withdraw 0、Defeat、progress/reward/pressure/lifecycle
- [x] T049 [P] [US3] 在 `Source/WacomTests/Private/Run/RunEventActionPointPolicySpec.cpp` 覆盖 Automatic/Free/Fixed、MinActionPoints、正成本 terminal 和 rollback
- [x] T050 [P] [US3] 在 `Source/WacomTests/Private/Run/RunShopActionPointSpec.cpp` 覆盖 browse0、first purchase1、same visit0、phase advance closes visit
- [x] T051 [P] [US3] 在 `Source/WacomTests/Private/Run/RunTreasureNodeSettlementSpec.cpp` 覆盖 pickup/card-interaction success1、failure0、duplicate0

### Implementation for User Story 3

- [x] T052 [US3] 在 `Source/WacomRun/Private/Exploration/RunNodeActivityModule.h` 和 `.cpp` 建立 active token、AP reservation、typed begin/complete/cancel 与 working-state commit
- [x] T053 [US3] 将 `Source/WacomRun/Private/Battle/RunBattleSettlementResolver.h/.cpp` 改为 node activity settlement，使用 MapNodeHandle 保存 withdraw progress 并返回显式结果
- [x] T054 [US3] 在 `Source/WacomApp/Public/GameFramework/WacomGameMode.h` 和 `Private/GameFramework/WacomGameMode.cpp` 使用 Encounter start/result token，删除 Trigger completion 与手工 ConsumeNode
- [x] T055 [P] [US3] 在 `Source/WacomData/Public/Events/RunEventDefinition.h` 将 MinNodeCount 改为 MinActionPoints，新增 ActionPointPolicy/FixedCost 并删除 ConsumeNode effect
- [x] T056 [US3] 在 `Source/WacomRun/Private/Events/RunEventExecutor.h/.cpp` 将 option effects、card payment、AP cost、event state 和 map lifecycle 合并到同一事务
- [x] T057 [P] [US3] 在 `Source/WacomEditor/Private/Validation/RunEventDefinitionValidation.cpp` 更新 action policy、terminal cost 与旧 effect validation
- [x] T058 [US3] 在 `Source/WacomRun/Private/Shops/RunShopTransaction.h/.cpp` 和 `Private/RunSession.cpp` 将 AP 消费移动到本 visit 第一次成功 purchase，EndVisit 不再扣点
- [x] T059 [US3] 在 `Source/WacomRun/Private/RunSession.cpp` 与 Run Pickup/WorldCardInteraction 提交入口原子结算 Treasure reward、AP 和 node lifecycle
- [x] T060 [US3] 在 App 的 RunEvent/Shop/WorldInteraction flows 应用 explicit activity result，并在 phase advance 后安全关闭 active screen
- [x] T061 [US3] 分批迁移 `Source/WacomTests/Private/Run/RunEventSpec.cpp`、`RunEventValidationSpec.cpp`、`NotificationCoalescingSpec.cpp`、Shop/Backpack tests 和 Battle settlement tests，不新增只返回 status 的兼容 helper
- [x] T062 [US3] 编译 `WacomEditor`，运行 `Wacom.Run.NodeActivity`、RunEvent、Shop、WorldInteraction、Battle Result 和受影响 UI tests
- [x] T063 [US3] 更新 `Docs/WacomRun.md`、`Docs/WacomData.md`、`Docs/WacomDataAuthoring.md` 与 `Docs/WacomWorldInteraction.md` 的内容成本和 result flow

**Checkpoint**: Battle/RunEvent/Shop/Treasure 的玩家可观察结果和成本矩阵独立完成，旧 ConsumeNode enum/调用在源码中为零。

---

## Phase 6: User Story 4 - 露营与不可逆楼层推进 (Priority: P2)

**Goal**: 完成无正式 UI 的 Camp/Floor Transition 规则状态机和未来 typed activity seam。

**Independent Test**: `Wacom.Run.Camp` 与 `Wacom.Run.FloorTransition` 通过 transient multi-floor Journey 验证所有规则，不要求 `L_Exploration` 增加第二层。

### Tests and Validation for User Story 4

- [x] T064 [P] [US4] 在 `Source/WacomTests/Private/Run/RunCampSpec.cpp` 覆盖 Night/idle/AP gate、directed BFS、NodeId tie-break、fake handler、cancel 和 Night→Morning
- [x] T065 [P] [US4] 在 `Source/WacomTests/Private/Run/RunFloorTransitionSpec.cpp` 覆盖所有持有区要求、无消耗、首次 unlock、过期 confirmation、不可返回和时间/压力保留

### Implementation for User Story 4

- [x] T066 [P] [US4] 在 `Source/WacomRun/Private/Exploration/RunOwnedCardRequirementEvaluator.h` 和 `.cpp` 实现所有持有区的 typed requirement evaluation
- [x] T067 [US4] 在 `Source/WacomRun/Private/Exploration/RunCampModule.h` 和 `.cpp` 实现 directed BFS、reservation、typed handler seam 和 special phase advance
- [x] T068 [US4] 在 `Source/WacomRun/Private/Exploration/RunFloorTransitionModule.h` 和 `.cpp` 实现 preview/confirmation token、unlock 和 new Floor initialization
- [x] T069 [US4] 在 `Source/WacomRun/Private/Exploration/RunExplorationSnapshotBuilder.h` 和 `.cpp` 输出 Camp eligibility、travelability、Floor history 和 transition preview facts
- [x] T070 [US4] 在 `Source/WacomRun/Private/Exploration/RunExplorationCommandResolver.cpp` 接入 Begin/CancelCamp 与 Request/Confirm/CancelFloorTransition，保证互斥和版本幂等
- [x] T071 [US4] 编译 `WacomEditor`，运行 `Wacom.Run.Camp`、`Wacom.Run.FloorTransition`、Pressure 和 card ownership tests
- [x] T072 [US4] 更新 `Docs/WacomMap.md`、`Docs/WacomRun.md`、`Docs/TODO.md` 与 `Docs/Questions.md`，保留 Camp 具体活动/正式 UI/旅程规模未决项

**Checkpoint**: Camp 和跨层规则可由未来 UI 直接消费，不存在临时恢复数值、调试自动选择或跨层回退。

---

## Phase 7: User Story 5 - 设计者安全制作与验证地图 (Priority: P2)

**Goal**: 提供可重复制作/验证工具，迁移现有探索场景和资产，最终删除所有旧反射路径。

**Independent Test**: Builder 连续运行两次后资产身份稳定；Data/Scene validation 与 Blueprint compile 通过；PIE 完成单层最小流程。

### Tests and Validation for User Story 5

- [x] T073 [P] [US5] 在 `Source/WacomTests/Private/Data/MapDefinitionValidationSpec.cpp` 覆盖全部 Journey/Floor error/warning contracts
- [x] T074 [P] [US5] 在 `Source/WacomTests/Private/Editor/RunExplorationDebugAssetBuilderSpec.cpp` 覆盖 builder 幂等、稳定 ID 和内容引用迁移
- [x] T075 [P] [US5] 在 `Source/WacomTests/Private/UI/RunSceneBindingValidationSpec.cpp` 覆盖 anchor/path/host 缺失、重复和错类型

### Implementation for User Story 5

- [x] T076 [US5] 在 `Source/WacomEditor/Private/Validation/WacomMapDefinitionValidation.cpp` 与 `WacomMapDefinitionValidator.cpp` 实现 graph/content/requirement validation
- [x] T077 [US5] 在 `Source/WacomEditor/Private/Validation/WacomRunSceneBindingValidation.cpp` 实现 loaded-world NodeAnchor/EdgePath/content host validation
- [x] T078 [US5] 在 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.h/.cpp` 定义单层 Debug Journey/Floor fixture 和可重复资产迁移
- [x] T079 [US5] 在 `Source/WacomEditor/Private/Commandlets/WacomBuildRunExplorationAssetsCommandlet.h/.cpp` 提供构建、编译、保存和 validation 命令入口
- [x] T080 [US5] 运行 builder 创建/更新 `Content/Wacom/Data/Map/DA_Journey_Debug.uasset`、`DA_Floor_Debug_01.uasset` 和新 Run Path Blueprint assets
- [x] T081 [US5] 迁移 `Content/Wacom/Core/GameModes/GM_Wacom.uasset`、`Content/Wacom/Core/Player/BP_WacomPlayerCharacter.uasset` 与 `Content/Wacom/Maps/L_Exploration.umap` 的 Journey、NodeId、EdgeId、Anchor 和 content host 绑定
- [x] T082 [US5] 编译并重存所有受影响 Blueprint/DataAsset/Map，运行 Data/Scene validation 与 `Wacom.UI.RunPathTraversal` focused tests
- [x] T083 [US5] 在 `Source/WacomApp/Private/GameFramework/WacomPlayerController.cpp`、`Source/WacomRun/Public/RunSession.h`、`Private/RunSession.cpp`、`Source/WacomTests/Public/Fixtures/WacomRunExplorationFixture.h` 和 `Source/WacomTests/Private/{Run,UI,Battle}/` 迁移全部 Run 初始化调用与 `ResetRunState` 到 params/result 合同，测试 fixture 必须携带 InitializationResult；删除旧 `Initialize(UCharacterDefinition*)` UFUNCTION，并在 `SaveGameRoundtripSpec.cpp` 锁定 schema 3 与 exploration state 不序列化
- [x] T084 [US5] 迁移 `Source/WacomEditor/Private/ContentBuilders/SettingsRuntimeAssetBuilder.cpp`、`Source/WacomTests/Private/Settings/SettingsScreenSpec.cpp`、受影响 first-person/ViewStage tests 和所有旧 Movement/Segment/Branch consumers 后，删除 `Source/WacomApp/Public/Components/WacomRunTunnelMovementComponent.h`、对应 cpp、旧 Segment/Branch Actor 源码和旧 Run Tunnel BP assets；保留独立纸片美术 `WacomRunTunnelPaperLayerActor`
- [x] T085 [US5] 删除 `Config/DefaultEngine.ini` 的 Prototype/Movement Class/Property/Function Redirect，并清理旧 Player/GameMode/Anchor/Settings builder 引用
- [x] T086 [US5] 使用 `rg`、AssetRegistry 和 Blueprint 全量编译确认旧 Initialize、NodeCount/ConsumeNode、旧 Run Tunnel 类型/资产/Redirect 为零引用
- [x] T087 [US5] 更新 `Docs/WacomApp.md`、`Docs/WacomDataAuthoring.md`、`Docs/Architecture.md` 与 `Docs/TechDebt.md` 的正式制作/迁移事实

**Checkpoint**: Debug Journey 可 PIE，新 Path classes 是唯一运行路径，旧类和兼容层完全删除。

---

## Phase 8: Polish、全量验证与长期文档回写

**Purpose**: 收口跨故事回归、文档真相和人工验收。

- [x] T088 [P] 拆分或迁走本轮触及的新增测试，确保不继续扩大 `RunEventSpec.cpp`、`WorldInteractionAndShopSpec.cpp`、`BackpackSpec.cpp` 和旧 `RunTunnelMovementSpec.cpp`
- [x] T089 [P] 检查所有新 designer-facing UPROPERTY/UFUNCTION tooltip 使用中文并说明单位、作用、推荐区间和真实硬限制
- [x] T090 更新 `CONTEXT.md`、`Docs/WacomMap.md`、`Docs/WacomRun.md`、`Docs/WacomApp.md`、`Docs/WacomData.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomWorldInteraction.md`、`Docs/Architecture.md`、`Docs/Game_Design.md`、`Docs/TODO.md`、`Docs/TechDebt.md` 与 `Docs/Questions.md`
- [x] T091 运行 `WacomEditor` Win64 Development 完整编译并记录结果到 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T092 运行 Map/Time/NodeActivity/Camp/FloorTransition/RunPathTraversal 及所有受影响 Run/UI/Battle focused automation，将结果记录到 `specs/004-run-exploration-core-rewrite/quickstart.md`
- [x] T093 运行完整 `Automation RunTests Wacom` 并与 T004 基线比较，不得新增失败
- [x] T094 执行 Blueprint 全量编译、Data Validation、AssetRegistry 零引用审计和 `git diff --check`
- [ ] T095 按 `specs/004-run-exploration-core-rewrite/quickstart.md` 在 `L_Exploration` 完成 PIE 验收并记录剩余表现风险
- [x] T096 审核 `specs/004-run-exploration-core-rewrite/spec.md`、`plan.md`、`data-model.md`、`contracts/` 与最终 Docs 一致，确认长期事实不只停留在 specs

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 Setup**: 无依赖。
- **Phase 2 Foundational**: 依赖 Phase 1，阻塞所有故事。
- **US1 Map/Traversal**: 依赖 Foundational，是 App 和节点身份 MVP。
- **US2 Time/Exposure**: 依赖 Foundational；可在 US1 App 工作后半段并行开发私有规则文件，但合并前必须通过公共 `FRunState` 冲突审查。
- **US3 Node Activities**: 依赖 US1 当前节点/lifecycle 与 US2 Action Point。
- **US4 Camp/Floor**: 依赖 US1 map commands 与 US2 time/exposure；不依赖 US3 内容 UI。
- **US5 Authoring/Migration**: 依赖 US1–US4 的最终 contracts，尤其新反射类和 DataAsset 字段。
- **Final Validation**: 依赖全部选定故事和旧路径删除。

### Critical migration order

```text
Data contracts
  -> Run core
  -> New App classes coexist with old assets
  -> Node activity migration
  -> Debug asset builder/re-save
  -> Old reflected class deletion
  -> Blueprint/full regression
```

不得把旧类删除和资产重存调换顺序。

### Parallel Opportunities

- T006–T008、T010–T011、T014–T015 可在不编辑同一公共头文件时并行。
- 每个故事的 focused test files 可先于对应 private implementation 并行编写。
- US1 的 Path actors、Scene Registry 与 RunMapModule 文件边界独立，但 Coordinator 集成需等 contracts 稳定。
- US3 的 RunEvent validator、Shop transaction 与 Battle settlement 可分开实现，最终由 NodeActivityModule 汇合。
- US4 的 requirement evaluator 与 Camp module 可并行，Floor transition 需等待 evaluator。
- US5 的 Data validator tests、scene binding tests 和 builder tests 文件独立。

## Implementation Strategy

### MVP First

1. 完成 Phase 1–2。
2. 完成 US1 规则 + transient App traversal，不修改正式地图资产。
3. 停止并验证 US1，确认 logical node 与 scene pose 不分叉。
4. 完成 US2/US3，使现有内容成本全部进入新核心。
5. 完成 US4 contract 后才冻结 DataAsset 字段。
6. 最后执行 US5 一次性资产迁移和旧类删除。

### Wacom Review Checklist

- [ ] Docs 与 `AGENTS.md` 已遵守并在实现后回写。
- [ ] WacomData/WacomRun/WacomApp/WacomEditor 边界无反向依赖。
- [ ] Public API 只包含稳定 Data/Command/Result/identity；规则算法保持 Private。
- [ ] UI/Actor 不直接修改 RunState、Action Point 或 lifecycle。
- [ ] DataAsset 与场景绑定都有 Validator 和幂等 builder。
- [ ] 没有旧 wrapper、CoreRedirect、Level Blueprint 或 Tick 轮询规则残留。
- [ ] 每个切片编译，规则/资产/生命周期有 focused automation。
- [ ] PIE 只验收表现，不代替事务、token 和 rollback tests。

## Notes

- 本任务列表不使用 `$speckit-implement` 一次性执行；主会话应逐阶段读取 live 文件、检查 dirty workspace、编译并验证。
- 用户确认的 Withdraw 口径覆盖旧文档：0 Action Point、保留进度/经验/卡牌、仅沿用现有 Fatigue/Wound 规则。
- 正式 Map/Camp UI、Camp 具体收益、正式 Floor 数量与地图 SaveGame 必须另开后续切片，不得在本轮临时补齐。

## Refinement Tasks: route choice and Anchored look (2026-07-15)

- [x] R001 在 App-private Coordinator 派生并发布 0/1/多合法出口路线状态。
- [x] R002 在 PathTraversal 增加 Anchored W/A/D one-shot intent，并保持 Anchored cursor look Tick。
- [x] R003 新增 App-private BranchSelectionController，统一鼠标、键盘和手柄选择语义。
- [x] R004 修正初始 Anchored cursor look 的无点击坐标回退、RunPath 基础 Yaw 所有权和 NodeAnchor 到 PathSpline 的起步视角连续性，并增加稳定地图门控诊断。
- [x] R004 将 BranchTarget 收敛为 Hidden/Available/Focused 表现 Actor，并增加可重建发光材质真源。
- [x] R005 更新 Debug builder 与 scene validation，使单出口无 BranchTarget、多出口每 Edge 恰好一个。
- [ ] R006 完成 focused automation、资产重建、Blueprint 编译与 PIE 验收。（自动化、资产重建与 Blueprint 编译已完成；等待人工 PIE 表现验收。）
