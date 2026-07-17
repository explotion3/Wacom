# Feature Specification: 正式 Floor 1 击倒分支奖励卡内容冻结

**Feature Branch**: `codex/formal-floor1-knockdown-reward-content-freeze`

**Created**: 2026-07-18

**Status**: Approved for documentation implementation

**Input**: 在不修改源码或资产的前提下，冻结四个 SerpentWood 敌人的八张 Aid/Destroy 击倒奖励卡、十一部位引用映射、路线奖励量与后续 Production 制作门禁。

## Wacom Rule Context

**Primary Domain**: Data-card authoring / Battle content / Run-exploration

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `Docs/Roadmap.md`
- [x] Spec 011 Production manifest/readiness 与 Spec 012 分支奖励合同

**Expected Owning Module(s)**: 本轮只冻结设计事实，不修改模块。未来八张卡与十一份 PartDefinition 属于 `WacomData` 静态内容，受控生成与校验属于 `WacomEditor`，现有击倒事务继续由 `WacomBattle` 解释，验证属于 `WacomTests`。

**Non-Goals / Boundaries**:

- 不修改 C++、Config、Build.cs、GameplayTag、SaveGame、Snapshot、Command、Resolution、ResultPacket 或模块依赖。
- 不创建、修改或保存 DataAsset、Blueprint、材质、纹理、`.uasset` 或 `.umap`，不运行任何 builder。
- 不改变 Aid/Destroy/Withdraw 可用性、击倒结算、手牌上限、Victory/Withdraw/Defeat 持久化、AP 或撤离重入规则。
- 不设计额外左/右手 buff、永久破坏、RunFlag、压力、战后 RunEvent、奖励替代物或去重状态。
- 不冻结插画、音效、CardView 表现、Host、世界 Transform、Floor 2/3 奖励卡或正式场景。
- 不解决背包容量溢出；继续使用现有 Run 获得卡与负重区兜底。

**Open Rule Questions**:

- 无。本轮卡牌字段、重复允许、每部位一次、Aid=Tool、Destroy=Weapon、稀有度梯度和 Guardian 群攻均已由用户确认或按批准默认冻结。

## User Scenarios & Testing

### User Story 1 - 使用精确的八张分支奖励卡合同 (Priority: P1)

作为卡牌内容作者，我需要每个 SerpentWood 敌人拥有一张 Aid 与一张 Destroy 奖励卡的完整字段，使后续资产制作不再需要决定费用、稀有度、关键词、目标、效果顺序或文案。

**Why this priority**: 八张卡是 Spec 011 的 46 资产 Production 制作包仍未就绪的唯一内容设计阻塞。

**Independent Test**: 静态审计恰好得到 8 个唯一 CardId、8 个唯一 package、4 Aid/4 Destroy；全部字段映射到当前 Card schema 和 GameplayTag 集合。

**Acceptance Scenarios**:

1. **Given** 任一 SerpentWood Archetype，**When** 内容作者查阅 manifest，**Then** 可获得唯一 Aid/Destroy CardId、名称、描述、费用、稀有度、关键词、TargetMode、按序 Effects 与 package leaf。
2. **Given** 任一冻结卡，**When** 对照当前 authoring matrix，**Then** 它只使用现有 Damage、Poison、Slow、Shield、Player、SingleEnemyPart、AllEnemyParts、Tool、Weapon 与 White/Blue/Yellow 合同。
3. **Given** ShallowGuardian.Destroy，**When** 查阅终局奖励合同，**Then** 它固定为 2 费 Yellow Weapon，对所有存活敌方部位依次造成 4 Damage、施加 1 Poison。

---

### User Story 2 - 按敌人复用奖励并审计路线产量 (Priority: P2)

作为战斗与 Run 内容设计人员，我需要十一份正式 Part 都显式引用所属敌人的同一对卡，并清楚知道关键路线和完整探索的奖励卡数量，避免误建每部位独立卡或低估卡组膨胀。

**Why this priority**: 当前规则每个已处理部位都会授予所选分支卡；引用粒度和产量会直接影响未来资产装配、背包容量与平衡验证。

**Independent Test**: 静态映射覆盖 `2 + 3 + 2 + 4 = 11` 个 Part；路线演算得到 A/C=14、B/C=15、A/D=16、B/D=17、完整探索=20。

**Acceptance Scenarios**:

1. **Given** 任一正式 Part，**When** 按 Production 合同配置奖励，**Then** `AidRewardCard` 与 `DestroyRewardCard` 指向所属 Archetype 的卡对，deprecated `KnockdownRewardCard` 为空。
2. **Given** 同一敌人的多个部位或同 Archetype 的多个 Encounter 槽，**When** 分别完成击倒选择，**Then** 每个部位可获得一个独立 Card Instance，允许重复，不添加去重或上限。
3. **Given** Floor 1 四种合法关键路线，**When** 汇总必经与支路 Encounter 的部位数，**Then** 奖励量分别为 14、15、16、17；完成所有 Encounter 时为 20。
4. **Given** 奖励选择，**When** 计算 Floor 1 行动点，**Then** 选择本身不额外消耗 AP，Floor 1 保持 `8–9 / 14–15 AP`。

