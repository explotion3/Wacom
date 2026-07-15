# Feature Specification: Run 探索规则核心重构

**Feature Branch**: `main`

**Created**: 2026-07-14

**Status**: Ready for Planning

**Input**: User description: "彻底重构 Run 探索规则核心，建立非连续有向地图、行动点、楼层暴露、露营与跨层规则；保留已验证的第一人称外围表现，并迁移现有探索场景作为可玩验证。"

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Run-exploration / Data-card authoring / UI-App shell / Architecture

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/Game_Design.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`

**Expected Owning Module(s)**: WacomData / WacomRun / WacomApp / WacomEditor / WacomTests

**Non-Goals / Boundaries**:
- 不制作正式 Map Screen、Floor Transition Modal、Camp Screen 或 Camp Activity 内容。
- 不决定首版正式 Floor 数量、节点密度、整段旅程天数或 Camp 恢复数值。
- 不实现随机地图、runtime PCG、地图响应式排版或由世界坐标生成规则图。
- 不启用或扩展 SaveGame，不固化地图存档 schema。
- 不修改 Battle 内部规则、卡牌资产语义、玩家档案或设置系统。
- 调试 Journey 只验证规则和外围接缝，不作为正式内容或平衡样板。

**Open Rule Questions**:
- 无。本轮实现所需规则口径已在 `Docs/WacomMap.md` 及本规格中确认；正式内容规模和 Camp Activity 数值留在后续独立规格。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 在非连续有向地图中可靠探索 (Priority: P1)

玩家能够从当前地点选择一条合法通道前进，在到达通道终点后才进入目标地图节点。玩家可以后退到通道起点取消前进，也可以在死胡同中打开地图并免费返回同层已经完成的节点，而不会让逻辑位置与画面位置分离。

**Why this priority**: 这是所有 Run 内容的空间基础；如果当前位置、节点状态和场景表现不能可靠同步，战斗、事件、商店、跨层与未来存档都无法建立稳定身份。

**Independent Test**: 使用一张包含分支、死胡同和已完成节点的最小 Floor，验证选择、前进、后退取消、到达提交和同层返回，不依赖战斗或 Camp 内容。

**Acceptance Scenarios**:

1. **Given** 玩家位于一个已访问节点且没有活动事务，**When** 选择一条从当前节点出发的合法通道并到达终点，**Then** 当前节点切换到目标节点，目标节点进入已访问状态，并揭示后续可达节点。
2. **Given** 玩家已开始沿通道前进但尚未到达终点，**When** 后退并回到通道起点，**Then** 本次前进被取消，逻辑当前节点保持不变。
3. **Given** 玩家到达没有可继续前进通道的节点，**When** 选择同层已经完成的节点进行地图传送，**Then** 玩家免费到达目标节点，行动点和压力不变。
4. **Given** 玩家正处于战斗、事件、露营、跨层确认或通道行进事务，**When** 尝试地图传送，**Then** 请求被拒绝且任何状态均不改变。
5. **Given** 玩家首次进入一个 Floor，**When** 构建探索 Snapshot，**Then** Floor Entrance 与 Boss 可作为远景轮廓显示，但其路径、门槛和具体内容仍按正常揭示规则保持隐藏。

---

### User Story 2 - 通过内容行动推进时段与压力 (Priority: P1)

玩家移动和查看地图不会消耗时间；只有成功完成明确内容行动才消耗 Action Point。行动点耗尽后时段按固定顺序推进，长期停留同一 Floor 会通过 Decay 逐日形成压力。

**Why this priority**: Action Point 与 Floor Exposure 共同构成探索的机会成本和长期时钟，必须与 Map Node 数量彻底分离。

**Independent Test**: 在无 UI 的规则测试中完成一整天的行动点消耗、Night 选择、Camp 特殊推进和多个 FloorDay 的 Decay 计算。

**Acceptance Scenarios**:

1. **Given** 当前时段行动点不足，**When** 玩家提交一个有固定成本的内容行动，**Then** 整个行动被拒绝，不产生部分奖励、压力、节点完成或时段推进。
2. **Given** 玩家恰好用尽当前时段行动点，**When** 内容事务成功提交，**Then** 时段推进一次并加载下一时段预算。
3. **Given** 新旅程处于第一天，**When** 初始化完成，**Then** 不立即增加 Decay；之后每次进入新一天 Morning 时恰好结算一次每日 Decay。
4. **Given** 玩家在当前 Floor 停留到第 4 天及以后，**When** 新一天开始，**Then** Daily Decay 等于 Base Decay 与该 FloorDay 的 Overstay Decay 之和。
5. **Given** 玩家在 Night 完成一次 Camp Activity，**When** Camp 结算成功，**Then** 剩余 Night 行动点被放弃、Sunrise 被跳过并进入次日 Morning。

---

### User Story 3 - 原子结算战斗、事件、商店与宝藏 (Priority: P1)

玩家在地图节点上进入战斗、RunEvent、Shop 或 Treasure 时，内容结果、奖励、压力、行动点和节点生命周期由同一次规则事务结算。失败或过期提交不会留下半完成状态。

**Why this priority**: 现有各入口自行扣“节点数”会产生重复消费、错误时段推进和 UI/规则状态分叉，必须在地图内容正式接入前统一。

**Independent Test**: 分别用最小 Encounter、RunEvent、Shop 和 Treasure fixture 验证成功、失败、取消、重复提交和行动点不足。

**Acceptance Scenarios**:

1. **Given** Encounter 开始前有至少 1 Action Point，**When** 战斗胜利，**Then** 消耗预留的 1 点、结算奖励与压力并完成节点。
2. **Given** 玩家从战斗撤离，**When** 战后结果提交，**Then** 不消费 Action Point，保留已破坏部位、经验和卡牌，节点保持未完成，并按既有规则结算 Fatigue 与 Wound。
3. **Given** 玩家访问 Shop，**When** 第一次成功交易，**Then** 交易和 1 Action Point 原子提交；同次访问后续成功交易不再消费行动点。
4. **Given** RunEvent 选项有明确行动点策略，**When** 选项成功提交，**Then** 所有效果与行动点一起提交；任一效果失败时全部回滚。
5. **Given** Treasure/Search 校验失败，**When** 玩家提交交互，**Then** 不消费行动点；首次成功取得结果时消费 1 点并完成节点。

---

### User Story 4 - 露营与不可逆楼层推进 (Priority: P2)

玩家在 Night 可以从当前地点或最近的合法已完成节点请求 Camp，也可以在满足卡牌条件后确认进入下一 Floor。跨层后不能返回上一层，但旧 Floor 进度仍可查看。

**Why this priority**: 它完成一层探索到下一层的长期旅程结构，同时为未来 Camp Activity 和正式地图 UI 提供稳定规则接口。

**Independent Test**: 通过纯规则 fixture 验证最近 Camp 节点、活动 token、跨层条件、首次解锁和不可返回，不要求正式页面存在。

**Acceptance Scenarios**:

1. **Given** 当前为 Night 且没有冲突事务，**When** 当前节点不允许 Camp，**Then** 系统按图距离选择同 Floor 最近的合法已完成节点，同距离使用稳定 NodeId 决胜。
2. **Given** 玩家不满足 Floor Entrance 的卡牌条件，**When** 请求跨层，**Then** 返回明确拒绝且不消费卡牌、行动点或压力。
3. **Given** 玩家任一真实持有区中拥有满足条件的卡牌，**When** 确认跨层，**Then** 入口永久解锁、卡牌不被消耗、进入目标 Floor 且不能返回旧 Floor。
4. **Given** 跨层成功，**When** 新 Floor 初始化，**Then** 已有压力、旅程天数、时段和剩余行动点保留，新 Floor 停留计数从当前日期重新计算。

---

### User Story 5 - 设计者安全制作与验证地图 (Priority: P2)

设计者可以使用稳定的 Journey、Floor、Node 和 Edge 数据定义制作手工地图，并在进入 PIE 前发现重复身份、无效连接、不可达内容、入口条件和场景绑定错误。

**Why this priority**: 地图规则必须来自可验证数据，而不是关卡 Actor 连线或世界坐标；否则后续内容扩展和存档恢复会再次依赖场景原型。

**Independent Test**: 创建有效与无效的临时地图定义并运行 Data Validation；重复运行调试资产构建流程后结果保持一致且所有 Blueprint 可编译。

**Acceptance Scenarios**:

1. **Given** Floor 中存在重复 NodeId、无效 Edge 或不可达必经节点，**When** 运行内容验证，**Then** 资产被明确标记为无效并指出对应身份。
2. **Given** 场景缺少 NodeAnchor、Edge Path 或节点内容 Host，**When** 运行场景绑定验证，**Then** 在 PIE 前报告缺失绑定。
3. **Given** 同一调试构建命令已经运行过，**When** 再次运行，**Then** 不创建重复资产、节点或场景绑定。
4. **Given** 现有 `L_Exploration` 完成迁移，**When** 进入 PIE，**Then** 玩家仍可使用已有第一人称镜头、卡牌层、战斗、事件、商店和宝藏外围完成最小探索流程。

### Edge Cases

- Journey、Floor、Node、Edge、内容资产或场景绑定为空、重复或与当前版本不匹配。
- 通道开始后场景表现无法启动，或到达终点时 ticket 已过期。
- 玩家在通道途中、内容页面、战斗、Camp 或跨层确认期间发起其它互斥操作。
- 自定义 Floor 图存在环、单向死路、多个同距离 Camp 目标或没有合法 Camp 地点。
- Action Point 为 0、成本大于剩余点数、成功交易导致时段立即推进。
- 撤离时没有有效 Trigger/Node 身份，或 Battle Result 重复提交。
- Floor Entrance 要求卡牌位于 Backpack、BattleDeck、SpecialZone 或 BurdenZone。
- 日历跨天、Camp 跳过 Sunrise、Floor Transition 发生在一天中途。
- 规则结果成功但 App 场景映射失效时，玩家不得被留在不可恢复的逻辑/画面分叉状态。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 系统 MUST 以 Journey、Floor、Map Node 和有向 Edge 表达逻辑探索地图，并使用稳定身份区分不同 Floor 的节点。
- **FR-002**: 系统 MUST 将 Map Node 生命周期限制为 `Hidden → Revealed → Visited → Resolved` 的单向推进，并为 Floor Entrance 与 Boss 输出不等同于 `Revealed` 的远景轮廓展示事实。
- **FR-003**: 系统 MUST 只在玩家到达通道终点后提交目标节点；返回通道起点 MUST 取消尚未完成的移动。
- **FR-004**: 系统 MUST 允许空闲玩家免费传送到当前 Floor 已完成节点，并禁止跨 Floor 或活动事务中的地图传送。
- **FR-005**: 系统 MUST 使用 Action Point 表达时段机会预算，不得再使用 Map Node 数量表达时间。
- **FR-006**: 系统 MUST 在行动点不足时拒绝整个内容事务且不产生部分副作用。
- **FR-007**: 系统 MUST 使用 `Morning → Day → Dusk → Night → Sunrise → Morning` 顺序和 `2/6/2/2/1` 默认预算；Morning Planning 固定占用 1 点。
- **FR-008**: 系统 MUST 在 Night 阻止普通行动，直到玩家选择 Night Exploration 或开始 Camp。
- **FR-009**: 系统 MUST 在进入新一天 Morning 时恰好计算一次 `BaseDecay + OverstayDecay`，初始 Day 1 不立即计算。
- **FR-010**: Base Decay 首版 MUST 每日为 `+5`；Overstay Decay MUST 为 FloorDay 1–3 `0`、4 `+2`、5 `+5`、6 `+9`、7+ `+12`。
- **FR-011**: 系统 MUST 以单一节点活动事务结算 Battle、RunEvent、Shop 和 Treasure 的结果、行动点、节点生命周期、奖励与压力。
- **FR-012**: Battle Victory MUST 消费预留的 1 Action Point 并完成 Encounter；Withdraw MUST 释放预留且保持节点未完成。
- **FR-013**: Withdraw MUST 保留已破坏部位、经验和卡牌奖励，不增加专属压力，并继续结算每场战斗 Fatigue 与既有 Wound 阈值事实。
- **FR-014**: Shop MUST 在本次访问第一次成功交易时消费 1 Action Point，浏览、失败、空手离开和同次后续交易不得再次消费。
- **FR-015**: RunEvent MUST 使用选项级行动点策略；普通终局默认成本为 1，非终局默认成本为 0，旧 ConsumeNode 效果不得继续存在。
- **FR-016**: Treasure/Search MUST 只在首次成功结果时消费 1 Action Point；失败校验不得消费。
- **FR-017**: Camp MUST 仅在 Night 和无冲突事务时开始，并按有向图最短距离选择当前或最近的合法已完成节点。
- **FR-018**: Camp MUST 预留 1 Action Point，完成一个 typed Camp Activity 后消费该点、放弃剩余 Night 点并跳过 Sunrise。
- **FR-019**: 系统 MUST 为 Rest、Card Upgrade、Special Event、Backpack 和 Skill Camp Activity 保留独立 typed 扩展边界，不得提供任意字符串效果或 UI 直接改卡入口。
- **FR-020**: Floor Transition MUST 验证所有持有区中的卡牌条件、不消耗卡牌、首次成功永久解锁并禁止返回旧 Floor。
- **FR-021**: Floor Transition MUST 保留已有压力、旅程天数、时段和剩余 Action Point，只重置新 Floor 的停留起点。
- **FR-022**: 静态地图和内容引用 MUST 由数据定义提供；场景 Actor 只能提供 NodeId、EdgeId、Anchor 和表现 Host 映射。
- **FR-023**: Run 规则 MUST 不读取 Actor、Spline、Widget、地图 UI 坐标或世界坐标。
- **FR-024**: UI 和场景表现 MUST 只发送意图并消费显式结果，不得直接修改 Run 状态、扣行动点或推进节点生命周期。
- **FR-025**: 初始化、移动、节点活动、Camp 和跨层 MUST 拒绝重复、过期或错误 token，并保证失败无副作用。
- **FR-026**: 现有第一人称 Cursor Look、CameraShake、ViewStage、卡牌层、Battle/Shop/RunEvent 页面和场景美术行为 MUST 在默认配置下保持不变。
- **FR-027**: 旧 NodeCount/ConsumeNode、旧 Run Tunnel 类、直接切 Spline 路径和相关 Redirect MUST 在迁移完成后零引用，不保留兼容 wrapper。
- **FR-028**: 系统 MUST 保持 Wacom 模块依赖方向，并只暴露跨模块所需的稳定数据、命令、结果和身份合同。
- **FR-029**: 系统 MUST 更新长期规则、数据制作、App 接入、架构、TODO、技术债和待确认问题文档。
- **FR-030**: 系统 MUST 提供规则、数据验证、App 接缝、资产迁移、Blueprint 编译和 PIE 的分层验证。

### Wacom-Specific Requirements

- **Docs-first evidence**: 实现以 `Docs/WacomMap.md`、`Docs/WacomRun.md`、`Docs/Architecture.md` 和 `AGENTS.md` 为规则真相，并同步 `Docs/WacomApp.md`、`Docs/WacomData*.md`、`Docs/Game_Design.md`、`Docs/TODO.md`、`Docs/TechDebt.md`、`Docs/Questions.md` 与 `CONTEXT.md`。
- **Module/API boundary**: WacomData 拥有静态图合同；WacomRun 拥有运行时状态与事务；WacomApp 拥有场景、输入与表现适配；WacomEditor 拥有制作和验证；实现 helper 保持 Private。
- **Data/GameplayTag impact**: 新增 Journey/Floor/Node/Edge 数据合同和 Validator；本轮不新增 GameplayTag。
- **Battle contract impact**: 不改变单场 Battle 规则；仅将 Battle Result 与 Run 节点活动、行动点和进度原子结算。
- **Run contract impact**: 重构 Run 初始化、探索状态、时间状态、显式命令/结果、节点活动 token 和事务回滚；不写入 SaveGame。
- **UI/App boundary**: 正式 Map/Camp UI 延后；现有外围消费规则结果，按版本幂等应用，并对生命周期、输入和场景缺失做安全恢复。
- **Testing expectation**: 新增小型 Map、Time、NodeActivity、Traversal 和 Validation specs；运行受影响 Run/UI/Battle tests、完整 Wacom 回归、Blueprint 编译和 PIE。
- **Temporary debt**: Camp Activity 具体规则、正式地图页面、Floor Modal 和地图持久化明确留给后续规格，不以临时 UI 或自动选择代替。

### Key Entities

- **Journey**: 一次旅程的静态 Floor 顺序、默认时间预算、Decay 曲线和支持角色集合。
- **Floor**: 一层独立逻辑图，拥有稳定身份、入口节点、节点集合与有向边集合。
- **Map Node**: 离散地点和内容入口，拥有稳定身份、类型、生命周期、地图位置和 typed content。
- **Map Edge**: 从一个节点通向另一个节点的有向连接；不等同于世界 Spline。
- **Map Edge Handle**: `FloorId + EdgeId` 组成的稳定身份；单独 `EdgeId` 只要求在所属 Floor 内唯一。
- **Action Point**: 当前时段可用于内容结算的机会预算，不是节点数量。
- **Floor Exposure**: 当前 Floor 的连续停留天数，用于计算 Overstay Decay。
- **Traversal Ticket**: 一次尚未到达终点的通道事务身份。
- **Node Activity Token**: Battle、RunEvent、Shop、Treasure 或 Camp 的互斥活动身份和预留成本。
- **Exploration Snapshot/Result**: App 和未来 Map UI 使用的只读状态与一次性规则结果。
- **Scene Binding**: NodeId/EdgeId 到 Anchor、Path 和内容表现 Host 的非权威映射。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 在最小调试 Floor 中，玩家可以完成分支选择、通道后退取消、终点提交和死胡同返回，连续 20 次操作均不出现逻辑节点与场景位置分叉。
- **SC-002**: 所有失败、重复、过期和行动点不足测试均证明状态版本、行动点、压力、奖励和节点生命周期零变化。
- **SC-003**: 一整天及 FloorDay 1–8 的自动化结果与确认预算和 Decay 表逐项一致。
- **SC-004**: Battle、RunEvent、Shop 和 Treasure 的成功/失败/撤离成本矩阵全部由独立自动化测试覆盖。
- **SC-005**: 有效 Journey/Floor/Scene Binding 资产验证为通过；每种结构错误至少有一条自动化或编辑器验证用例能够在 PIE 前阻断。
- **SC-006**: 默认配置下现有 Run first-person 镜头、卡牌层、Battle、RunEvent、Shop 和 Treasure 主要玩家流程通过回归与 PIE 验收。
- **SC-007**: 源码、配置和资产审计中旧 NodeCount/ConsumeNode、旧 Run Tunnel 类型和 Redirect 为零引用。
- **SC-008**: `WacomEditor` 编译、相关 focused automation、Blueprint 全量编译和完整 `Wacom` 回归不新增失败。
- **SC-009**: 所有长期规则和制作事实在功能完成时回写对应 `Docs/`，没有关键事实只存在于 feature spec。

## Assumptions

- 首轮使用单层 Debug Journey 迁移现有 `L_Exploration`，跨层和 Camp 主要通过规则自动化验证。
- Morning Planning 规则和事件信号落地，但正式 Planning UI/内容不在本轮；Night 门控不会用调试自动选择绕过。
- Camp 只交付生命周期和 typed handler Seam，自动化可以使用私有 fake handler，生产环境不提供未设计的恢复效果。
- 地图传送目标在提交规则请求前由 App 验证存在场景 Anchor。
- 调试 Journey 不参与正式内容平衡，也不决定未来多角色旅程可用性。
- 当前 SaveGame 保持关闭；新地图状态不写盘，旧存档兼容不约束本轮规则结构。

## Refinement: Run Path route choice and Anchored look (2026-07-15)

- 单一合法出口不再依赖 BranchTarget；玩家首次按 W 时 App 自动选择唯一 Edge，并通过原有 traversal ticket 开始移动。
- 多个合法出口才进入显式选择状态。A/D 或左摇杆按道路世界方向切换，E / 手柄 A 或鼠标点击确认，确认后立即开始移动。
- 结构死胡同与暂时锁定出口必须区分提示；本 refinement 不实现 Map Screen，但保留免费返回已完成节点的既定扩展入口。
- Anchored 持续更新 cursor look / view transform，且只消费一次性导航意图；只有 Traversing 修改 Spline distance、Walk CameraShake 或移动反馈。
- 首次进入 Anchored 不依赖 viewport 点击；RunPath 独占基础 Yaw，cursor look 只叠加到 ControlRotation。开始 Traversal 时从 NodeAnchor View 平滑对齐 PathSpline，不允许入口朝向差异造成首帧瞬跳或逐帧朝向争夺。
- BranchTarget 是静态多出口 Decision Gate 的 App 表现绑定，只保存 EdgeId，并以 Hidden / Available / Focused 只读状态驱动可重建发光入口。
