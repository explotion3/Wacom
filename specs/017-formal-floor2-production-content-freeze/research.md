# Research: 正式 Floor 2 Production 内容合同冻结

## R1. 复用现有 schema，不扩展运行时

**Decision**: 四个 Enemy、Behavior、Part、Intent，七个 Encounter，十二张 Card、四个 Pickup、三个 Event 和一个 Shop 全部使用当前 DataAsset 字段与现有 GameplayTag/FName 合同。

**Rationale**: Damage、Poison、Slow、Shield、Draw、Heal，现有 target、rarity、keyword、RunEvent condition/effect 与 Credential 已覆盖批准设计。纯内容冻结不应反向引入字段或模块依赖。

**Rejected**: 为 MoltCavern 新增专用 selector、phase、状态、事件效果或 GameplayTag；它们会扩大为规则开发且没有内容需求。

## R2. 每部位独立 Sequence，Shield target 明确区分

**Decision**: 所有 Behavior 只有 `Default` phase；每个 Part 使用一个 `Sequence` IntentSet，`CooldownSelections=0`、无 selector rule/fallback。Damage/Poison/Slow 指向 Player，Shield 指向行动 Part 自身。

**Rationale**: 这与现有敌人制作范式和批准表一致。显式写全 target 可避免把敌方 Shield 与卡牌 Shield（Player）混为一谈。

**Rejected**: 用同一共享 IntentSet 覆盖多个 Part，或仅写自然语言“自身”；会破坏稳定 PartSlot/IntentSet 身份和生成可验证性。

## R3. Floor 2 战斗梯度

**Decision**: Encounter HP 按节点职责冻结为 21、36、42、36、34、57、70；最多两个敌人。长线梯度表述为 `21 → 34–42 → 36 → 57 → 70`，其中 BridgeSentinel 复用 36 HP StoneScaleGuard。

**Rationale**: Floor 2 比 Floor 1 提高部位数和血量，保留单/双敌人制作预算；同一敌人可在不同节点承担不同叙事职责，无需复制资产。

**Rejected**: 为 BridgeSentinel 复制一份敌人 Definition；这会制造无规则差异的身份分叉。

## R4. 每敌人一对击倒奖励

**Decision**: 每个 Archetype 冻结一张 Aid 和一张 Destroy 卡，所属 Part 全部显式引用同一对卡，legacy 字段为空。每次已处理部位获得所选分支的一张独立实例，允许重复。

**Rationale**: 复用 Spec 012 的统一查询、原子授予和 FormalProduction profile，也与 Floor 1 的稳定制作粒度一致。

**Rejected**: 每 Part 一对卡、每 Enemy Instance 限领、奖励去重或改成货币；这些都会增加资产量或运行时状态。

## R5. 固定 Pickup 卡与蜕印双身份

**Decision**: 三个普通 Treasure 各授予一张固定卡；MoltSeal 同时授予 `Card.Run.MoltSeal` 和 `Credential.Run.MoltSeal`，Floor 3 入口只检查 Credential。

**Rationale**: 延续已落地的通用任务凭证合同，避免可移除表现卡造成软锁。

**Rejected**: 只授予卡、从背包反推 Credential 或消耗 Credential；均与长期 Run 合同冲突。

## R6. 事件信息/经济双路

**Decision**: CastoffEcho 和 LostDelver 分别提供情报、取金与恢复选择；MoltingRite 消费两个 flag 之一或 Gold，也提供强闯兜底。所有 Choice terminal、Automatic、完成并关闭，无 CardPayment。

**Rationale**: A/B 两路都能在 0 Gold 起点获得至少 3 Gold；选择情报则主动放弃即时购买力，换取 D 路更优结算。现有 FName RunFlag 与 pressure effect 足以表达。

**Rejected**: 新建任务系统、持久化事件 flag、卡牌支付或多节点 Event；超出 Floor 2 内容冻结。

## R7. 固定 Shop 与跨层资源累计

**Decision**: DeepWayfarer 使用固定五 Offer 和 `3/3/4/4/5` 价格；前两路的取金选择至少支持一个 3 Gold Offer。Gold、卡组与压力继续跨 Floor 累计。

**Rationale**: 固定库存可直接用当前 Shop schema，复用 Floor 1 三张正式卡并引入两个 Floor 2 Offer，不需要随机池或价格公式。

**Rejected**: 复制 Floor 1 卡、动态库存、Floor 入场资源重置；会制造重复身份或新规则。

## R8. AP 与奖励产量

**Decision**: Floor 2 保持 `8–9 / 14–15 AP`，唯一区间来自首次成功购买。必经 11 Part；A/C、B/C、A/D、B/D 分别为 17、18、17、18，完整探索 24。

**Rationale**: 击倒选择仍属于 Encounter 内事务，不增加 AP。产量直接由现有图和 Enemy Part 数推导，是后续背包/平衡 PIE 的风险输入。

**Rejected**: 为降低奖励量增加去重、领取上限或 AP；这些需要独立规则设计。

## R9. 47-package manifest 与未来制作边界

**Decision**: 路径沿用主题 namespace，分支卡位于 Archetype 子目录；本轮只冻结精确 manifest。后续资产轮必须独立建立受控 seeder、writer allowlist、真实加载、Data Validation、AssetRegistry、引用/哈希、幂等与 LFS 证据。

**Rationale**: 稳定 package 是安全资产制作的前置，但文档不能替代二进制验证。

**Rejected**: 本轮顺手创建空壳 DataAsset，或复制 Floor 1 seeder 改字符串；都会绕过用户审阅和 Unreal writer workflow。

## R10. 已知 BugGirl 污染隔离

**Decision**: `DA_Character_BugGirl` StarterDeck 污染保持外部问题。本轮不引用、不修改、不提交该资产，现有 Production dependency validator 保持不变。

**Rationale**: 用户已经明确接受该越界问题，且 Character 不属于 Floor 2 47-package manifest。

**Rejected**: 修改 Character、降低 dependency gate、将失败记为通过或用其它 worktree 版本覆盖。
