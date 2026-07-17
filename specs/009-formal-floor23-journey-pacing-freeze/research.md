# Research: 正式 Floor 2/3 图与 Journey 总节奏冻结

## Decision 1 — 两层都使用 20 节点/21 边双分岔骨架

**Decision**: `Floor.Main.02` 与 `Floor.Main.03` 都沿用 Floor 1 的“两轮二选一、各自汇合”结构和相同制作规模。

**Rationale**: 三层具有一致的地图阅读成本、Camp 密度、Path/Host 制作预算和静态审计方式；内容类型、顺序和终局链足以形成难度递进，不需要为差异而引入三岔或新节点类型。

**Alternatives considered**:

- Floor 3 改成一次三岔：增加 BranchTarget/路线制作复杂度，不能明显改善当前终局目标。
- 允许 18–22 节点浮动：更自由，但会让 Production 预算和 Journey AP 预测提前漂移。
- 复制 Floor 1 所有类型分布：终层仍有 Shop/Entrance，削弱主题和终局语义。

## Decision 2 — 结构 ID 允许跨 Floor 复用

**Decision**: 两层继续使用 `Node.Entry/Main/Junction/Route/Key/Core/Guardian/Exit` 与 `Edge.Main/Route` 结构 ID。NodeId/EdgeId 只在各自 Floor 内唯一；跨层引用使用 `FWacomMapNodeHandle(FloorId, NodeId)`，内容 Host 使用 `<FloorId>.<NodeId>`。

**Rationale**: 结构 ID 保持制作可读性并避免主题替换导致身份迁移；Floor qualification 已能消除跨层冲突。

**Alternatives considered**:

- 在每个 NodeId 中重复 Floor 编号：冗余且降低图表可读性。
- 内容语义 NodeId：后续替换事件或奖励时容易产生身份漂移。
- 全 Journey 裸 NodeId 唯一：与当前数据合同作用域不一致。

## Decision 3 — Floor 2 提高 Encounter 密度但保持 Floor 1 AP 区间

**Decision**: Floor 2 类型分布为 `4 Navigation / 7 Encounter / 3 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`。Route C 使用 Shop + Encounter；其余路径保持每个内容节点默认 1 AP。

**Rationale**: 中层比浅林多一个 Encounter、少一个 Event，主题压力提升；可计费内容总数仍为 14，加上可选 Shop 首次交易得到完整探索 14–15 AP。

**Alternatives considered**:

- 增加总节点：会改变已选的相近规模。
- 用更多 Treasure 代替 Event：降低洞窟中层战斗递进。
- 移除 Floor 2 Shop：三层只剩首层一次经济整理，Production 平衡空间过窄。

## Decision 4 — 蜕印沿用表现卡与持久 Credential 双层合同

**Decision**: `Pickup.MoltCavern.MoltSeal` 预留 `Card.Run.MoltSeal` 主奖励与 `Credential.Run.MoltSeal` grant；`Node.Exit.01` 只在 `RequiredCredentialIds` 引用 Credential，目标是 `Floor.Main.03`。

**Rationale**: 玩家仍获得可见任务物，但入口资格不依赖可移除的实体卡。现有通用 Credential 集合、Pickup grant 和 validator dominance 合同已经支持任意稳定 FName，无需 Floor 2 特例。

**Alternatives considered**:

- 只授予 Credential：规则安全，但缺少可见任务物和与蛇印一致的反馈。
- 入口检查 Card：重新引入已解决的软锁。
- 同时检查 Card 与 Credential：表现卡移除后仍会锁死，违背双层合同目的。

## Decision 5 — Floor 3 不设 Shop 或 FloorEntrance

**Decision**: Floor 3 类型分布为 `4 Navigation / 8 Encounter / 4 RunEvent / 4 Treasure`。终层没有 Shop，最终 Guardian 不是 FloorEntrance 的前置，而是图的无出边终点。

**Rationale**: 毒巢核心应比前两层更集中于战斗与风险；Journey 已无下一 Floor，不应创建空 TargetFloorId 或伪入口。

**Alternatives considered**:

- 保留终局 Shop：提供经济整理，但削弱核心区压迫感且引入第三个 Shop AP 区间。
- 添加指向不存在 Floor 4 的 Entrance：违反现有 TargetFloorId 合同。
- 让 Guardian 指向一个 Navigation 尾节点：不增加规则价值，只会稀释“直接结束 Journey”。