---

### User Story 3 - 交接可实施的 46 资产 Production 包 (Priority: P3)

作为后续实现与集成人员，我需要 Spec 011 的 38 核心 manifest 明确追加八张卡，并保留资产、引用、builder 与运行时验证门禁，使下一轮可以一次性制作 46 个正式 DataAsset 而不越界。

**Why this priority**: 内容设计冻结只有进入受控 manifest、长期 Docs 和 readiness gate 后，才能安全转化为二进制制作任务。

**Independent Test**: 文档审计确认 `38 core + 8 branch reward = 46`，八张卡均处于四个 Archetype 子目录，Production 表零 Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior 引用。

**Acceptance Scenarios**:

1. **Given** Spec 013 提交，**When** 集成人员检查 changed files，**Then** 仅有 Markdown、`AGENTS.md` 托管指针与 `.specify/feature.json`，没有源码、Config 或二进制资产。
2. **Given** 下一轮受控 builder，**When** 创建 Floor 1 内容，**Then** 写集合恰好为 Spec 011 的 38 个 package 加本 Spec 的 8 个 package，现有 Starter/PoisonFang 与其它资产保持只读。
3. **Given** 46 个资产尚未创建，**When** 评估 Production Floor 1 readiness，**Then** DataAsset、AssetRegistry、引用/哈希、场景和 PIE 继续保持阻塞，不以文档冻结冒充运行时验证。

### Edge Cases

- 每次击倒只能选择 Aid、Destroy 或可用时的 Withdraw；同一部位不会同时获得 Aid 与 Destroy 两张卡。
- 最后存活部位不可 Withdraw；选择 Aid/Destroy 后卡仍记录进战后包，即使战斗立即结束或战内手牌上限把 runtime card 移入弃牌堆。
- 多个相同 Archetype 实例继续产生独立 Card Instance；CardId 重复是批准行为，package 和 Definition 不重复创建。
- Aid 卡可以同时影响 Player 与显式选中的敌方部位；`TargetMode=SingleEnemyPart` 负责要求该目标，各 Effect 使用自己的 Target。
- Damage 后目标已经被破坏时，后续 Poison 按现有 Effect Chain 处理；本轮不发明死亡目标补偿。
- 描述中的 `{Effect.0}`、`{Effect.1}` 必须与冻结 Effects 顺序一致。
- 无正式资产时不得运行 builder、AssetRegistry 或 PIE 来声称内容已交付。

## Requirements

### Functional Requirements

- **FR-001**: 必须冻结恰好 8 张奖励卡，命名为 `Reward.SerpentWood.<Archetype>.Aid/Destroy`，且 CardId 与 package path 分别唯一。
- **FR-002**: 每张卡必须冻结 DisplayName、Description 模板、BaseCost、Rarity、Keywords、TargetMode 与有序 Effects。
- **FR-003**: Aid 卡必须使用 `Card.Keyword.Tool`，Destroy 卡必须使用 `Card.Keyword.Weapon`；不得使用 Swift、Exhaust 或其它关键词。
- **FR-004**: BrushSnake 两张卡必须为 White；MoltGuard/RootStalker 为 Blue；ShallowGuardian 为 Yellow。
- **FR-005**: 除 ShallowGuardian.Destroy 为 Cost 2 外，其余七张卡必须为 Cost 1。
- **FR-006**: 八张卡必须只使用当前已实现的 Damage、Poison、Slow、Shield Effect 与 Player/SingleEnemyPart/AllEnemyParts Target。
- **FR-007**: 八张卡的 Physique、PerfectReleaseEffects、ZoneHooks 与 Passives 必须为空；插画、音效和 CardView 表现保持未冻结。
- **FR-008**: 卡牌效果必须按来源敌人的现有 Intent 拟态，并使用批准数值：Brush Aid `Shield2+Slow1`、Destroy `Damage3+Poison1`；Molt Aid `Shield7`、Destroy `Damage6`；Root Aid `Shield3+Slow2`、Destroy `Damage5+Poison1`；Guardian Aid `Shield10`、Destroy `All Damage4+Poison1`。
- **FR-009**: 八张 package 必须位于 `/Game/Wacom/Data/Cards/Rewards/SerpentWood/<Archetype>/DA_Card_<Archetype>_<Choice>`。
- **FR-010**: 十一份正式 Part 必须按所属 Archetype 显式填写 Aid/Destroy 引用并清空 deprecated `KnockdownRewardCard`。
- **FR-011**: 奖励粒度必须保持每个敌人一对 Definition、每个部位选择后获得一个 Card Instance；允许重复且不得新增去重、领取上限或替代奖励。
- **FR-012**: 路线奖励量必须记录为 A/C 14、B/C 15、A/D 16、B/D 17、完整探索 20。
- **FR-013**: 击倒奖励选择不得新增 AP；Floor 1 继续保持关键推进 `8–9`、完整探索 `14–15 AP`。
- **FR-014**: Production 总量必须扩展为 38 core 加 8 branch reward，共 46 个未来 DataAsset。
- **FR-015**: Production 内容表不得引用 Debug、Authoring、`Test.*`、BadgeDisplayTests、TrainingWarrior 或 legacy Snake 内容。
- **FR-016**: 必须同步 `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomBattle.md`、`WacomMap.md`、`TODO.md`、`Questions.md` 与 `Roadmap.md`。
- **FR-017**: 本轮只能修改 Markdown、`AGENTS.md` 托管指针和 `.specify/feature.json`；不得修改源码、Config、GameplayTag、Build.cs、DataAsset 或地图。
- **FR-018**: 必须记录纯文档轮跳过编译、Automation、AssetRegistry、Builder、Blueprint 与 PIE 的理由、零运行时回归面和未来卡组膨胀风险。

