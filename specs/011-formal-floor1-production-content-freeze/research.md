# Research: 正式 Floor 1 Production 内容合同冻结

## Decision 1 — 只冻结内容，不创建 DataAsset

**Decision**: 本轮交付 38 个未来 DataAsset 的字段、ID、路径、依赖和验证合同；不启动 Unreal、不运行 builder、不创建或保存二进制包。

**Rationale**: 用户批准的是内容设计冻结。运行时 schema、Credential、三层图与 Journey success 已在 main 中稳定，而 38 个资产仍需要独立生成、校验、引用审计和场景适配；把这些工作混入文档轮会扩大资产权威风险。

**Alternatives considered**:

- 同轮生成 38 个资产：需要 C++ builder、Data Validation、二进制审计与编译/Automation，超出纯文档范围。
- 手工先建空壳资产：会把不完整内容伪装成 Production readiness。
- 继续只保留槽 ID：无法解除 Floor 1 内容设计 blocker，也不能给后续制作提供稳定字段合同。

## Decision 2 — 现有 schema 足够表达全部内容

**Decision**: 不新增 DataAsset 字段、enum 或 GameplayTag。卡牌使用现有 `Heal / Slow / Shield / Draw`，敌人意图使用 `Damage / Poison / Slow / Twilight / Shield`，事件使用 `MinGold / RunFlagSet / AddGold / AddPressure / SetRunFlag`。

**Rationale**: live headers、`FWacomBattleRuleContentContract` 与 `Docs/WacomDataAuthoring.md` 已覆盖全部组合。RunEvent `AddPressure` 接受负值并由现有事务/Clamp 处理恢复。

**Alternatives considered**:

- 用自然语言描述效果：无法被后续 builder 与 validator 精确实现。
- 新增剧情脚本 callback：违反静态 DataAsset 与领域 resolver 边界。
- 用 GameplayTag 表达事件 flag：当前 RunFlag 是 FName set；新增 tag 没有必要。

## Decision 3 — 一个敌人原型对应一个 Behavior 资产

**Decision**: BrushSnake、MoltGuard、RootStalker、ShallowGuardian 各自拥有一个 Behavior，`InitialPhaseId=Default`；每个 PartSlot 在同一 phase 中匹配唯一 `Sequence` IntentSet。

**Rationale**: 这沿用 Snake/TrainingWarrior 的制作范式，同时让部位行为集中、槽位映射可校验、Encounter 能安全复用 EnemyDefinition。

**Alternatives considered**:

- 每个部位一个 Behavior：资产数量膨胀且重复 phase 配置。
- 四敌共用一个巨型 Behavior：槽位/意图命名冲突，后续单敌调整困难。
- 把行为写进 EnemyPart：当前 schema 明确不支持。

## Decision 4 — 冻结项目级可读 ID

**Decision**: EnemyId 使用 `Enemy.SerpentWood.<Archetype>`；BehaviorId 使用 `SerpentWood.<Archetype>.Behavior`；PartId 使用 `SerpentWood.<Archetype>.<Part>`；IntentSet/Intent ID 继续带主题、原型和部位前缀。

**Rationale**: EnemyId 保持用户批准值；其余 ID 沿用现有 `<Enemy>.Behavior`、`<Enemy>.<Part>` 范式并加入主题命名空间，避免跨包重名和诊断歧义。

**Alternatives considered**:

- 只用 `Head/Body`：跨敌人无法唯一诊断。
- 使用资产文件名作为 ID：重命名会污染静态身份。
- 纯 GUID：不可读，不利于 validator 与日志。

## Decision 5 — 新部位奖励保持 null，但不是最终规则

**Decision**: 十一个新 EnemyPart 的 `KnockdownRewardCard=null`。`Aid/Destroy/Withdraw` 的正式分支效果、奖励差异与节点联动继续是独立 P0。

**Rationale**: 当前字段把 Aid/Destroy 指向同一张奖励卡，无法表达用户尚未确认的三分支差异。临时填同卡会把错误口径固化进正式 Floor 内容。

**Alternatives considered**:

- 所有部位发同一奖励：污染正式内容且掩盖 P0。
- 复用 PoisonFang：把旧蛇样例奖励误升为 SerpentWood 规则。
- 本轮扩展击倒 schema：超出内容冻结范围。

## Decision 6 — 四张新卡只使用当前卡牌合同

**Decision**: 三张 Reward 卡和 SerpentSigil 只配置单一结构化效果、零 Physique、无被动/ZoneHook/PerfectRelease；前三张使用 `Card.Keyword.Tool`，SerpentSigil 不新增关键词。

**Rationale**: 这些卡服务 Floor 1 清晰奖励职责，不引入新的战斗机制。`TargetMode=None` 适用于 Player/Draw 自身流程，HunterSnare 使用 `SingleEnemyPart`。