## Decision 6 — 第二轮汇合使用必经核心恩赐

**Decision**: Floor 3 C/D 汇合到 Treasure `Node.Core.01`，内容 ID 为 `Pickup.VenomCore.CoreBoon`；之后进入 Junction、两个终局前 Encounter 和 Guardian。

**Rationale**: 在三连 Encounter 前提供一次明确奖励和节奏停顿，同时让 Core 节点成为可验证的终局链支配点。

**Alternatives considered**:

- 汇合处再放 Encounter：类型变为 9 Encounter/3 Treasure，终局连续战斗过密。
- 使用 RunEvent：会把完整类型分布改为 5 Event，并弱化“核心恩赐”的明确回报。
- 直接汇合到 Junction：节点数不足且缺少终局前准备点。

## Decision 7 — 两层四个 Navigation 全部允许 Camp

**Decision**: `Node.Entry`、`Node.Junction.01`、`Node.Junction.02`、`Node.Junction.03` 的 `bAllowsCamp=true`；内容节点默认 false。

**Rationale**: 延续 Floor 1 的可预测恢复落点和最近合法节点算法，支持多日 Journey 节奏。Camp 仍是 Night 事务，不是 Map Node 或免费恢复。

**Alternatives considered**:

- 只允许两个汇合点：压力更强，但对较长中/终层的回退和恢复不友好。
- 新增专用 Camp 节点：违反现有节点类型合同。

## Decision 8 — 总 AP 与天数同时冻结

**Decision**:

```text
Floor 1: shortest 8–9, full 14–15
Floor 2: shortest 8–9, full 14–15
Floor 3: shortest 10,  full 16
Journey: shortest 26–28, full 44–46
```

关键推进目标约 3 天；完整探索正常目标约 5–6 天。积极 Night Exploration、完整利用 Sunrise 的理论下界约 4 天。

**Rationale**: 每日固定 Morning Planning 已占 1 AP；正常 Camp 会消费并结束 Night、跳过 Sunrise，内容吞吐通常约 8–10 AP/天。44–46 内容 AP 因此形成约 5 天主体，加上恢复、路线回访或非内容行为留出第 6 天空间；26–28 AP 可在约 3 天完成。

**Alternatives considered**:

- 4–5 天完整目标：要求接近连续 Night Exploration，与正常 Camp 体验不符。
- 6–7 天完整目标：需要额外内容、空节点或更低日吞吐，可能触发不必要的 Floor overstay。
- 修改时段预算/压力：扩大到已落地 Run 规则，超出本轮。

## Decision 9 — Guardian 只冻结终点语义，不伪造成功实现

**Decision**: `Floor.Main.03.Node.Guardian.01` 为 `bBoss=true`、无出边的 Journey terminal design identity。当前运行时仍只会 Resolve Encounter；`bRunActive=false` 保持 Defeat 语义。Production 激活前必须另案定义 Journey success state、event/result、总结页和返回目标。

**Rationale**: 当前 `FWacomMapFloorEntrancePayload` 必须指向更后 Floor，`FRunState` 没有完成状态，Battle settlement 只在 Defeat 清除 active。文档必须区分“静态图可表达终点”和“运行时已能结束 Journey”。

**Alternatives considered**:

- 复用 Defeat 的 `bRunActive=false`：无法区分成功/失败并污染后续统计、UI 和存档。
- 在文档中假设 App 检查 Actor/NodeId：把规则泄漏到表现层并形成硬编码。
- 本轮一并实现 success schema：显著扩大到 Run/App/Save/UI contract，不符合纯设计冻结授权。

## Decision 10 — Production readiness 继续分阶段

**Decision**: 本轮关闭 Floor 2/3 图和 Journey 时长 blocker；仍保留三项硬门禁：通用 Journey success contract、三层共 46 个非 Debug Production definitions、Production DataAsset/map 的 AssetRegistry/引用/哈希权威审计。

**Rationale**: 图设计完成不等于资产内容、世界场景或终局流程已交付；继续分阶段可避免空壳资产、Debug 晋升和盲目二进制覆盖。

**Alternatives considered**:

- 立即创建空 Floor DataAsset：可能通过部分 validator，却没有内容或终局能力。
- 复用 Debug definitions：污染 Production 权威。
- 修改 `L_Exploration`：未经过资产权威审计，违反二进制安全边界。