### Wacom-Specific Requirements

- **Docs-first evidence**: 以 Spec 011/012 和 live `Docs/` 为权威，并将最终卡牌、引用、路线产量与 readiness 事实回写长期文档。
- **Module/API boundary**: 不新增或修改公共 API。未来资产仍由 `WacomData` 保存，`WacomBattle` 使用 Spec 012 的统一查询解释，`WacomEditor` 负责受控生成/验证。
- **Data/GameplayTag impact**: 本轮无 DataAsset、schema 或 GameplayTag 改动；只冻结未来字段值和 package manifest。
- **Battle contract impact**: 不改变击倒 Command/View/Resolution/Event/ResultPacket；只为现有分支配置正式内容。
- **Run contract impact**: 不改变获得卡、背包、AP、PersistentId 或 SaveGame；重复卡仍为独立实例。
- **UI/App boundary**: 不修改 UI；现有 Dialog 继续被动显示 Spec 012 ViewData。
- **Testing expectation**: Spec Kit 跨工件分析、表格计数、ID/package/Part/路线演算、schema/禁止引用、Markdown link、`git diff --check` 与 Git/LFS/range 审计。
- **Temporary debt**: 无新增技术债。背包容量与其它击倒分支效果继续在 `Docs/Questions.md` 跟踪。

### Key Entities

- **Branch reward card contract**: 一张未来 `UCardDefinition` 的稳定身份、可制作字段和 package locator。
- **Archetype reward pair**: 一个敌人原型共用的一张 Aid 与一张 Destroy Definition。
- **Part reward assignment**: 十一份 `UEnemyPartDefinition` 到所属 Archetype 卡对的显式引用。
- **Route reward yield**: 按 Encounter 部位数推导的关键路线与完整探索获得卡数量。
- **Production manifest extension**: Spec 011 的 38 core 加本 Spec 八张 CardDefinition 的 46 资产集合。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 内容表恰好包含 8 张卡、4 Aid、4 Destroy、8 个唯一 CardId 和 8 个唯一 package。
- **SC-002**: 七张卡为 Cost 1，一张 Guardian.Destroy 为 Cost 2；稀有度分布为 `2 White / 4 Blue / 2 Yellow`。
- **SC-003**: 关键词分布为 `4 Tool / 4 Weapon`，Swift、Exhaust、Passive、ZoneHook、PerfectRelease 与非零 Physique 数量均为 0。
- **SC-004**: 十一份 Part 全部映射到所属敌人的卡对，legacy 引用数量为 0。
- **SC-005**: 路线演算精确产生 `14 / 15 / 16 / 17 / 20` 五个奖励量，AP 仍为 `8–9 / 14–15`。
- **SC-006**: 全部 Effect、Target、Rarity 与 Keyword 都存在于当前 authoring matrix；新增 tag、enum、字段和运行时规则数量为 0。
- **SC-007**: Production manifest/引用表对 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior 和 legacy Snake 的正式引用数量为 0。
- **SC-008**: Spec、Plan、Data Model、Contracts、Tasks、Quickstart、Checklists 与长期 Docs 对 ID、数值、路径、映射、数量和阻塞无冲突。
- **SC-009**: Git diff 只包含允许的文本和 Spec Kit 指针文件，没有 Source、Config、Build.cs、`.uasset` 或 `.umap`。
- **SC-010**: worktree/LFS 在独立提交后干净，未 merge main、未 push。

## Assumptions

- 当前 `UCardDefinition`、GameplayTag 与 Effect semantics 足以表达八张卡，不需要 schema 扩展。
- Aid=Tool、Destroy=Weapon 使用批准默认；卡牌具体效果仍按敌人 Intent 拟态，而不是套用一组通用治疗/攻击模板。
- 相同 CardId 的重复实例是批准的 Floor 1 奖励经济；平衡与背包容量在资产实现和 PIE 轮验证，不在本轮修改规则。
- 描述冻结为可替换效果占位文本；本轮不冻结插画、音效、材质或正式 WBP 表现。
- 纯文档变更不需要 Unreal 编译或运行时测试；后续 46 资产实现轮必须补 builder、Data Validation、Automation、AssetRegistry、引用/哈希与 PIE。
