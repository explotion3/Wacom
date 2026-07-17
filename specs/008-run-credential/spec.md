# Feature Specification: Run 持久任务凭证

**Feature Branch**: `codex/run-level-authoring-baseline`

**Created**: 2026-07-17

**Status**: Ready for planning

**Input**: 用户接受“蛇印卡负责可见表现，独立 Run Credential 负责不可丢失的永久通行资格”，并要求开始下一轮。

## Wacom Rule Context

**Primary Domain**: Run-exploration / Save-load / Data-card authoring

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `specs/007-formal-floor1-content-freeze/`

**Expected Owning Modules**: `WacomData` 定义静态 Credential 授予/入口需求；`WacomRun` 持有、持久化并求值权威状态；`WacomApp` 只把 Pickup Definition 提交给 Run 并继续显示卡牌奖励；`WacomEditor` 校验可达保证来源；`WacomTests` 提供规则、存档与数据回归。

**Non-Goals / Boundaries**:

- 不创建 `Card.Run.SerpentSigil`、正式 Pickup、Journey/Floor DataAsset 或关卡资产。
- 不启用 `AWacomGameMode::bSaveSystemEnabled`，不把当前部分 SaveGame 宣称为完整 Run 存档。
- 不修改 GameplayTag、Build.cs、模块依赖、Battle Command/Snapshot/Resolution 或 UI 视觉资产。
- 不禁止、拦截或改写实体卡的删除、删牌换金币、RunEvent 支付、世界交互消耗等既有规则；Credential 必须与这些路径解耦。
- 不为 Floor 1、蛇印 CardId 或某张资产写硬编码分支。
- 不扩展 RunEvent Credential effect、商店 Credential 商品、Credential UI 列表或多玩家复制。

**Confirmed Rule Decisions**:

- 稳定 CredentialId 使用独立身份 `Credential.Run.SerpentSigil`；不复用表现卡 `Card.Run.SerpentSigil`，也不绑定 Journey-scoped 路径。
- SaveGame v3→v4 将 Credential 集合初始化为空；不从旧卡牌、Actor、Pickup 或场景状态反推凭证。

## User Scenarios & Testing

### User Story 1 - 获得不可丢失的任务凭证 (Priority: P1)

作为玩家，我在必经蛇印 Treasure 完成拾取时，同时获得可见蛇印卡和独立通行凭证；之后无论实体卡如何离开持有区，通行资格仍保持。

**Why this priority**: 这是消除 Floor 1 软锁的最小规则闭环，也是正式 Production 资产解除阻塞的首要前置。

**Independent Test**: `Wacom.Run.Credential` 在无世界、Widget 或资产保存条件下验证一次 Pickup 事务同时授予卡和 Credential，且各种永久移除路径不改变 Credential。

**Acceptance Scenarios**:

1. **Given** 一个有效 Card Pickup 同时声明一个 CredentialId，**When** 玩家第一次成功结算 Pickup，**Then** 卡牌、Credential、Pickup 完成状态、Treasure 节点结算和 Action Point 必须在同一 working-state 事务中提交并只广播一次。
2. **Given** 玩家已经获得 Credential，**When** 对应实体卡被直接销毁、删牌换金币、RunEvent 支付或世界交互消耗，**Then** Credential 仍存在且不会被这些卡牌事务移除。
3. **Given** CredentialId 为空、重复或 Pickup 主奖励无效，**When** 尝试结算，**Then** 整个事务失败或按数据合同拒绝，不出现“有卡无凭证”或“有凭证无 Pickup 完成状态”的部分提交。
4. **Given** 同一 Credential 被多个合法来源重复授予，**When** 后续来源结算，**Then** 授予是幂等的，状态中只保留一个稳定身份，同时其它合法奖励仍正常提交。

---

### User Story 2 - 入口只依赖权威 Credential (Priority: P2)

作为地图内容制作者，我可以为 FloorEntrance 配置 RequiredCredentialIds，使入口预览、Request 和 Confirm 都读取最新 Run Credential 状态，而不是扫描实体卡。

**Why this priority**: 独立状态只有进入正式授权链才能真正消除软锁；同时必须保留既有 OwnedCardRequirements 给其它内容使用。

**Independent Test**: `Wacom.Run.FloorTransition` 证明只有卡没有 Credential 时拒绝、只有 Credential 没有卡时成功，并继续覆盖 Request/Confirm 重新校验与旧持有卡条件。

**Acceptance Scenarios**:

1. **Given** FloorEntrance 只要求蛇印 Credential，**When** 玩家拥有 Credential 但实体卡已不存在，**Then** Preview、Request 与 Confirm 均允许通行且不消费 Credential。
2. **Given** 玩家只持有同名蛇印卡但从未获得 Credential，**When** 请求入口，**Then** 入口拒绝，避免从表现卡反推权威资格。
3. **Given** 入口同时配置 Credential 与既有 OwnedCardRequirements，**When** 任一组未满足，**Then** 入口拒绝；两组均满足才允许。
4. **Given** Request 后权威状态变化，**When** Confirm 重新校验，**Then** 只按最新 Credential/卡牌事实提交或拒绝。

