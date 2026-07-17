# Feature Specification: 正式 Floor 2/3 图与 Journey 总节奏冻结

**Feature Branch**: `codex/run-level-authoring-baseline`

**Created**: 2026-07-17

**Status**: Approved for documentation implementation

**Input**: 在既有 `Journey.Main.01` 与正式 Floor 1 冻结基础上，完整冻结 Floor 2/3 的 20 节点/21 边图、Production 内容槽、跨层蜕印资格、终局 Guardian 语义以及三层 Journey 的 Action Point 与目标天数；本轮不修改运行时或资产。

## Wacom Rule Context

**Primary Domain**: Run-exploration / Data authoring / Content design / Testing

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `specs/007-formal-floor1-content-freeze/`
- [x] `specs/008-run-credential/`

**Expected Owning Module(s)**: 本轮只冻结设计事实，不修改模块。未来静态 Journey/Floor/内容定义归 `WacomData`，探索与 Journey 成功结算归 `WacomRun`，场景和终局表现归 `WacomApp`，制作校验归 `WacomEditor`。

**Non-Goals / Boundaries**:

- 不创建或修改 Journey/Floor/Card/Encounter/Event/Shop/Pickup DataAsset，不修改 `.uasset/.umap`。
- 不修改 C++、Blueprint、Config、GameplayTag、SaveGame、Snapshot、Command、Resolution、Build.cs 或模块依赖。
- 不实现 Journey 成功状态、完成事件、总结页、返回目标或终局存档；只冻结 `Floor.Main.03.Node.Guardian.01` 的终点语义。
- 不把 `bRunActive=false`、Defeat 或现有 FloorTransition 伪装成 Journey 成功结算。
- 不设计正式敌人组合、RunEvent 选项、商店库存、Pickup 数值、Camp Activity 数值或世界关卡摆放。
- 不引用或晋升 Authoring/Debug 内容，不修改 Debug builder、`L_Exploration` 或 `L_RunExploration_Debug`。
- 不开始 Production DataAsset/场景制作；终局运行时合同、46 个 Production definitions 和资产权威审计仍是后续门禁。

**Confirmed Rule Decisions**:

- Floor 2 与 Floor 3 各固定为 `20 Nodes / 21 Edges`，都沿用两轮二选一分支、分别汇合的结构。
- Floor 2 使用 `Card.Run.MoltSeal` 作为表现、`Credential.Run.MoltSeal` 作为不可丢失资格；Floor 2 Exit 只检查 Credential。
- Floor 3 不设 Shop 或 FloorEntrance，最终 Guardian 无出边，设计语义为胜利后直接完成 Journey。
- Floor 2/3 的四个 Navigation 节点全部允许 Camp。
- Journey 最短推进为 `26–28 AP`，完整探索为 `44–46 AP`；正常体验目标约 5–6 天。

## User Scenarios & Testing

### User Story 1 - 使用完整的正式三层图身份 (Priority: P1)

作为后续 DataAsset、关卡、内容和测试制作人员，我需要 Floor 2/3 拥有与 Floor 1 同等级的稳定 Node/Edge 注册表，使 `Journey.Main.01` 不再依赖空壳层或临时 Debug 图。

**Why this priority**: 有效 Floor 图是 Production Journey、跨层入口、场景 Host 和未来存档身份的共同前置。

**Independent Test**: 静态审计确认 Floor 2 与 Floor 3 各有 20 个唯一 NodeId、21 个唯一 EdgeId、全部节点从 Entry 可达，且没有 Authoring/Debug 身份或资产引用。

**Acceptance Scenarios**:

1. **Given** 制作人员查阅正式 Journey 合同，**When** 定位 `Floor.Main.02` 或 `Floor.Main.03`，**Then** 能找到完整 Node/Edge、默认 MapPosition、节点类型、Camp 和内容槽表，而不是“图未设计”占位。
2. **Given** 三层允许复用结构化 NodeId/EdgeId，**When** 形成跨层运行时 key，**Then** 必须使用 FloorId + NodeId handle 或 `<FloorId>.<NodeId>` PersistentId，不能只使用裸 NodeId。
3. **Given** DisplayName、MapPosition 或世界 Transform 后续调优，**When** 视觉制作改变这些字段，**Then** 已冻结 Journey/Floor/Node/Edge/Card/Credential 身份保持不变。

---

### User Story 2 - 通过蜕印安全进入终层 (Priority: P2)

作为玩家与内容制作者，我需要 Floor 2 的必经蜕印同时提供可见卡牌与不可丢失 Credential，使玩家即使移除表现卡也不会失去进入 Floor 3 的资格。

**Why this priority**: 这延续已经实现的通用 Credential 合同，避免为第二把钥匙重新引入普通持有卡软锁。

**Independent Test**: 图和内容合同证明 `Node.Key.01` 支配 Guardian/Exit，`Pickup.MoltCavern.MoltSeal` 同时预留 Card/Credential，Exit 仅要求 `Credential.Run.MoltSeal`。

**Acceptance Scenarios**:

1. **Given** 玩家选择任一第一轮和第二轮支路，**When** 前往 `Node.Exit.01`，**Then** 路线都必须经过 `Node.Key.01`、Elite 与 Guardian。
2. **Given** 蜕印表现卡后来被移除，**When** 未来 Production 入口求值，**Then** 设计合同仍只依赖持久 Credential，不允许从卡牌存在性推断资格。
3. **Given** Floor 2 有一个 Shop，**When** 计算最短与完整 AP，**Then** 唯一 1 AP 区间来自首次成功交易，浏览或空手离开仍为 0。

---

### User Story 3 - 按明确总节奏抵达 Journey 终点 (Priority: P3)

作为地图与 Run 策划，我需要 Floor 3 在不增加新节点类型的情况下形成更高战斗密度，并明确最终 Guardian 是 Journey 终点以及 5–6 天总节奏目标。

**Why this priority**: 终层若仍伪装成 FloorEntrance 会扭曲数据合同；若不记录当前运行时缺口，又会让 Production 制作误以为 Journey 已能成功完成。

**Independent Test**: 静态图确认 Floor 3 无 Shop、无 FloorEntrance，Guardian 无出边；AP 模型得到 Floor 3 `10 / 16`、Journey `26–28 / 44–46`，并明确成功结算尚未实现。

**Acceptance Scenarios**:

1. **Given** 玩家完成任一两轮分支组合，**When** 抵达第二次汇合，**Then** 必经 `Node.Core.01` 核心恩赐，再经过两个终局前 Encounter 与 Guardian。
2. **Given** Guardian 战斗胜利，**When** 设计合同描述后续行为，**Then** 它被标记为 Journey 成功终点，但文档不得声称当前 `FRunState`、Result 或 UI 已支持该状态。
3. **Given** 玩家按正常 Camp/阶段节奏探索，**When** 审计三层总 AP，**Then** 关键推进约 3 天，完整探索目标约 5–6 天，积极 Night Exploration 的理论下界约 4 天。

### Edge Cases

- Floor 3 无 FloorEntrance 是合法静态死胡同，但当前运行时只会在 Guardian 胜利后 Resolve Encounter；这不能被描述为已完成 Journey。
- `bRunActive=false` 当前属于 Defeat/Run 失败语义，禁止将其作为成功完成标志。
- `NodeId`、`EdgeId` 只要求 Floor 内唯一；跨层重复结构 ID 是有意设计，跨层引用必须携带 FloorId。
- 蜕印保证获得不等于表现卡保证保留；通行资格只由 `Credential.Run.MoltSeal` 承担。
- Shop 浏览/空手离开为 0 AP，首次成功交易为 1 AP；Floor 1/2 两个 Shop 共同产生 Journey 的 2 AP 上界差值。
- `44–46 AP` 是内容结算成本，不含固定 Morning Planning、Camp 行为和未成功尝试；5–6 天是正常节奏目标，不是强制日数。
- 预留 Production ID 不代表资产、敌人组合、事件数值、奖励或世界 Host 已交付。

## Requirements

### Functional Requirements

