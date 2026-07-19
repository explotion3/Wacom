# Feature Specification: 正式 Floor 2 Production 内容合同冻结

**Feature Branch**: `codex/formal-floor2-production-content-freeze`

**Created**: 2026-07-19

**Status**: Approved for documentation implementation; pending user review before commit

**Input**: 在既有 Floor 2 图、蜕印 Credential 与 Floor 1 Production 内容范式上，以纯文档方式冻结 `MoltCavern` 的 47 个未来 DataAsset、战斗梯度、事件经济、击倒奖励与制作门禁；不创建资产或修改运行时。

## Wacom Rule Context

**Primary Domain**: Data-card authoring / Battle content / Run-exploration / Map content

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`
- [x] Spec 009 的 Floor 2 图与节奏、Spec 011/013/014 的 Floor 1 内容与制作范式

**Expected Owning Module(s)**: 本轮不修改模块。未来静态内容归 `WacomData`，现有战斗与探索合同分别由 `WacomBattle`、`WacomRun` 解释，受控播种与制作校验归 `WacomEditor`，验证归 `WacomTests`。

**Non-Goals / Boundaries**:

- 不修改 C++、Config、GameplayTag、Build.cs、模块依赖、Snapshot、Command、Resolution、ResultPacket、SaveGame 或公共 schema。
- 不创建、修改或保存 DataAsset、Blueprint、地图、材质、纹理、`.uasset` 或 `.umap`；不运行任何 builder。
- 不创建 Floor 2 FloorDefinition、world、Host、Production Journey 或跨层 world handoff。
- 不设计 Floor 3 内容，不调整 Floor 1 内容，不把 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior 或 Character 资产作为 Production 依赖。
- 不改变 AP、Shop 首购、击倒选择、重复奖励、背包溢出、Credential、压力、RunFlag 生命周期或战后持久化规则。
- 不修改已知受外部工作影响的 `DA_Character_BugGirl`，不削弱 Production dependency validator，也不把其 closure 误报为通过。

**Open Rule Questions**: 无。本轮敌人、卡牌、Encounter、Event、Shop、奖励产量与路径均已在批准计划中冻结。背包容量与其它非卡牌击倒后果继续保留为独立问题。

## User Scenarios & Testing

### User Story 1 - 使用完整的 MoltCavern 战斗内容合同 (Priority: P1)

作为后续内容作者，我需要四个敌人原型、十二部位、四个行为和七个 Encounter 拥有精确身份、数值、意图顺序与编组，使 Floor 2 资产制作不再依赖临时判断。

**Why this priority**: 战斗图是七个 Encounter Definition、十二份 Part 引用与击倒奖励的共同依赖，也是 47-package manifest 的最大依赖组。

**Independent Test**: 静态审计得到 4 Enemy、12 Part、4 Behavior、7 Encounter，所有 ID/package 唯一；Encounter HP 精确为 21、36、42、36、34、57、70，单场最多两个敌人。

**Acceptance Scenarios**:

1. **Given** 任一 MoltCavern Archetype，**When** 制作人员查阅合同，**Then** 可找到 Default phase、同原型 Behavior、按序 Part、每 Part Sequence IntentSet、完整 Intent 及 HP/EXP。
2. **Given** 任一 Intent，**When** 对照当前 authoring schema，**Then** 只使用 Damage、Poison、Slow、Shield 与 Player/acting-Part self target，`CooldownSelections=0` 且无 selector rule/fallback。
3. **Given** 七个 Encounter，**When** 检查 authored slot order，**Then** 顺序、Enemy 引用、总 HP 与最多两个敌人的边界全部确定；`bBoss=true` 仍只属于 Floor 节点 payload。

---

### User Story 2 - 使用完整的奖励、事件与经济合同 (Priority: P2)

作为 Run 内容作者，我需要四张固定 Pickup 卡、八张 Aid/Destroy 卡、四个 Pickup、三个 Event 与一个 Shop 拥有精确字段，使两条前半路线从 0 Gold 出发都有购买路径，并让情报选择服务 D 路事件。

**Why this priority**: 这些 Definition 决定 Floor 2 的路线经济、蜕印资格、奖励体量和完整 47-package 数量。

**Independent Test**: 静态审计得到 12 Card、4 Pickup、3 Event/10 Choice、1 Shop；所有 Effect、Target、Rarity、Keyword、condition、pressure 与 credential 均属于当前 schema。

**Acceptance Scenarios**:

1. **Given** 玩家沿 Route A 或 B 从 0 Gold 开始，**When** 选择对应取金选项并抵达 DeepWayfarer，**Then** 至少可购买一个 3 Gold Offer；选择情报则可满足 MoltingRite 的对应条件。
2. **Given** 玩家取得 MoltSeal Pickup，**When** 未来运行时解析现有 Pickup 合同，**Then** 同一事务预留 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal`，Floor 3 入口只依赖非消耗 Credential。
3. **Given** 任一正式 Part，**When** 配置击倒奖励，**Then** Aid/Destroy 显式引用所属 Archetype 的卡对，deprecated `KnockdownRewardCard` 为空。
4. **Given** 相同 Archetype 的多个部位或 Encounter 实例，**When** 选择 Aid/Destroy，**Then** 每个已处理部位产生一张所选卡的独立实例，允许重复且不增加 AP。

