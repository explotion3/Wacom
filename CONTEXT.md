# Wacom Domain Context

本文件固定 Wacom 规则实现中的核心领域词汇，帮助代码、文档和测试使用同一套名称。

## Language

**PlayCard Transaction（出牌事务）**:
消费 Prepared PlayCard 后，从 `CardPlayed` 到卡牌效果、被动、中毒、敌方行动和战斗结束判断的一次完整同步规则结算。
_Avoid_: 出牌预览流程、UI 出牌流程

**PlayCard Evaluation（出牌求值）**:
读取当前 BattleState，为候选出牌生成规范化执行绑定、运行时费用和当前可提交性的只读规则事实。
_Avoid_: 出牌预检、UI 目标校验

**Target Probe（目标探测）**:
针对一个显式目标对象、不校验阶段与费用的严格 PlayCard 目标兼容性求值。
_Avoid_: 预览焦点校验、完整出牌校验

**Preview Focus（预览焦点）**:
Target Preview 中用于单目标表现摘要、但不构成 PlayCard 执行绑定的当前指向对象。
_Avoid_: 执行目标、命令目标

**Prepared PlayCard（已准备出牌）**:
通过完整 PlayCard Evaluation、携带状态版本与规范化执行事实、可交给 PlayCard Transaction 的一次性只读准备结果。
_Avoid_: 已提交出牌、可缓存命令

**Action Preview（行动预览）**:
在复制的战斗状态上执行出牌事务的确定性部分后，向表现层返回的只读预测事实。
_Avoid_: 模拟结算、UI 伤害计算

**Unresolved Fact（未决事实）**:
行动预览因随机选择或未知效果而明确不展开的后续结果。
_Avoid_: 预览失败、预测误差

**RunEvent Choice Evaluation（RunEvent 选项求值）**:
读取当前 RunState，按定义顺序生成一个 RunEvent 选项的条件、卡牌支付需求事实和当前可提交性。
_Avoid_: UI 可用性判断、后果模拟

**Battle Card Zone Transition（战斗卡牌区域迁移）**:
一批真实卡牌区域移动及其 Location、事件和被动后果的一次同步有序规则操作；它不承诺嵌套效果失败时回滚。
_Avoid_: 先移动再通知、弃牌事件服务、卡牌移动 Helper

**Turn Lifecycle（回合生命周期）**:
从首回合启动或当前玩家回合结束开始，按唯一顺序推进 TurnStart、TurnEnd、手牌清理、敌方行动、BattleEnd gate、下一回合与表现 checkpoint 的同步规则范围。
_Avoid_: EndTurn Resolver 流程、回合事件拼装、UI 回合状态机

**Effect Semantics（效果语义）**:
一类 Effect 从资产字段到制作合法性、typed 参数、目标计划、Preview determinism、handler 和 Target Preview 投影的唯一代码定义。
_Avoid_: Effect 配置行、Dispatcher 特判、Preview 专用效果规则

**Effect Chain（效果链）**:
按定义顺序执行一个或多个 Effect segment、并在明确词法生命周期内共享 `LastShuffledCard` scratch 的同步规则范围。
_Avoid_: Effect 数组循环、全局效果上下文、跨触发器 scratch

**Combatant Mutation（战斗单位变更）**:
对玩家或敌方部位应用一次伤害、治疗、护盾或 stack status 意图，并统一维护 HP、Shield、阈值、状态层数、事件和部位破坏边沿的同步规则操作。
_Avoid_: Handler 直接扣血、Poison 专用死亡逻辑、状态标签与层数双写

**Status Semantics（状态语义）**:
按状态和宿主解释施加、物化、消费与触发时机，并把原始变更委托给 Combatant Mutation、Card Runtime State 或 Initiative Timeline 的唯一代码定义。
_Avoid_: 通用 timed-status 表、Effect Handler 状态特判、把所有状态都存进 Combatant StatusStacks

**Initiative Timeline（先机时间线）**:
安装、延迟或推进敌方部位 `CurrentInitiative`，并产生逐部位实际变化事实的唯一运行时写入口；部位破坏的原子归零除外。
_Avoid_: Resolver 直接写 CurrentInitiative、只发全局推进摘要

**Card Runtime State（卡牌运行时状态）**:
单卡状态层数、战内费用修正、有效费用和出牌限制的唯一运行时真相与求值入口。
_Avoid_: Preview 单独算费用、用 RuntimeCostModifier 混存状态、把卡牌状态挂到玩家本体

**Pending Hand Affliction（待生效手牌控制）**:
敌方意图施加给玩家、在下个玩家回合抽牌并重建 Hand 后才选择具体卡牌的 Slow、Freeze 或 Twilight 事实。
_Avoid_: 玩家 StatusStacks 与卡牌 StatusStacks 双写、在抽牌前提前选卡

**Damage Facts（伤害事实）**:
一次 Combatant Mutation 解析出的请求伤害、护盾吸收、实际 HP 损失、overkill 和本次破坏结果；`DamageDealt.Amount` 投影实际 HP 损失。
_Avoid_: 名义伤害等于扣血、用 Combat Log 反推护盾吸收

