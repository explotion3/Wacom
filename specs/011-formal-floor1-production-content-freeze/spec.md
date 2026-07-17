# Feature Specification: 正式 Floor 1 Production 内容合同冻结

**Feature Branch**: `codex/formal-floor1-production-content-freeze`

**Created**: 2026-07-17

**Status**: Approved for documentation implementation

**Input**: 在不修改源码或资产的前提下，冻结 `Floor.Main.01` 的 15 个 Production 节点 Definition，以及支撑它们的 SerpentWood 敌人、行为、部位、卡牌、事件、拾取与商店合同。

## Wacom Rule Context

**Primary Domain**: Data-card authoring / Run-exploration / Battle content / Testing

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`

**Expected Owning Module(s)**: 本轮只冻结设计事实，不修改模块。未来静态 Definition 属于 `WacomData`，制作与校验属于 `WacomEditor`，运行时解释继续使用现有 `WacomBattle / WacomRun`，测试属于 `WacomTests`。

**Non-Goals / Boundaries**:

- 不修改 C++、Build.cs、GameplayTag、SaveGame schema、Snapshot、Command、Resolution 或模块依赖。
- 不创建、修改或保存任何 DataAsset、Blueprint、材质、纹理、`.uasset` 或 `.umap`。
- 不运行 builder，不把 TrainingWarrior、Debug、Authoring、`Test.*` 或 BadgeDisplayTests 内容作为 SerpentWood Production 依赖。
- 不冻结美术、Host Blueprint、世界 Transform、Encounter 敌人世界阵型或 SceneEnemyHost 映射。
- 不冻结击倒 `Aid / Destroy / Withdraw` 的三分支正式效果；全部新部位保持 `KnockdownRewardCard=null`，等待独立 P0 决策。
- 不创建正式 Journey/Floor DataAsset 或场景；38 个未来资产必须在后续实现轮生成、校验和审计。
- 不修改 Floor 1 的 20 Node/21 Edge 图、AP 规则、Camp、Credential、入口或 Journey 成功合同。

**Open Rule Questions**:

- 击倒 `Aid / Destroy / Withdraw` 的正式分支效果、奖励差异与节点联动继续由现有 P0 问题阻塞；本功能不把空奖励误记为最终规则。
- Floor 1 Production 世界资产权威继续待 AssetRegistry、引用与哈希审计后确认。

## User Scenarios & Testing

### User Story 1 - 使用可制作的 SerpentWood 战斗内容合同 (Priority: P1)

作为后续内容制作人员，我需要四个敌人原型、十一份部位、四份行为与六个 Encounter 的精确静态合同，使 Floor 1 的战斗梯度能够在现有 Enemy/Behavior/Encounter schema 中被一致制作。

**Why this priority**: 六个 Encounter 是 Floor 1 主要难度与 AP 节奏的骨架，也是 Host、场景敌人和后续 Golden Path 的核心前置。

**Independent Test**: 静态审计确认 4 Enemy、11 EnemyPart、4 Behavior 和 6 Encounter ID/路径唯一；所有意图都使用现有 `Sequence`、Effect、Target 与数值字段；Encounter 总 HP 为 `16 → 26–32 → 44 → 52`，单场最多两个敌人。

**Acceptance Scenarios**:

1. **Given** 内容人员查阅任一 SerpentWood 敌人，**When** 按合同制作其 Enemy、Behavior 与 Part 资产，**Then** 每个 PartSlot 都能在 `Default` phase 中匹配唯一 `Sequence` IntentSet，且所有 Intent 可由现有 resolver 执行。
2. **Given** 六个冻结 Encounter，**When** 按 EnemySlots 汇总部位 HP，**Then** Scout 为 16、MoltGuard 为 28、Ambush 为 32、RootStalker 为 26、EliteSentinel 为 44、ShallowGuardian 为 52。
3. **Given** Guardian Encounter，**When** 内容人员查阅 Boss 语义，**Then** `bBoss=true` 只存在于 Floor node payload，不被复制为 Encounter 字段或敌人特例。
4. **Given** 击倒三分支仍未定稿，**When** 制作任一新部位，**Then** `KnockdownRewardCard` 保持 null，且文档明确这是 P0 阻塞而非正式空奖励。

---

### User Story 2 - 使用一致的奖励、拾取与商店经济合同 (Priority: P2)

作为卡牌与 Run 内容制作人员，我需要四张新卡、四个 Pickup 和一个五商品 Shop 的明确映射，使前半两条路线都存在从 0 金币获得一次 2 Gold 购买力的选择，并保持 Floor 1 原有 AP 区间。

**Why this priority**: 奖励、蛇印 Credential 和商店库存共同决定路线价值、任务门槛与 AP 的唯一 0/1 差值。

**Independent Test**: 静态核对 4 个新 CardId、4 个 PickupId、1 个 ShopId 与五个 Offer；两条前半路线分别提供 `+2` 与 `+3` Gold 选择，Shop 最低价格为 2，蛇印 Pickup 同时授予表现卡和 Credential。

**Acceptance Scenarios**:

1. **Given** 玩家选择 Route A 的 `SellSkin`，**When** 之后抵达 Wayfarer，**Then** 从初始 0 Gold 出发可购买一个 2 Gold Offer。
2. **Given** 玩家选择 Route B 的 `LootPack`，**When** 之后抵达 Wayfarer，**Then** 从初始 0 Gold 出发可购买一个 2 或 3 Gold Offer。
3. **Given** 玩家选择情报而非金币，**When** 抵达 Shop，**Then** 文档将缺少即时购买力视为玩家的路线取舍，不伪造额外金币来源。
4. **Given** 玩家完成 `Pickup.SerpentWood.SerpentSigil`，**When** 后续实体卡被移除，**Then**入口资格仍由同次 Pickup 授予的 `Credential.Run.SerpentSigil` 维持；本轮不修改既有 Credential 运行时合同。
5. **Given** 商店库存引用既有正式毒牙，**When** 制作 Offer，**Then** 精确复用 `CardId=PoisonFang` 与 `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang`，不创建第二个 SerpentWood 毒牙资产。

---

### User Story 3 - 使用现有 RunEvent schema 制作四个路线事件 (Priority: P3)

作为事件内容制作人员，我需要四个单节点 terminal RunEvent 的完整 Choice、Condition、Effect 与 RunFlag 合同，使路线金币、劣迹、疲劳和情报标记都能由现有事务解释器原子执行。

**Why this priority**: 事件把前半路线选择传递到商店与毒沼，同时必须避免新增脚本回调、数值 flag 或未持久化能力承诺。

**Independent Test**: 静态核对四个 Event、十三个 Choice；全部 Choice 都是 terminal、标记完成、关闭事件并使用 `Automatic`，只使用现有 MinGold、RunFlagSet、AddGold、AddPressure、SetRunFlag 合同。

**Acceptance Scenarios**:

1. **Given** `StudyPattern` 或 `ReadTrail` 被选择，**When** 事件事务成功，**Then** 设置对应 FName RunFlag 且固定消耗 1 AP。
2. **Given** `TradeMoltClue` 或 `FollowMarkedRoute`，**When** 所需 RunFlag 未设置，**Then** 选项不可用且不提交任何部分效果。
3. **Given** `BuyMap` 或 `BurnOffering`，**When** Gold 低于门槛，**Then** 选项不可用；满足门槛时扣费与其它效果在同一现有 RunEvent 事务中提交。
4. **Given** 任一事件选项成功结束，**When** 计算 Floor 1 AP，**Then** 仍按普通 terminal 事件记 1 AP，Floor 1 保持 `8–9 / 14–15 AP`。

---

### User Story 4 - 在 Production readiness gate 下交接 38 个未来资产 (Priority: P4)

作为后续实现与集成人员，我需要一份精确的 38 资产 manifest、路径规范和禁止引用清单，使资产制作可以独立开始，但不会误认为本轮已经交付二进制内容或正式世界场景。

**Why this priority**: 文档冻结必须转化为可审计的后续工作包，同时继续保护 Authoring、Debug 和 Production 权威边界。

**Independent Test**: manifest 恰好包含 15 个节点 Definition、4 Card、4 Enemy、4 Behavior 和 11 EnemyPart；所有目标路径位于批准的主题目录，Production 表不引用 Debug/Authoring/Test/BadgeDisplayTests。

**Acceptance Scenarios**:

1. **Given** 本轮提交，**When** 集成人员检查 changed files，**Then** 只看到 Markdown 与 Spec Kit 指针元数据，没有源码、Config、Build.cs 或二进制资产。
2. **Given** 后续资产制作，**When** 选择参考实现，**Then** TrainingWarrior 只作为当前制作范式参考，不进入 SerpentWood 的正式引用闭包。
3. **Given** 38 个资产尚未创建，**When** 评估 Production Journey/Floor/map readiness，**Then** 仍保持阻塞，直到资产实现、Data Validation、AssetRegistry/引用/哈希审计和正式场景制作分别完成。

### Edge Cases

- 任何 EnemyId、PartId、BehaviorId、IntentSetId、IntentId、EncounterId、CardId、PickupId、ShopId、EventId、NodeId 或 ChoiceId 在其作用域为空或重复时，冻结合同无效。
- `PoisonFang` 是现有正式 CardId，不得为了命名整齐重命名、复制或计入四张新卡。
- 敌人 Slow 作用玩家时必须使用现有 `HandAffliction` 默认/单目标语义；Twilight 作用玩家时使用现有整手牌语义，不复用 Duration 表示目标数。
- 非伤害 Intent 的 `ResistanceValue` 默认为 0；伤害 Intent 使用冻结表中的 R 值。护盾、状态与伤害不得塞进同一未记录的复合 Intent。
- `AddGold` 负值依赖对应 `MinGold` 条件；RunEvent 事务仍负责失败回滚与 Gold 下限，不在内容文档发明新扣费规则。
- `RunFlag` 是本次 Run 的 FName set 且当前不写入 SaveGame；本轮不把路线情报描述为跨读档持久事实。
- MapPosition、世界 Transform、Host Blueprint、美术与 Encounter 场景阵型不属于本合同。
- Production 引用审计必须针对 manifest/内容表，不能因禁止条款自身出现 `Debug/Test/Authoring` 字样产生误报。

## Requirements

### Functional Requirements

- **FR-001**: 必须冻结 4 个 Enemy、11 个 EnemyPart、4 个 EnemyBehavior 的 ID、未来资产路径、部位 HP/EXP 与完整循环 `Sequence` IntentSet。
- **FR-002**: 必须冻结 6 个 Encounter 的 EnemySlotId、Enemy 引用、顺序与总 HP；单个 Encounter 最多两个敌人。
- **FR-003**: 必须冻结 4 张新卡的 CardId、显示名、费用、稀有度、Tool 关键词、TargetMode 和单一效果；`Card.Run.SerpentSigil` 不新增关键词。
- **FR-004**: 必须冻结 4 个 Pickup 的固定 Card 主奖励映射；蛇印 Pickup 额外授予 `Credential.Run.SerpentSigil`。
- **FR-005**: 必须冻结 `Shop.SerpentWood.Wayfarer` 的五个固定 Offer 与价格，并精确复用既有正式 `PoisonFang`。
- **FR-006**: 必须冻结 4 个 RunEvent 的 EventId、ChoiceId、条件、效果、RunFlag 与 terminal 语义；所有 Choice 使用 `Automatic`、完成并关闭事件。
- **FR-007**: 所有新部位必须保持 `KnockdownRewardCard=null`，并把击倒三分支奖励记录为未解决 P0，不得写成正式空奖励规则。
- **FR-008**: 必须保持 Encounter HP 梯度为 `16 → 26–32 → 44 → 52`。
- **FR-009**: Route A/B 必须各自存在从初始 0 Gold 达到至少一个 2 Gold Offer 的选择；选择情报可主动放弃即时购买力。
- **FR-010**: 必须保持 Floor 1 内容 AP 为最短 `8–9`、完整 `14–15`；区间只来自 Shop 首次成功购买。
- **FR-011**: 必须冻结恰好 38 个未来 DataAsset 的 manifest：15 节点 Definition、4 Card、4 Enemy、4 Behavior、11 EnemyPart。
- **FR-012**: 必须使用 `/Game/Wacom/Data/{Enemies,Encounters,Events,Pickups,Shops,Cards}/.../SerpentWood/` 主题路径合同；既有 Starter 与 PoisonFang 只作为明确的正式外部引用。
- **FR-013**: Production 内容表不得引用 Debug、Authoring、`Test.*`、BadgeDisplayTests 或 TrainingWarrior 资产。
- **FR-014**: 必须只使用当前已实现的 Effect、Target、Rarity、Keyword、RunEvent Condition/Effect、Pressure ID 和 DataAsset schema。
- **FR-015**: 必须把冻结事实同步到 `Docs/WacomData.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomMap.md`、`Docs/TODO.md` 与 `Docs/Questions.md`。
- **FR-016**: 必须关闭 Floor 1 内容设计 blocker，并新增 38 DataAsset 实现/校验任务；Production DataAsset/关卡制作仍保持独立阻塞。
- **FR-017**: 本轮只能修改 Markdown、`AGENTS.md` 托管指针和 `.specify/feature.json`，不得修改源码或二进制资产。
- **FR-018**: 必须记录纯文档轮跳过编译、Automation、AssetRegistry、Builder、Blueprint 和 PIE 的原因、零运行时回归风险及后续资产轮风险。

### Wacom-Specific Requirements

- **Docs-first evidence**: 更新 `Docs/WacomData.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomMap.md`、`Docs/TODO.md`、`Docs/Questions.md`；`Docs/Architecture.md` 无新架构事实。
- **Module/API boundary**: 不新增或修改公共 API。未来资产仍由 `WacomData` 保存静态事实，现有 `WacomBattle/WacomRun` 解释，`WacomEditor` 生成/校验。
- **Data/GameplayTag impact**: 本轮无 DataAsset、schema 或 GameplayTag 变更；只冻结未来资产字段与路径。
- **Battle contract impact**: 不修改 Battle Command/Snapshot/Event/Result 或 resolver；全部意图和卡牌效果必须落在当前 authoring matrix。
- **Run contract impact**: 不修改 `URunSession`、`FRunState`、RunEvent 事务、Credential、AP、PersistentId 或 SaveGame。
- **UI/App boundary**: 无 UI、Actor、Input、Host 或场景变更。
- **Testing expectation**: Spec Kit 跨工件分析、manifest/计数/ID/路径/schema/经济/AP 静态审计、`rg` 禁止引用审计、`git diff --check` 与 Git/LFS/range 审计；Unreal 验证按设计跳过。
- **Temporary debt**: 无新增技术债；现有击倒 P0 与 Production 资产权威问题继续在 `Docs/Questions.md` 跟踪。

### Key Entities

- **SerpentWood enemy archetype**: 一个 Enemy、一个 Behavior、若干部位与每部位唯一 `Sequence` IntentSet 的静态制作单元。
- **Production node Definition**: Floor 1 15 个内容节点各自引用的 6 Encounter、4 RunEvent、4 Pickup、1 Shop。
- **Production reward card**: 三张 `Reward.SerpentWood.*` 与一张 `Card.Run.SerpentSigil`，均使用当前 Card schema。
- **Route memory**: `SerpentWood.MoltTrailKnown` 与 `SerpentWood.MarshRouteKnown` 两个本 Run FName flags。
- **Asset manifest**: 38 个未来 DataAsset 的唯一 ID、资产名、目录、类型与依赖。
- **Existing formal dependency**: 三张 Starter 卡与 `PoisonFang`，被 Shop 明确复用但不计入 38 个新资产。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 15 个节点 Definition 的分类恰好为 `6 Encounter / 4 RunEvent / 4 Pickup / 1 Shop`。
- **SC-002**: manifest 恰好包含 4 Enemy、11 EnemyPart、4 Behavior、4 新 Card，所有 ID 与目标资产路径在各自作用域唯一。
- **SC-003**: 六个 Encounter 总 HP 精确为 16、28、32、26、44、52，单场敌人数不超过 2。
- **SC-004**: 所有 Card/Intent/RunEvent 字段值均能映射到当前 live schema 与 authoring matrix，新增 tag、enum 或字段数量为 0。
- **SC-005**: Route A 的 `SellSkin` 提供 2 Gold、Route B 的 `LootPack` 提供 3 Gold；Wayfarer 至少有四个 2 Gold Offer，因此两路均存在一次购买路径。
- **SC-006**: Floor 1 AP 仍为 `8–9 / 14–15`，事件全部按 1 AP，唯一范围差值来自 Shop 首次成功购买。
- **SC-007**: Production manifest/合同表对 Debug、Authoring、`Test.*`、BadgeDisplayTests 与 TrainingWarrior 的正式引用数量为 0。
- **SC-008**: Spec、Plan、Data Model、Contracts、Tasks、Quickstart、Checklist 与长期 Docs 对数量、ID、数值、路径、阻塞和非目标无冲突。
- **SC-009**: Git diff 只包含允许的文本与 Spec Kit 指针文件，没有 C++、Build.cs、Config、Content、`.uasset` 或 `.umap`。

## Assumptions

- `Docs/WacomDataAuthoring.md` 的 current battle authoring matrix 与 live headers 是本轮 schema 权威；Spec 007 中“具体内容未冻结”的旧阶段事实由本 Spec 和更新后的长期 Docs取代。
- 既有正式毒牙使用 `CardId=PoisonFang`；不改名、不复制，也不计入本轮四张新卡或 38 个新资产。
- 卡牌目标模式按现有 schema冻结：HerbalPoultice/MoltWard 为 `None`，HunterSnare 为 `SingleEnemyPart`，SerpentSigil 为 `None`；效果 Target 分别使用 `Player / SingleEnemyPart / Player / Player`。
- 敌人状态/护盾 Intent 的 `ResistanceValue=0`；伤害 Intent 使用冻结的 R 值。Slow 的玩家 `HandAffliction.TargetCardCount=1`，Twilight 使用当前 all-current-hand 默认语义。
- 所有 RunEvent 仅有一个起始节点 `Start`；Choice 不跳转下一节点，靠完成与关闭终结事件。
- 纯文档变更不需要 Unreal 编译、Automation、AssetRegistry、Blueprint 或 PIE；后续 38 资产实现轮必须补 Data Validation、生成资产 smoke、引用审计与场景 Golden Path。