---

### User Story 3 - 跨 SaveGame 保留通行资格 (Priority: P3)

作为未来恢复存档流程的开发者，我需要 Credential 使用稳定 FName 集合进入版本化磁盘 schema，使资格不依赖卡牌 instance、资产路径或数组位置。

**Why this priority**: “永久资格”必须有明确磁盘合同，但本轮不扩大到启用完整存档。

**Independent Test**: `Wacom.Run.Save` 验证 v4 roundtrip、确定性写入、旧版本迁移、非法空/重复 Credential 拒绝且 RunState 原子不变。

**Acceptance Scenarios**:

1. **Given** RunState 含多个 Credential，**When** Build/Apply SaveGame，**Then** 所有非空唯一 ID 无损恢复，写入顺序确定。
2. **Given** 一个旧 v3 存档，**When** 迁移到 v4，**Then** 按已确认迁移合同得到确定结果，不从 Actor GUID、卡牌 instance 顺序或场景状态猜测。
3. **Given** v4 存档包含空或重复 CredentialId，**When** Apply，**Then** 整体拒绝且当前 RunState、revision 与通知次数不变。
4. **Given** SaveGame 总开关仍关闭，**When** 正常游戏流程运行，**Then** 本轮不会擅自启动自动保存或读取。

---

### User Story 4 - 阻止不可达的 Credential 门槛 (Priority: P4)

作为内容制作者，我需要 Data Validation 证明每个 RequiredCredentialId 在入口前存在保证授予来源，且来源节点支配入口，避免把新的 Credential 系统再次做成软锁。

**Why this priority**: 规则能力必须配套制作校验，不能依赖人工记忆保证钥匙可达。

**Independent Test**: `Wacom.Data.Map.Validation` 和 `Wacom.Data.RunPickup.Validation` 覆盖空/重复 ID、缺失来源、非支配来源与必经 Pickup 来源。

**Acceptance Scenarios**:

1. **Given** 入口要求的 Credential 没有任何前置保证来源，**When** 校验 Journey，**Then** 返回错误。
2. **Given** Credential Pickup 位于可绕过分支，**When** 删除该节点后入口仍可达，**Then** 该来源不能被视为保证来源。
3. **Given** Credential Pickup 位于支配入口的必经节点，**When** 校验 Journey，**Then** 该 Credential 条件通过。
4. **Given** 现有 Debug Pickup 未声明 Credential，**When** 加载和校验，**Then** 旧内容行为保持不变且不被晋升为正式蛇印来源。

### Edge Cases

- CredentialId 为 `NAME_None`、列表重复或 SaveGame 被手工篡改。
- Pickup 主奖励、节点活动或 Action Point 结算失败时 Credential 必须回滚。
- 一个 Pickup 声明多个 Credential，或多个 Pickup 重复声明同一 Credential。
- FloorEntrance 同时配置 RequiredCredentialIds 与 OwnedCardRequirements。
- Request 与 Confirm 之间卡牌变化；Credential 本轮无撤销入口，因此只可能保持或增加。
- 旧 SaveGame 没有 Credential 字段，而正式蛇印 Production 内容此前从未存在。
- 当前 `CollectedPickupIds`、探索图、时间和金币仍未进入 SaveGame；本轮不掩盖这项既有边界。

## Requirements

### Functional Requirements

- **FR-001**: 系统必须在 `FRunState` 中维护独立、非空、去重的稳定 CredentialId 集合，且不从玩家当前持有卡推导。
- **FR-002**: Credential 的授予必须是幂等、不可由普通 UI 直接撤销的 Run 规则事务能力；具体算法保持在 `WacomRun/Private`。
- **FR-003**: `UWacomRunPickupDefinition` 必须可声明零个或多个授予 CredentialId，现有 Gold/Card 主奖励和 Debug 资产默认行为保持不变。
- **FR-004**: 数据驱动 Pickup 必须在同一 working-state 事务中完成主奖励、全部 Credential 授予、`CollectedPickupIds`、Treasure 节点结算和 Action Point；任一失败零提交。
- **FR-005**: 实体卡的销毁、删牌换金币、RunEvent 支付和世界交互消耗不得删除、撤销或重新计算 Credential。
- **FR-006**: FloorEntrance 必须支持 RequiredCredentialIds，并与既有 OwnedCardRequirements 采用 AND；Preview、Request 和 Confirm 都重新读取最新权威状态。
- **FR-007**: FloorEntrance 不得根据同名卡牌、CardDefinition、CardId、关键词、Actor、资产路径或 Pickup 完成状态反推 Credential。
- **FR-008**: Credential 必须写入 `UWacomSaveGame` v4；序列化使用确定顺序，读档拒绝空/重复 ID，并保持失败原子性。
- **FR-009**: v3→v4 迁移必须将 Credential 集合初始化为空，不得从旧卡牌、Actor、Pickup 或场景状态反推，并在 `WacomSaveGame.cpp`、测试和长期文档中固定。
- **FR-010**: 蛇印 Credential 稳定身份必须为 `Credential.Run.SerpentSigil`；它是 FName 内容身份，不是 GameplayTag。
- **FR-011**: Map Data Validation 必须验证 RequiredCredentialIds 非空唯一，并证明每个 ID 在入口前由支配入口的固定 Pickup 保证授予。
- **FR-012**: Pickup Definition Validation 必须拒绝空或重复 GrantedCredentialIds，并保持既有 Gold/Card 配置规则。
- **FR-013**: 本轮不得启用正常 SaveGame 流程、创建/保存二进制资产或修改 GameplayTag、Build.cs 和模块依赖。
- **FR-014**: 长期事实必须同步到 `Docs/WacomRun.md`、`Docs/WacomMap.md`、`Docs/WacomData.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md` 与 `Docs/Questions.md`。
- **FR-015**: 每个可编译切片必须编译 `WacomEditor` 并运行对应 `Wacom.Run.Credential`、`Wacom.Run.FloorTransition`、`Wacom.Run.Save` 和 Data Validation 定向测试。