**Alternatives considered**:

- 为蛇印新增任务关键词：入口资格已经由 Credential 处理，新增 tag 重复真相。
- 给蛇印不可销毁稀有度：会改变用户批准的 White 配置且混淆表现卡/资格。
- 增加插画/被动：本轮不冻结表现或新机制。

## Decision 7 — 复用现有正式 PoisonFang，不改 ID

**Decision**: Wayfarer 的毒牙 Offer 精确引用 `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang`，其 live `CardId=PoisonFang`；不创建 `Reward.SerpentWood.PoisonFang`。

**Rationale**: 用户明确要求“现有正式 PoisonFang”。live builder/tests 已把 `PoisonFang` 冻结为当前正式 ID；重命名或复制会扩大资产和兼容范围。

**Alternatives considered**:

- 新建主题毒牙：形成两张语义重复的卡。
- 文档写成 `Reward.PoisonFang`：与 live ID 不一致。
- 重命名现有卡：属于资产迁移与运行时引用变更，不在本轮授权。

## Decision 8 — Pickup 固定卡牌主奖励，蛇印同事务授予 Credential

**Decision**: 四个 Pickup 均使用 `RewardType=Card`。只有 SerpentSigil 的 `GrantedCredentialIds` 包含 `Credential.Run.SerpentSigil`；其余为空。

**Rationale**: 当前 Pickup schema 只支持固定 Gold 或固定一张 Card 主奖励，可同时授予 Credential。该设计完全复用已落地的软锁解决方案。

**Alternatives considered**:

- 用 RunEvent 发卡：改变节点类型和 AP 事务。
- 只发 Credential：丢失已冻结的蛇印表现卡。
- 从持有卡反推 Credential：已被现有 Run 合同禁止。

## Decision 9 — 四个事件都是单节点 terminal 图

**Decision**: 每个 Event 使用 `StartNodeId=Start`、一个节点、无 NextNode；全部 13 个 Choice 均 `ActionPointPolicy=Automatic`、`bMarkEventCompleted=true`、`bCloseEventAfterResolve=true`。

**Rationale**: 用户要求每个成功选择固定 1 AP；现有 Automatic terminal 合同正好提供该语义，同时保留 RunEvent working-state 回滚。

**Alternatives considered**:

- 用 Fixed=1：可行但重复表达默认 terminal 规则。
- 非 terminal 多节点事件：增加未批准的文本/分支与 AP 语义。
- 把扣费写成 effect：当前 schema 已删除任意 AP effect。

## Decision 10 — 路线经济提供购买机会而非保证购买

**Decision**: Route A 的 SellSkin 给 2 Gold，Route B 的 LootPack 给 3 Gold；Wayfarer 有四个 2 Gold Offer 和一个 3 Gold Offer。选择情报/道德选项可主动放弃即时购买力。

**Rationale**: 从初始 0 Gold 出发，两路都存在至少一次成功购买路径；但路线选择仍有取舍，不把 Shop 购买写成强制或免费。

**Alternatives considered**:

- 所有选项都给 Gold：消除道德/情报取舍。
- Shop 最低价 1：削弱两路 Gold 奖励差异。
- 保证玩家无论选项都能购买：需要额外基础金币规则，超出本轮。

## Decision 11 — Future builder 采用严格写集合

**Decision**: 后续 SerpentWood 内容生成入口只能写 manifest 中 38 个新 package；三张 Starter 卡和 PoisonFang 是只读依赖。不得写 Map、Debug/Authoring、TrainingWarrior、UI、材质或其它 Agent 资产。

**Rationale**: 明确写集合能防止全量内容重建覆盖人工调参，并与项目现有专用 enemy-pack/debug builder 边界一致。

**Alternatives considered**:

- 直接扩大 `WacomRegenerateContent` 全量重建：可能覆盖无关人工内容，且缺少独立资产审计边界。
- 手工制作不提供 builder 合同：难以保证 38 个资产可重复生成与测试。
- 复用 Debug builder：其写集合和职责与 Production 内容完全不同。

## Decision 12 — 本轮验证只覆盖文档合同

**Decision**: 运行 Spec Kit 分析、静态计数/唯一性/schema/economy/AP/禁止引用审计、`git diff --check`、Git/LFS/range audit；不运行编译、Automation、AssetRegistry、Builder、Blueprint 或 PIE。

**Rationale**: 没有 runtime 或 asset bytes 变化。启动 Unreal 不能证明尚未创建的内容资产，反而增加编辑器写入风险。

**Alternatives considered**:

- 跑现有 Data tests：只能验证旧资产，不验证本轮 Markdown 表。
- 跑 Debug builder：会写无关二进制夹具。
- 做 PIE：Production Floor/Definition 尚不存在，没有可验收路径。