---

### User Story 3 - 交接可播种的 47-package Production manifest (Priority: P3)

作为后续实现和集成人员，我需要一个精确、无 Debug 依赖的 47-package manifest、路线产量模型与 Production readiness gate，使下一轮能按小组受控创建资产而不会把文档冻结误当成交付完成。

**Why this priority**: 稳定 package、类型、引用和数量是未来 seeder、writer allowlist、AssetRegistry 与幂等验证的输入。

**Independent Test**: manifest 精确为 `7 Encounter + 3 Event + 4 Pickup + 1 Shop + 4 Enemy + 4 Behavior + 12 Part + 12 Card = 47`，无重复或禁止引用；路线奖励为 17/18/17/18，完整探索 24。

**Acceptance Scenarios**:

1. **Given** 本轮文档完成，**When** 检查 Git diff，**Then** 仅包含 Markdown、`.specify/feature.json` 与 `AGENTS.md` 托管指针，不包含 Source、Config 或 Content。
2. **Given** 后续资产轮，**When** 生成 writer allowlist，**Then** 它能从 manifest 精确列出 47 个完整 `/Game/...` package，并把 Floor 1 Shop 引用卡作为只读依赖而非新增资产。
3. **Given** 47 个资产尚未创建，**When** 评估 Floor 2 Production readiness，**Then** Data Validation、AssetRegistry、引用闭包、哈希、幂等、场景与 PIE 全部保持未完成，不以静态合同替代真实验证。

### Edge Cases

- Shield Intent 的 target 是行动部位自身；卡牌 Shield 的 target 是 Player，不能因同名 Effect 混淆。
- Slow Intent 使用现有 `Default / TargetCardCount=1` 手牌投递；本轮不新增 hand-selection mode。
- `BridgeSentinel` 与 `StoneScaleGuard` 都引用同一敌人并同为 36 HP，这是批准的不同节点内容复用，不创建重复 Enemy。
- `VenomShard`、Destroy 卡的 Damage 与 Poison 顺序冻结；前一效果破坏目标后，后一效果按现有 effect-chain 语义处理。
- 每个 Part 只获得所选 Aid 或 Destroy 一张卡；Withdraw 不取卡，最后存活部位可用性保持现有规则。
- `RunFlag` 是现有 FName 内存态，不是 GameplayTag，也不承诺 SaveGame 恢复。
- Shop 浏览/离开为 0 AP；本次访问首次成功购买为 1 AP。Floor 2 `8–9 / 14–15 AP` 的唯一区间来自该行为。
- `DA_Character_BugGirl` 的已知 StarterDeck 污染不在本 manifest 引用闭包内；本轮不修复、不覆盖、不清理，也不放宽 validator。

## Requirements

### Functional Requirements