**Battle Initialization Result（战斗初始化结果）**:
一次新战斗初始化 commit 的显式原子结果，绑定 status、开场 events 和 post snapshot；失败时保留 Session 当前旧战斗且不发布事件。
_Avoid_: 初始化事件队列、Session 开场事件拉取

**Local Settings（本地设置）**:
由 `UWacomGameUserSettings` 持久化、只属于当前设备的显示、图形、音频、视角响应和表现辅助配置；编辑通过 `UWacomSettingsSubsystem` 的 token 化 Preview / Apply / Cancel 事务完成，不进入玩家档案、活动旅程或滚动备份。
_Avoid_: 玩家设置存档、旅程设置、Settings Screen 状态

## Relationships

- **Target Probe** 和 Target Preview 使用同一份 **PlayCard Evaluation** 目标规则，但前者要求一个具体显式目标。
- 正式提交与 **Action Preview** 共享完整的 **PlayCard Evaluation**，并只把 **Prepared PlayCard** 交给 **PlayCard Transaction**。
- **Preview Focus** 只属于 Target Preview 表现事实，不会改变 **Prepared PlayCard** 的执行绑定。
- 一次 **Action Preview** 消费一份复制状态上的 **PlayCard Transaction**。
- 正式提交与 **Action Preview** 共享同一份 **PlayCard Transaction** 顺序。
- 一个 **Action Preview** 可以包含零个或多个 **Unresolved Fact**。
- RunEvent Snapshot 与正式选项提交分别在各自调用时执行一次新的 **RunEvent Choice Evaluation**。
- **RunEvent Choice Evaluation** 只读取当前事实；选项 Effects 在求值通过后才进入 working-state 事务。
- 一个 **PlayCard Transaction** 可以包含零个或多个 **Battle Card Zone Transition**。
- **Battle Card Zone Transition** 只为真实成功移动发布事实；Action Preview adapter 继续透传到弃牌后的嵌套被动。
- **Turn Lifecycle** 在回合结束时消费一份 **Battle Card Zone Transition** 的 retained / discarded facts，并独占 `CardsRetained` 与 EndTurn presentation checkpoint 的发布时机。
- **Turn Lifecycle** 只编排既有规则 Module；Enemy Action、BattleEnd 和 Card Zone 的具体算法仍由各自 Implementation 维护。
- 每个 Effect invocation 都由同一份 **Effect Semantics** 解释；正式执行、Action Preview、Target Preview 和制作校验不维护 EffectType 分支副本。
- 一个 **Effect Chain** 可以连续消费多个 segment；Main、ZoneHook、PerfectRelease 和 Passive 只决定 chain 生命周期，不持有 scratch 或逐条解释 Effect。
- Action Preview adapter 位于 **Effect Semantics** 的 resolved invocation 与 handler 之间；随机目标选择只能在 adapter 放行后发生。
- Effect handler 把状态意图交给 **Status Semantics**；后者按宿主委托 **Combatant Mutation**、**Card Runtime State** 或 **Initiative Timeline**，不在调用方散落状态分支。
- **Turn Lifecycle** 在抽牌并重建 Hand 后物化 **Pending Hand Affliction**，并在 TurnEnd 清除回合级 Card Status。
- **PlayCard Evaluation**、Target Preview、Action Preview 和 Snapshot 共享 **Card Runtime State** 的费用与冻结事实。
- **Status Semantics** 在出牌开始时捕获敌方 Freeze，确保当前卡新施加的 Freeze 只拦截下一次真实先机推进。
- 一次伤害 **Combatant Mutation** 产生一份 **Damage Facts**；事件、Combat Log 和 Action Preview 共享其中的实际 HP 损失。
- **Battle Initialization Result** 与单次命令的 `FBattleResolution` 分别拥有自己的事件批次；BattleSession 不提供跨调用累积输出队列。
- **Local Settings** 可以在旅程加载前生效；它与玩家档案、活动旅程和 Battle / Run Snapshot 没有序列化或同步关系。

## Example dialogue

> **开发者：** “这张牌的弃牌被动需要在 Action Preview 里再写一套吗？”
> **规则设计：** “不需要。让 Action Preview 在副本上执行同一份 PlayCard Transaction；随机后续记录为 Unresolved Fact。”

> **开发者：** “全体攻击卡 hover 到一个敌人部位时，这个部位会变成正式命令目标吗？”
> **规则设计：** “不会。它只是 Preview Focus；PlayCard Evaluation 会为 AllEnemyParts 生成不带单部位绑定的 Prepared PlayCard。”

> **开发者：** “RunEvent 按钮已经显示可用，正式提交能直接信这个 Snapshot 吗？”
> **规则设计：** “不能。正式提交要针对最新 RunState 重新执行 RunEvent Choice Evaluation。”

## Flagged ambiguities

- “Preview” 曾同时指轻量 Target Preview 和完整 Action Preview；本文中的 **Action Preview** 只指完整出牌后的确定性净结果预测。
- Target Preview 中的 **Preview Focus** 曾被当作正式执行目标；现在两者明确分离，`None / Self / AllEnemyParts` 的规范化命令不携带显式目标。
- RunEvent Snapshot 的 `bAvailable` 是生成时的 **RunEvent Choice Evaluation** 事实，不是可缓存的提交授权。
