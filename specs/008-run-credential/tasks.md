# Tasks: Run 持久任务凭证

**Input**: [spec.md](./spec.md), [plan.md](./plan.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/](./contracts/), [quickstart.md](./quickstart.md)

**Tests**: 规则、SaveGame、Data Validation 与跨模块 Actor 接入均需要小型定向 Automation。每个可编译 checkpoint 先编译 `WacomEditor`，再运行列出的最小测试前缀；所有 Unreal 命令加 `-NoDreamShaderEditorBridge`，不运行 builder。

## Phase 1: Setup and live-source audit

**Purpose**: 锁定工作区、规则决策和当前实现事实，避免对旧假设编码。

- [x] T001 检查 branch、HEAD、git status、Git LFS 与 Unreal/ShaderCompileWorker 进程，并记录到 `specs/008-run-credential/quickstart.md`
- [x] T002 读取 `AGENTS.md`、`Docs/AgentIntegrationWorkflow.md`、Run/Map/Data/App/Architecture 长期 Docs、Spec 007 与 live Pickup/FloorTransition/SaveGame/validator/tests 源码
- [x] T003 在 `specs/008-run-credential/spec.md` 与 `checklists/requirements.md` 冻结 `Credential.Run.SerpentSigil` 和 v3→v4 空集合迁移，清除所有 clarification

**Checkpoint**: 工作区边界清楚，稳定身份和迁移规则已经用户确认。

---

## Phase 2: Planning and shared contracts

**Purpose**: 先形成所有实现切片共用的数据、事务、入口与存档合同。

- [x] T004 生成并审阅 `plan.md`、`research.md`、`data-model.md`、`contracts/*.md` 与 `quickstart.md`，明确模块所有权、零资产边界和验证命令
- [x] T005 更新 `AGENTS.md` 托管 Spec Kit 指针为 `specs/008-run-credential/plan.md`，不修改其它协作规则
- [x] T006 运行 Spec Kit 跨工件一致性审阅，修复 spec/plan/tasks 的遗漏、重复、错误模块或过大测试任务

**Checkpoint CP0**: 设计工件一致、无模板占位、无未决规则，允许修改源码。

---

## Phase 3: Foundational static data contracts

**Purpose**: 先建立 Pickup grant 与 FloorEntrance requirement 的反射字段和局部数据合法性。

- [x] T007 在 `Source/WacomData/Public/Pickups/RunPickupDefinition.h` 增加可为空、非空项唯一的 `GrantedCredentialIds`，并在 `Source/WacomData/Private/Pickups/RunPickupDefinition.cpp` 扩展配置 warning
- [x] T008 在 `Source/WacomData/Public/Map/WacomMapTypes.h` 增加 `RequiredCredentialIds`，保持既有 `OwnedCardRequirements` 合同
- [x] T009 在 `Source/WacomTests/Private/Run/RunPickupDefinitionValidationSpec.cpp` 增加空/重复 Credential grant 与现有 Debug 资产空数组兼容测试
- [x] T010 在 `Source/WacomEditor/Private/Validation/RunPickupDefinitionValidation.cpp` 对新 warning 保持 Data Validation 错误桥接，并按 live 结构补充必要上下文
- [x] T011 编译 `WacomEditor`，运行 `Wacom.Data.RunPickup.Validation`，审计 failed-load/Debug DataAsset 加载，并把日志和结果写入 `quickstart.md`

**Checkpoint CP1**: 新字段可编译、旧资产默认空且不改变行为；尚未进入运行时写入。

---

## Phase 4: User Story 1 - 原子获得不可丢失 Credential (Priority: P1)

**Goal**: 数据驱动 Pickup 在一个 Run working-state 事务中授予主奖励、Credential、Pickup/节点/AP 状态，且卡牌移除不影响 Credential。

**Independent Test**: `Wacom.Run.Credential` 证明 grant 幂等、非法输入零提交、Card/Gold 主奖励原子提交，并覆盖直接销毁、删牌换金币、RunEvent 支付和世界交互消耗后 Credential 不变。

