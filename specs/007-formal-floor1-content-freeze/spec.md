# Feature Specification: 正式 Floor 1 内容设计与稳定身份冻结

**Feature Branch**: `codex/run-level-authoring-baseline`

**Created**: 2026-07-17

**Status**: Approved for documentation implementation

**Input**: 冻结三层 Journey 骨架、正式 Floor 1 的 20 节点/21 边拓扑、稳定身份、内容槽与 Action Point 节奏；保持 Authoring、Debug 与未来 Production 资产隔离，并明确蛇印入口条件的软锁阻塞。

## Wacom Rule Context

**Primary Domain**: Run-exploration / Data authoring / Architecture / Testing

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`

**Expected Owning Module(s)**: 本轮只冻结设计事实，不修改模块；未来静态定义归 `WacomData`，Run 规则归 `WacomRun`，场景接入归 `WacomApp`，制作校验归 `WacomEditor`。

**Non-Goals / Boundaries**:

- 不创建或修改 Journey/Floor/Card/Encounter/Event/Shop/Pickup DataAsset，不修改 `.uasset/.umap`。
- 不修改 C++、Blueprint、GameplayTag、SaveGame schema、Snapshot、Command、Resolution、Build.cs 或模块依赖。
- 不设计 Floor 2/3 的节点图、正式 Camp Activity、事件具体选项数值、商店库存、奖励卡数值或正式世界场景。
- 不把现有 Authoring/Debug 身份、带 `Debug` 语义的内容资产或当前 8 节点基线晋升为正式内容。
- 不解除蛇印入口条件的生产阻塞；任务凭证防移除或恢复规则必须另案确认。

**Open Rule Questions**:

- 无本功能内待确认项。蛇印任务凭证的保留/恢复机制是已知的后续阻塞，不在本轮静默决定。

## User Scenarios & Testing

### User Story 1 - 使用稳定的正式地图身份 (Priority: P1)

作为后续关卡、内容、存档和测试的制作人员，我需要一套与 Authoring/Debug 分离的 Journey、Floor、Node、Edge 稳定身份，使不同工种能够引用同一份正式 Floor 1 图而不靠资产名、数组下标或世界坐标猜测。

**Why this priority**: 稳定身份是正式 DataAsset、场景 Actor、未来持久化和跨层内容的共同前置条件。

**Independent Test**: 审阅身份注册表，确认三层身份、20 个 NodeId、21 个 EdgeId 和蛇印 CardId 唯一、完整，并明确哪些字段允许后续调优。

**Acceptance Scenarios**:

1. **Given** 当前 Authoring 与 Debug 身份仍在使用，**When** 制作人员查阅正式身份合同，**Then** 能找到 `Journey.Main.01`、三层 FloorId、全部 Floor 1 NodeId/EdgeId，且没有复用 `Journey.Authoring`、`Floor.Authoring.01` 或 Debug 身份。
2. **Given** 内容 Host 需要跨 Floor 唯一的运行状态 key，**When** 制作人员由 FloorId 与 NodeId 派生 PersistentId，**Then** 不需要再发明独立人工身份，且不同 Floor 不会冲突。
3. **Given** 地图排版或世界摆放需要调整，**When** 修改 DisplayName、描述、MapPosition 或世界 Transform，**Then** 已冻结身份保持不变。

---

### User Story 2 - 按正式 Floor 1 节奏制作内容 (Priority: P2)

作为地图与内容策划，我需要一张完整、可审计的双分岔 Floor 1 图，明确每个节点类型、内容职责、预留生产内容 ID、Camp 落点与 AP 目标，从而让后续内容制作不再沿用 8 节点调试基线。

**Why this priority**: 只有内容密度与拓扑冻结后，Encounter、RunEvent、Shop、Treasure 和正式关卡才能独立排期。

**Independent Test**: 静态验证 Floor 1 恰好为 20 Nodes / 21 Edges，类型分布为 4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance，所有节点从 Entry 可达，两轮分支均汇合。

**Acceptance Scenarios**:

1. **Given** 玩家从 `Node.Entry` 开始，**When** 沿任一第一轮支路和任一第二轮支路前进，**Then** 都会经过保证蛇印、精英、守卫和 Floor 2 入口。
2. **Given** 玩家选择最短推进路线，**When** 按现有 AP 口径完成必需内容，**Then** 预计消费 8–9 AP。
3. **Given** 玩家完整探索所有支路，**When** 所有一次性内容成功结算，**Then** 预计消费 14–15 AP，差值只来自 Shop 是否发生首次成功交易。
4. **Given** 玩家在 Night 请求 Camp，**When** 最近已完成结构节点可用，**Then** 四个 Navigation 节点都属于允许 Camp 的设计落点，而 Camp 仍不是节点类型。

---

### User Story 3 - 在安全边界内规划生产实现 (Priority: P3)

作为后续实现负责人，我需要清楚区分已经冻结的正式事实、仍可调的表现字段、尚未制作的内容规则和阻塞生产资产落地的软锁风险，避免把设计文档误当成可立即生成资产的授权。

**Why this priority**: 蛇印是普通持有卡时可能被永久移除；如果不显式阻塞，正式入口可能产生不可恢复软锁。

**Independent Test**: 文档明确列出 Production 预留路径、禁止引用集合、蛇印支配关系与软锁阻塞条件，并明确本轮没有二进制或运行时改动。

**Acceptance Scenarios**:

1. **Given** 当前只有带 Debug 语义的 Event/Shop/Reward 资产，**When** 后续团队查看内容槽合同，**Then** 只能看到新的预留生产 ID，不能把 Debug 资产作为正式引用。
2. **Given** `Card.Run.SerpentSigil` 可能被卡牌移除流程移走，**When** 团队评估创建正式 Journey/Floor DataAsset，**Then** 文档明确拒绝继续，直到任务凭证不可移除或可恢复的正式规则完成。
3. **Given** Floor 2/3 只有身份和主题职责，**When** 团队评估让三层 Journey 通过校验，**Then** 文档明确要求先完成有效的 Floor 2/3 图，不创建临时空壳或伪正式图。

### Edge Cases

- 任何正式 NodeId、EdgeId、FloorId、JourneyId 或蛇印 CardId 重复、为空或复用 Authoring/Debug 身份时，设计冻结无效。
- 任一节点不可从 Entry 到达、任一分支不能汇合到蛇印、或存在绕过蛇印到达入口的路径时，拓扑无效。
- MapPosition 超出 1920×1080 设计画布、非有限或完全重合时，默认布局无效；后续合法调图不改变身份。
- Shop 空手离开消费 0 AP，首次成功交易消费 1 AP；AP 目标必须同时记录两种结果。
- 蛇印保证获得不等于保证保留；Validator 当前不能证明玩家离开节点后仍持有蛇印。
- Production 内容 ID 只是本轮冻结的内容槽合同，不代表对应资产、规则数值或场景 Host 已经交付。

## Requirements

### Functional Requirements

- **FR-001**: 必须冻结 `Journey.Main.01` 以及 `Floor.Main.01`、`Floor.Main.02`、`Floor.Main.03` 的顺序与主题职责。
- **FR-002**: 必须冻结 Floor 1 的 20 个 NodeId、21 个 EdgeId、节点类型、连线关系、默认 MapPosition 与内容槽职责。
- **FR-003**: 必须冻结 `Card.Run.SerpentSigil`，并把 `Node.Key.01` 设计为所有通往 `Node.Exit.01` 路线的必经 Treasure。
- **FR-004**: `Node.Exit.01` 必须指向 `Floor.Main.02`，持有蛇印的条件非消耗；生产实现必须因蛇印可移除软锁而保持阻塞。
- **FR-005**: 四个 Navigation 节点必须全部允许 Camp；不得新增 Camp 节点类型或 Camp Activity 数值。
- **FR-006**: 内容 Host PersistentId 必须按 `<FloorId>.<NodeId>` 派生，Floor 内 NodeId 和跨 Floor runtime key 不维护两套人工身份。
- **FR-007**: 必须区分冻结字段与可调字段；DisplayName、描述、MapPosition 和世界 Transform 不进入持久化身份合同。
- **FR-008**: 必须为 6 Encounter、4 RunEvent、4 Treasure 和 1 Shop 预留新的 Production 内容 ID，并明确禁止引用带 Debug 语义的现有内容资产。
- **FR-009**: 必须记录最短推进 8–9 AP、完整探索 14–15 AP 的演算，并说明 Shop 首次成功交易造成的 1 AP 差值。
- **FR-010**: 必须保持 Authoring baseline、Debug fixture 与未来 Production 资产三方隔离，不改变现有资产权威或 builder 写集合。
- **FR-011**: 必须把冻结事实同步到长期 Docs，把已解决问题从问题清单移除，并登记任务凭证、Floor 2 图、生产内容和正式场景后续任务。
- **FR-012**: 本轮只能修改 Markdown 与 Spec Kit 定位元数据，不得修改源码、二进制资产或运行时合同。

### Wacom-Specific Requirements

- **Docs-first evidence**: 更新 `Docs/WacomMap.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md`、`Docs/Questions.md`；`Docs/Architecture.md` 无事实变化。
- **Module/API boundary**: 不新增或修改公共 API；未来所有权继续遵循 `WacomData` 静态定义、`WacomRun` 规则、`WacomApp` 场景适配、`WacomEditor` 校验。
- **Data/GameplayTag impact**: 无 schema、DataAsset 或 GameplayTag 改动；只记录未来 Production 资产合同。
- **Run contract impact**: 无 `URunSession`、`FRunState`、Snapshot、Command、Resolution 或 SaveGame 改动。
- **UI/App boundary**: 无 UI、输入、生命周期或场景行为改动。
- **Testing expectation**: 文档静态审计、跨工件一致性、身份/计数/可达性/AP 演算与 `git diff --check`；编译、Automation、AssetRegistry、Blueprint 和 PIE 明确不适用。
- **Temporary debt**: 蛇印任务凭证保留/恢复规则是显式生产阻塞，登记到 `Docs/TODO.md` 与 `Docs/Questions.md`，不使用临时代码绕过。

### Key Entities

- **Journey skeleton**: `Journey.Main.01` 与三层不可逆顺序，只完整定义 Floor 1。
- **Floor 1 graph**: 20 个节点、21 条有向边、节点内容槽和默认地图布局。
- **Stable identity registry**: JourneyId、FloorId、NodeId、EdgeId、蛇印 CardId，以及由 FloorId/NodeId 派生的 Host PersistentId。
- **Production content slot**: 为未来 Encounter/Event/Shop/Pickup 预留的内容 ID、职责、风险和收益类别，不包含具体数值资产。
- **Production blocker**: 蛇印可能被移除以及 Floor 2/3 尚无有效图，二者阻止正式 Journey/Floor DataAsset 落地。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 身份注册表包含 1 个 JourneyId、3 个 FloorId、20 个唯一 NodeId、21 个唯一 EdgeId 和 1 个蛇印 CardId，重复与空值均为 0。
- **SC-002**: Floor 1 的 20 个节点全部从 Entry 可达，两组分支均汇合，所有到达 Guardian/Exit 的路径都经过蛇印节点。
- **SC-003**: 节点类型数量与批准分布完全一致，所有默认 MapPosition 合法且无重合。
- **SC-004**: 最短路线和完整探索 AP 演算分别落在 8–9 与 14–15，且每一项成本能追溯到现有长期规则。
- **SC-005**: 所有正式内容槽使用新的 Production ID，引用 Authoring/Debug 内容资产的数量为 0。
- **SC-006**: 长期 Docs 与 Spec Kit 工件对正式身份、可调字段、AP 节奏和生产阻塞的表述一致，遗留模糊问题为 0。
- **SC-007**: 本轮变更不包含任何源码、Build.cs、Config、`.uasset` 或 `.umap` 文件。

## Assumptions

- 当前时段预算、节点活动消耗、Camp、Floor Transition 与地图校验合同保持 `Docs/WacomMap.md` 的现状。
- `Card.Run.SerpentSigil` 是 FName 内容身份，不是 GameplayTag；本轮不创建对应卡牌。
- 预留 Production 内容 ID 在后续内容制作中作为资产内 ID 使用，但具体选项、库存、奖励数值和敌人组合另案冻结。
- Production 资产未来使用 `/Game/Wacom/Data/Map/Production` 与 `/Game/Wacom/Maps/Run`，本轮只保留路径合同。
- 纯文档改动不需要 Unreal 编译、Automation 或 PIE；跳过项和原因必须记录在 quickstart。
