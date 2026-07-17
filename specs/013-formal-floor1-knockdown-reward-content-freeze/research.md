# Research: 正式 Floor 1 击倒分支奖励卡内容冻结

## Decision 1: 以 Spec 012 作为唯一运行时前置

**Decision**: 只冻结未来 DataAsset 内容；不再修改分支奖励字段、resolver、ViewData、UI 或 validator。

**Rationale**: 基线 `10815349` 已提供 `AidRewardCard`/`DestroyRewardCard`、legacy fallback、原子授予、被动预览和 FormalProduction 门禁，并通过 45 项聚焦 Automation。

**Alternatives considered**: 为 Floor 1 新建奖励表或节点规则；拒绝，因为会绕过统一 PartDefinition 查询并重复状态。

## Decision 2: 使用现有 Card schema 的简单主动效果

**Decision**: 每张卡只包含 1–2 个现有效果；Physique、PerfectRelease、ZoneHook、Passive 为空，不增加 GameplayTag。

**Rationale**: live authoring matrix 已支持 `Effect.Damage`、`Effect.ApplyStatus.Poison`、`Effect.ApplyStatus.Slow`、`Status.Shield` 与所需 Targets。简单主动效果便于下一轮直接生成和校验。

**Alternatives considered**: 新被动、按破坏部位数缩放、随机目标、伤口或 RunEvent；拒绝，因为需要新 runtime 合同并超出内容冻结。

## Decision 3: 按敌人 Intent 拟态并保持清晰构筑身份

**Decision**: 效果取自来源敌人的招式组合；Aid 统一 `Tool`，Destroy 统一 `Weapon`。Brush 为 White，Molt/Root 为 Blue，Guardian 为 Yellow。

**Rationale**: 拟态让奖励可追溯至敌人体验，Tool/Weapon 保留构筑分类，稀有度反映 Floor 1 敌人梯度。

**Alternatives considered**: 全部无关键词、逐卡随机分类或 Aid/Destroy 使用完全通用模板；分别会削弱构筑身份、增加记忆成本或失去敌人主题。

## Decision 4: 冻结八张卡的数值与效果顺序

**Decision**:

- Brush Aid：Shield 2 后 Slow 1；Destroy：Damage 3 后 Poison 1。
- Molt Aid：Shield 7；Destroy：Damage 6。
- Root Aid：Shield 3 后 Slow 2；Destroy：Damage 5 后 Poison 1。
- Guardian Aid：Shield 10；Destroy：Cost 2、AllEnemyParts Damage 4 后 Poison 1。

**Rationale**: 数值分别映射 Hide+Coil、Bite+Venom、Harden+Brace、Snap/Slam、RootGuard+Tangle、Lunge+Sap、Harden+CrownGuard 与 Sweep+Venom。效果顺序同时固定描述占位与未来 builder 写入。

**Alternatives considered**: Guardian 单体高伤或控制；拒绝，用户已批准 2 费 Yellow 群攻终局牌。

## Decision 5: 每敌人一对 Definition、每部位一次选择

**Decision**: 十一 Part 复用所属敌人的卡对；每个部位处理击倒时获得所选分支的一个独立 Card Instance，允许重复，不新增去重或上限。

**Rationale**: 这是当前 runtime 合同，用户明确接受完整探索最多 20 张的体量。复用四对 Definition 避免每部位膨胀到 22 张卡。

**Alternatives considered**: 同敌去重、只有首张免费、奖励替代物；拒绝，因为需要新的 Battle/Run state 和结算规则。

## Decision 6: 将奖励量作为显式 Production 风险

**Decision**: 关键路线固定记录 A/C=14、B/C=15、A/D=16、B/D=17，完整探索=20；AP 不变。

**Rationale**: 必经 Scout、Elite、Guardian 共 11 个 Part，A/B 分别增加 3/4，D 增加 2，C 不增加 Encounter。该体量会影响未来卡组和背包，但不是本轮规则修改理由。

**Alternatives considered**: 忽略重复数量或只记录全探索 20；拒绝，因为会隐藏路线间真实差异。

## Decision 7: 追加 manifest，不改写 Spec 011 历史证据

**Decision**: Spec 011 的 38 core package 保持原样；Spec 013 提供八张卡的增量 manifest，后续 Production 总写集合为 46。

**Rationale**: Spec 011 是已集成历史合同。增量合同能清楚区分核心内容冻结与分支奖励冻结，同时给未来 builder 完整写集合。

**Alternatives considered**: 回写修改 Spec 011 manifest；拒绝，避免改写已交付工件和模糊基线。

## Decision 8: 本轮只做静态验证

**Decision**: 不编译、不运行 Automation、AssetRegistry、Builder、Blueprint 或 PIE。

**Rationale**: 变更仅为 Markdown 和 Spec Kit 指针；未来 package 尚不存在，运行 Unreal 不能证明这些设计已实现。静态计数、schema、引用和 Git 范围审计覆盖本轮风险。

**Alternatives considered**: 重跑 Spec 012 Automation；拒绝，因为 base 集成已完成 45/45，且本轮不改变其代码或资产。