- [x] T012 在 `Source/WacomTests/Private/Run/RunCredentialSpec.cpp` 增加原子 grant、幂等、非法空/重复 ID rollback 与四类卡牌移除独立性测试
- [x] T013 在 `Source/WacomRun/Public/RunState.h` 增加 `GrantedCredentialIds`，在 `Source/WacomRun/Private/Credential/RunCredentialModule.{h,cpp}` 实现非空校验、幂等批量授予和 HasAll
- [x] T014 在 `Source/WacomRun/Public/RunSession.h` 暴露窄 `HasCredential` 与 C++ 数据驱动 Pickup 入口，并在 `Source/WacomRun/Private/RunSession.cpp` 收敛 Gold/Card/Definition 为同一 working-state 事务
- [x] T015 在 `Source/WacomApp/Private/Actors/WacomRunRewardPickupActor.cpp` 改为提交完整 Pickup Definition，并在小型 `Source/WacomTests/Private/UI/RunRewardPickupCredentialSpec.cpp` 验证 Actor 只转发 Definition 且保持既有 Gold/Card toast/失败表现；不扩展巨型 `WorldInteractionAndShopSpec.cpp`
- [x] T016 编译 `WacomEditor`，运行 `Wacom.Run.Credential`、`Wacom.Run.Pickup` 与 `Wacom.UI.WorldInteraction.RunRewardPickupCredential`，把通知次数、failed-load 和结果写入 `quickstart.md`

**Checkpoint CP2**: Credential 的唯一运行时写 owner、原子 grant 与表现卡解耦均通过；不依赖 FloorEntrance 或 SaveGame。

---

## Phase 5: User Story 2 - FloorEntrance 使用权威 Credential (Priority: P2)

**Goal**: 入口 Preview/Request/Confirm 读取最新 Credential，并与旧持有卡条件采用 AND。

**Independent Test**: 只有 Credential 可通过 credential-only 入口；只有同名卡不可通过；mixed 条件必须全部满足；Confirm 重新校验。

- [x] T017 在 `Source/WacomTests/Private/Run/RunFloorTransitionSpec.cpp` 增加 credential-only、card-only、mixed AND、非消耗与 Request/Confirm 最新状态覆盖
- [x] T018 在 `Source/WacomRun/Private/Exploration/RunFloorTransitionModule.cpp` 使用私有 Credential evaluator 求值 `RequiredCredentialIds`，保持已解锁入口和 OwnedCardRequirements 行为
- [x] T019 在 `Source/WacomTests/Private/Data/MapDefinitionValidationSpec.cpp` 增加空/重复 requirement、缺失来源、可绕过来源和支配入口来源测试
- [x] T020 在 `Source/WacomEditor/Private/Validation/WacomMapDefinitionValidation.cpp` 扩展 Credential requirement 与固定 Pickup grant 支配校验，不改 Node/Edge/MapPosition 合同
- [x] T021 编译 `WacomEditor`，运行 `Wacom.Run.FloorTransition` 与 `Wacom.Data.Map.Validation`，记录图校验与结果到 `quickstart.md`

**Checkpoint CP3**: Runtime 入口和 Editor authoring gate 共享同一稳定 CredentialId 语义，旧卡牌条件回归通过。

---

## Phase 6: User Story 3 - SaveGame v4 持久化 (Priority: P3)

**Goal**: Credential 以确定数组进入 v4，旧 v3 确定迁移为空，非法磁盘数据原子拒绝。

**Independent Test**: `Wacom.Run.Save` 覆盖排序 roundtrip、v0/v1/v2/v3 链、未来版本拒绝、空/重复 Credential 零状态/零通知修改。