- **FR-001**: 必须冻结 Floor 2 的 20 个 NodeId、21 个 EdgeId、节点类型、连线、默认 MapPosition、Camp 和 15 个 Production 内容槽。
- **FR-002**: Floor 2 类型分布必须为 `4 Navigation / 7 Encounter / 3 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`。
- **FR-003**: 必须冻结 Floor 3 的 20 个 NodeId、21 个 EdgeId、节点类型、连线、默认 MapPosition、Camp 和 16 个 Production 内容槽。
- **FR-004**: Floor 3 类型分布必须为 `4 Navigation / 8 Encounter / 4 RunEvent / 4 Treasure / 0 Shop / 0 FloorEntrance`。
- **FR-005**: 两层全部节点必须从 `Node.Entry` 可达；A/B 必须汇合到 `Node.Junction.02`，C/D 必须分别汇合到 Floor 2 `Node.Key.01` 和 Floor 3 `Node.Core.01`。
- **FR-006**: Floor 2 `Node.Key.01` 必须支配 Guardian/Exit；`Pickup.MoltCavern.MoltSeal` 必须预留 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal`，Exit 只要求后者且不消费。
- **FR-007**: Floor 3 `Node.Core.01` 必须支配终局链；`Node.Guardian.01` 必须 `bBoss=true`、无出边、无 FloorEntrance，并冻结为 Journey 成功终点设计语义。
- **FR-008**: 四个 Navigation 节点在两层都必须允许 Camp；不得新增 Camp NodeType 或 Camp Activity 数值。
- **FR-009**: 必须冻结 `MoltCavern` 与 `VenomCore` 下共 31 个非 Debug Production 内容 ID，并继续按 `<FloorId>.<NodeId>` 派生内容 Host PersistentId。
- **FR-010**: 默认 MapPosition 必须有限、位于 `[0,1920] × [0,1080]` 且 Floor 内无重合；这些坐标和世界 Transform 不进入稳定身份。
- **FR-011**: AP 合同必须为 Floor 2 `8–9 / 14–15`、Floor 3 `10 / 16`、Journey `26–28 / 44–46`，并记录关键推进约 3 天、完整探索约 5–6 天。
- **FR-012**: 必须把三层完整图、身份、内容槽、节奏和生产门禁同步到长期 Docs，并关闭 Floor 2/3 图与 Journey 总时长问题。
- **FR-013**: 必须新增独立后续阻塞：通用 Journey 成功状态/事件/总结与返回目标；不得复用失败状态或 FloorTransition 绕过。
- **FR-014**: 本轮只能修改 Markdown、`.specify/feature.json` 与 `AGENTS.md` 托管 Spec Kit 指针，不得修改 Source、Config、Content 或运行时合同。
- **FR-015**: 必须使用静态图、身份、坐标、AP、Debug 引用、跨工件和 Git/LFS 审计验证本轮，并明确记录 Unreal 编译、Automation、AssetRegistry、Blueprint 与 PIE 跳过原因。

### Wacom-Specific Requirements

- **Docs-first evidence**: 更新 `Docs/WacomMap.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md`、`Docs/Questions.md`；`Docs/Architecture.md` 与 `Docs/WacomRun.md` 无实现事实变化。
- **Module/API boundary**: 不新增或修改公共 API；未来终局成功状态归 `WacomRun`，表现归 `WacomApp`，不能由 Widget/Actor 直接写 RunState。
- **Data/GameplayTag impact**: 无 DataAsset/schema/GameplayTag 改动；只冻结未来 Production ID 与路径合同。
- **Run contract impact**: 无 `URunSession`、`FRunState`、Snapshot、Command、Resolution 或 SaveGame 改动；显式记录缺失 Journey success contract。
- **UI/App boundary**: 无 UI、输入、焦点或生命周期改动；总结页与返回目标另案确认。
- **Testing expectation**: 静态 Node/Edge/type/reachability/dominance/layout/AP 审计、Spec Kit 跨工件一致性、`rg` 边界检查、`git diff --check` 与 Git/LFS 状态。
- **Temporary debt**: 无临时代码；Journey 成功结算是明确功能阻塞，写入 `Docs/TODO.md` 与 `Docs/Questions.md`。

### Key Entities

- **Floor 2 graph**: 20 节点、21 条有向边、蜕印 guarantee、Guardian 与 Floor 3 入口。
- **Floor 3 terminal graph**: 20 节点、21 条有向边、核心恩赐、终局 Encounter 链与无出边 Guardian。
- **Stable identity extension**: 两层 Node/Edge、31 个内容 ID、`Card.Run.MoltSeal`、`Credential.Run.MoltSeal` 和 Floor-qualified PersistentId。
- **Journey pacing model**: 三层关键/完整 AP、正常 Camp 与 Night Exploration 下的目标天数。
- **Production blocker**: 终局成功运行时合同、46 个 Production definitions 和 Production asset authority audit。

## Success Criteria

### Measurable Outcomes

- **SC-001**: Floor 2 与 Floor 3 各包含 20 个唯一 NodeId、21 个唯一 EdgeId，类型计数与冻结分布完全一致。
- **SC-002**: 两层各 20 个节点全部可达，两组分支正确汇合；Floor 2 Key 支配 Guardian/Exit，Floor 3 Core 支配终局链。
- **SC-003**: Floor 2 Exit 只指向 `Floor.Main.03` 并只要求 `Credential.Run.MoltSeal`；Floor 3 Guardian 无出边、无 FloorEntrance。
- **SC-004**: 31 个新 Production 内容 ID、CardId、CredentialId 非空唯一且不映射任何 Authoring/Debug 资产；默认坐标合法且无重合。
- **SC-005**: AP 演算精确得到 Floor 2 `8–9 / 14–15`、Floor 3 `10 / 16` 与 Journey `26–28 / 44–46`，天数说明与当前时段/Camp 合同不冲突。
- **SC-006**: Spec Kit 工件和四个长期 Docs 对身份、图、节奏、终局阻塞与 Production readiness 表述一致，无 Critical/High/Medium 冲突。
- **SC-007**: 本轮变更不包含 Source、Config、Content、`.uasset/.umap` 或 LFS 对象；纯文档提交后 worktree/LFS 干净。

## Assumptions

- 现有 `2/6/2/2/1` 时段预算、Action Point 成本、三天 Floor Exposure 宽限、Camp 和不可逆 FloorTransition 合同保持不变。
- `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal` 是 FName 内容身份，不是 GameplayTag；本轮不创建对应资产。
- 两层可以复用 Floor 1 的结构 NodeId/EdgeId，因为唯一性作用域是单 Floor；跨层运行时 key 始终携带 FloorId。
- Floor 3 无入口的静态图可通过当前局部结构校验，但 Production 激活仍需通用 Journey 成功结算功能。
- 纯文档改动不需要 Unreal 编译、Automation、AssetRegistry、Blueprint 或 PIE；跳过项和剩余生产风险必须记录在 quickstart。