- **FR-001**: 必须冻结恰好 47 个未来 DataAsset package，分类为 `4 Enemy / 4 Behavior / 12 Part / 7 Encounter / 3 Event / 4 Pickup / 1 Shop / 12 Card`。
- **FR-002**: 四个 Enemy 必须使用 `Default` phase、同原型 Behavior、按序 Part 引用和每 Part 独立 `Sequence` IntentSet；BehaviorOverride 为空。
- **FR-003**: Behavior、Part、IntentSet、Intent ID 必须分别使用 `MoltCavern.<Archetype>.Behavior`、`MoltCavern.<Archetype>.<Part>`、`...Sequence` 与 `...<Intent>`。
- **FR-004**: 全部 Intent 必须使用 `CooldownSelections=0`、空 selector rule/fallback；Damage/Poison/Slow 指向 Player，Shield 指向行动部位自身，Slow 为 `Default / 1 card`。
- **FR-005**: Part HP/EXP 与 Intent D/I/R/状态/护盾数值必须精确匹配批准表；Enemy 总 HP/EXP 分别为 ScaleCrawler `21/2`、StoneScaleGuard `36/4`、VenomHunter `34/5`、CavernGuardian `70/10`。
- **FR-006**: 七个 Encounter 的 authored slot order 与总 HP 必须精确为 `21, 36, 42, 36, 34, 57, 70`，单场最多两个敌人；`bBoss` 不进入 Encounter。
- **FR-007**: 必须冻结四张固定 Pickup Card 与八张 Aid/Destroy Card 的 CardId、名称、费用、稀有度、关键词、TargetMode、有序 Effects 与 package。
- **FR-008**: 十二张卡必须全部零 Physique，且不使用 Swift、Exhaust、PerfectRelease、ZoneHook 或 Passive；四张 Aid 使用 Tool，四张 Destroy 使用 Weapon。
- **FR-009**: 十二份 Part 必须显式引用所属 Archetype 的 Aid/Destroy 卡对，deprecated `KnockdownRewardCard` 全部为空。
- **FR-010**: 四个 Pickup 必须使用固定 Card mapping；`Pickup.MoltCavern.MoltSeal` 必须同时预留 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal`。
- **FR-011**: 三个 Event 必须都是 `Start` 单节点、terminal、Automatic、完成并关闭、无 CardPayment，共恰好 10 个 Choice；条件、Effect 顺序和 FName flag/pressure identity 必须匹配冻结表。
- **FR-012**: `Shop.MoltCavern.DeepWayfarer` 必须固定五个 Offer 和顺序，价格为 `3/3/4/4/5`；不新增随机池、动态价格或重复 Offer。
- **FR-013**: Route A/B 必须各自从 0 Gold 存在至少一次购买路径；情报选项必须分别可满足 `MoltingRite` 的两条 flag 条件。
- **FR-014**: 关键路线击倒奖励必须为 A/C=17、B/C=18、A/D=17、B/D=18，完整探索=24；奖励选择不增加 AP。
- **FR-015**: Floor 2 AP 必须保持最短 `8–9`、完整 `14–15`，唯一区间来自 Shop 首次成功购买。
- **FR-016**: 47 个 package、稳定 ID 与受控引用必须唯一；Production manifest 不得引用 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior 或 Character 资产。
- **FR-017**: 必须同步 `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomBattle.md`、`WacomMap.md`、`TODO.md`、`Questions.md` 与 `Roadmap.md`，关闭 Floor 2 内容设计 blocker并新增 47 资产制作任务。
- **FR-018**: 本轮只能修改 Markdown、`AGENTS.md` 托管指针和 `.specify/feature.json`；不得修改源码、Config、GameplayTag、Build.cs、DataAsset、地图或其它二进制资产。
- **FR-019**: 必须执行 Spec Kit 跨工件、manifest 数量/唯一性、schema、路线、禁止引用、Markdown、Git/LFS/range 静态审计，并记录全部 Unreal 验证跳过原因。
- **FR-020**: 文档完成后必须先交用户审阅；未经明确确认不得 stage 或 commit。

### Wacom-Specific Requirements

- **Docs-first evidence**: 以 live `Docs/`、Spec 009 和当前 schema 为权威；Spec 017 的最终内容事实必须回写长期文档。
- **Module/API boundary**: 不新增公共 API；未来资产只实例化现有 `WacomData` schema，由既有 Battle/Run 合同解释。
- **Data/GameplayTag impact**: 本轮零 DataAsset、schema 和 GameplayTag 变更，只冻结未来字段值与 package manifest。
- **Battle contract impact**: 不改变击倒事务、Intent resolver、ResultPacket、经验或战内卡语义。
- **Run contract impact**: 不改变 Pickup/Shop/Event、Credential、AP、背包、压力、RunFlag 或 SaveGame 合同。
- **UI/App boundary**: 无 UI、Actor、Host、输入、焦点、镜头或场景变更。
- **Testing expectation**: 纯文档静态审计；不编译、不运行 Automation、AssetRegistry、Builder、Blueprint 或 PIE，并明确记录原因。
- **Temporary debt**: 无新增临时代码。背包奖励膨胀与其它击倒后果继续留在 `Docs/Questions.md`；47 资产、Floor/map/Host 与跨层场景是显式后续任务。

### Key Entities

- **MoltCavern enemy graph**: 4 Enemy、4 Behavior、12 Part、每 Part Sequence IntentSet 与 26 个有序 Intent。
- **Floor 2 node definitions**: 7 Encounter、3 RunEvent、4 Pickup 与 1 Shop，对应 Spec 009 的 15 个内容节点。
- **Floor 2 card set**: 4 张固定 Pickup/Run 卡与 8 张 Archetype Aid/Destroy 分支卡。
- **Route memory/economy**: 两个 FName RunFlag、A/B Gold 选择、D 路事件条件与固定 Shop inventory。
- **Production manifest**: 47 个未来 package 的唯一 locator、class、stable ID 与受控依赖。
- **Known external issue**: 不属于本 manifest 的 `DA_Character_BugGirl` StarterDeck 污染，继续由现有 validator 暴露。

## Success Criteria

### Measurable Outcomes

- **SC-001**: manifest 精确包含 47 个唯一 package，类型计数为 `4/4/12/7/3/4/1/12`，无重复 stable ID。
- **SC-002**: 四敌人共有 12 Part、4 Behavior 与 26 Intent；所有 ID、顺序、HP/EXP、D/I/R 与 effect target 与冻结合同一致。
- **SC-003**: 七 Encounter 总 HP 精确为 `21/36/42/36/34/57/70`，战斗梯度可表述为 `21 → 34–42 → 36 → 57 → 70`，单场敌人数不超过 2。
- **SC-004**: 12 Card、4 Pickup、3 Event/10 Choice 与 1 Shop 全部可由当前 schema 表达，新增 tag、enum、字段和运行时规则数量为 0。
- **SC-005**: 12/12 Part 显式配置同敌卡对且 legacy 为空；路线奖励精确为 `17/18/17/18/24`，Floor AP 仍为 `8–9 / 14–15`。
- **SC-006**: Production manifest 对 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior 和 Character 的正式引用数量为 0。
- **SC-007**: Spec、Plan、Research、Data Model、Contracts、Tasks、Quickstart、Checklists 与长期 Docs 对 ID、数值、路径、数量、路线和阻塞无 Critical/High/Medium 冲突。
- **SC-008**: Git diff 只包含允许的文本与 Spec Kit 指针文件，没有 Source、Config、Build.cs、`.uasset` 或 `.umap`。
- **SC-009**: 用户审阅前提交数为 0；确认后使用独立提交 `docs(content): freeze floor2 production content`，不 merge main、不 push。

## Assumptions

- 当前 Card、Enemy、Behavior、Part、Encounter、Pickup、Shop 和 RunEvent schema 足以表达全部冻结内容。
- `Status.Shield` 在敌方 Intent 中指向行动部位自身，在卡牌中指向 Player；文档按完整 target 明确区分。
- `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal` 是 FName 内容身份，不是 GameplayTag；Credential 按现有 SaveGame v5 合同跨 Floor 保留。
- Gold、卡组与压力沿 Journey 跨 Floor 累计；本轮不调整 Floor 1 结余或增加 Floor 2 入场重置。
- DisplayName、Description、美术、音效、Host、世界 Transform 和后续平衡修订不属于稳定身份。
- 纯文档改动不需要 Unreal 编译或运行时验证；后续 47 资产轮必须补受控 seeder、Data Validation、Automation、AssetRegistry、引用/哈希、幂等和 LFS，场景轮再补 PIE。