- [x] T022 在 `Source/WacomTests/Private/Run/SaveGameRoundtripSpec.cpp` 只更新 v4 静态断言/既有迁移矩阵，并在小型 `Source/WacomTests/Private/Run/RunCredentialSaveSpec.cpp` 增加确定排序、roundtrip、空/重复拒绝与失败原子性
- [x] T023 在 `Source/WacomRun/Public/WacomSaveGame.h` 增加 v4 Credential 数组并更新版本断言，在 `Source/WacomRun/Private/WacomSaveGame.cpp` 增加明确 v3→v4 空集合迁移
- [x] T024 在 `Source/WacomRun/Private/Save/RunSaveGameSerializer.cpp` 实现排序写入、严格读取校验与临时 RunState 恢复
- [x] T025 编译 `WacomEditor`，运行 `Wacom.Run.Save`，记录迁移、确定性、非法数据原子拒绝结果到 `quickstart.md`

**Checkpoint CP4**: Credential 自身具备稳定 v4 磁盘合同；SaveGame 总开关和其它未持久化 Run 状态仍保持原边界。

---

## Phase 7: User Story 4 - Production authoring 防软锁收口 (Priority: P4)

**Goal**: 以长期 Docs 和最终回归证明 Production Floor 1 可以在下一资产轮安全引用 Credential，而 Debug 内容没有被晋升。

**Independent Test**: 所有定向前缀通过；源码/Docs 身份审计只在通用合同和 Floor 1 设计记录中出现稳定 ID；Content 二进制零变化。

- [x] T026 同步 `Docs/WacomRun.md`、`Docs/WacomMap.md`、`Docs/WacomData.md`、`Docs/WacomDataAuthoring.md` 与 `Docs/Architecture.md` 的 Credential、FloorEntrance、v4 与 authoring 事实
- [x] T027 更新 `Docs/TODO.md`：完成 Credential 规则前置但保留 Floor 2/3、Production 内容和正式场景制作；更新 `Docs/Questions.md` 关闭凭证防移除问题
- [x] T028 运行完整 `WacomEditor` 编译及全部受影响定向前缀，审计 Blueprint/Asset load、Automation failed-load、`rg` 稳定身份、`.uasset/.umap` 零 diff、LFS 与 `git diff --check`
- [x] T029 在 `specs/008-run-credential/quickstart.md` 填写全部 checkpoint 日志、哈希/跳过项/PIE 风险/changed files/final commit 前状态，并逐项勾选本文件
- [x] T030 按 `Docs/AgentIntegrationWorkflow.md` 提交一个独立实现 commit，确认 worktree/LFS 干净，不 merge、不 push

**Checkpoint CP5**: 代码、测试、长期 Docs 与交接记录收口，下一轮可开始 Production DataAsset/正式场景制作前置审计。

---

## Dependencies & execution order

```text
Phase 1 preflight
  -> Phase 2 contracts
  -> Phase 3 reflected data fields (CP1)
  -> Phase 4 Run authority + atomic pickup (CP2)
  -> Phase 5 entrance + authoring validation (CP3)
  -> Phase 6 SaveGame v4 (CP4)
  -> Phase 7 docs + full regression + commit (CP5)
```

- 不在 CP1 前修改 Run 事务；否则运行时会先于静态数据合同。
- 不在 CP2 前接 FloorEntrance；否则入口没有权威查询源。
- 不在 CP3 前宣称软锁解除；runtime 与 validator 必须同时完成。
- SaveGame v4 在运行态字段稳定后实施，避免迁移追逐临时结构。
- App Actor 只在 Run API 完成后接入，不把 grant 逻辑复制到 WacomApp。

## Review checklist

- [x] Public API 保持窄，只读查询与数据驱动结算入口之外无任意写 API。
- [x] Credential 算法位于 `WacomRun/Private`，Data/App/Editor 不成为权威状态 owner。
- [x] 无蛇印/Floor 1 硬编码运行时分支。
- [x] 无 GameplayTag、Build.cs、模块依赖、Battle contract、资产或 SaveGame 总开关改动。
- [x] 每个 checkpoint 已编译并运行对应最小测试。
- [x] 长期事实已同步 Docs，Spec Kit 不成为唯一规则来源。