### Wacom-Specific Requirements

- **Docs-first evidence**: 规则真相归 `WacomRun.md/WacomMap.md`，静态字段和制作合同归 `WacomData.md/WacomDataAuthoring.md`，阻塞完成状态归 `TODO/Questions`。
- **Module/API boundary**: `WacomData` 只定义反射字段；`WacomRun` 是唯一权威写 owner；`WacomApp` 只提交 Definition 和消费结果；`WacomEditor` 只做静态校验。
- **Data/GameplayTag impact**: 新增 Pickup/FloorEntrance FName 字段和 validator；不新增 GameplayTag，不创建或迁移资产。
- **Battle contract impact**: 无。实体卡进入战斗后的规则与 Credential 无关。
- **Run contract impact**: 新增 RunState credential set、只读查询/数据化 Pickup 结算入口、Floor transition evaluator 与 SaveGame v4。
- **UI/App boundary**: 不新增 UI；现有 RewardPickup Actor 继续显示 Card toast，并把完整 Definition 交给 Run 原子结算。
- **Testing expectation**: 小型 `RunCredentialSpec.cpp`，扩展 FloorTransition/Save/Data validation 小型覆盖；完整 `WacomEditor` 编译，命令统一加 `-NoDreamShaderEditorBridge`。
- **Temporary debt**: SaveGame 仍是关闭且不完整的部分 schema；本轮只保证 Credential 字段自身正确，不把其它未持久化 Run 状态顺带纳入 v4。

### Key Entities

- **Run Credential State**: `FRunState` 中非空唯一 FName 集合；本次 Run 的通行/任务资格权威真相。
- **Credential Grant**: 静态 Pickup Definition 声明的零个或多个 CredentialId；结算时原子、幂等授予。
- **Credential Requirement**: FloorEntrance 声明的 RequiredCredentialIds；与 OwnedCardRequirements 一起求值。
- **Credential Save Schema**: v4 中稳定、确定排序的 FName 数组；读档后恢复为集合。
- **Presentation Card**: `Card.Run.SerpentSigil`，只作为玩家可见卡牌内容，不是授权真相。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 获得蛇印 Credential 后，覆盖四类卡牌永久移除语义的测试均证明 Credential 数量和身份不变。
- **SC-002**: 只有 Credential、没有实体卡时 FloorEntrance 可通过；只有实体卡、没有 Credential 时明确拒绝。
- **SC-003**: v4 SaveGame 对 Credential 做无损 roundtrip，v3 迁移结果确定，非法空/重复条目零状态修改。
- **SC-004**: Data Validation 能拒绝缺失来源或可绕过来源，并接受一个支配入口的固定 Credential Pickup。
- **SC-005**: 现有 Debug Pickup、OwnedCardRequirements、Gold/Card Pickup 和卡牌移除测试保持通过。
- **SC-006**: 变更不包含 `.uasset/.umap`、GameplayTag、Build.cs、模块依赖、Battle contract 或 SaveGame 总开关。

## Assumptions

- 正式蛇印 Production Pickup 和 Floor DataAsset 尚不存在，因此旧 v3 存档不可能合法包含已发放的正式蛇印 Credential。
- Credential 本轮只增不减；未来可撤销任务状态必须另案定义来源、审计和存档语义。
- 一个 Pickup 可以授予多个 Credential，集合授予幂等；本轮唯一冻结的生产用例是蛇印。
- Credential 不进入通用 Run UI Snapshot；FloorTransitionPreview 的 `bRequirementsMet` 足以服务当前入口 UI。
- 当前 SaveGame 总开关继续关闭，PIE 无正式 Production 内容可验；规则和数据合同由 Automation 验证。
