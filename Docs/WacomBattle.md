---
type: domain-spec
scope: wacom-battle
status: active
updated: 2026-06-15
tags:
  - wacom/battle
  - wacom/rules
  - wacom/combat
  - wacom/snapshot
---

# WacomBattle 模块文档

> [!info] 本文职责
> 本文是 WacomBattle 模块的当前规则真相。规则变更时先改本文，再改代码。

> [!note] 相关入口
> Battle UI 表现见 [WacomBattleUI.md](./WacomBattleUI.md)，场景目标与 target handle 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，first-person 拖卡表现见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。静态字段见 [WacomData.md](./WacomData.md)，GameplayTag 见 [WacomGameplayTags.md](./WacomGameplayTags.md)，制作矩阵和校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。战后 Run 结算见 [WacomRun §10](./WacomRun.md#wacomrun-battle-settlement)，长期方向见 [Roadmap: 战斗规则](./Roadmap.md#roadmap-battle-rules)。

## §1 模块职责与边界

WacomBattle 是单场战斗的规则内核。它负责战斗生命周期、玩家命令结算、手牌区域、效果执行、敌方部位行动、Snapshot、Event 和 ResultPacket。

WacomBattle 不负责 UI 展示、世界 Actor authoring、Run 探索、存档、静态资产生成或 Data Validation。它可以读取 DataAsset 定义，但不决定资产制作矩阵。

边界约束：

- `UBattleSession` 是外部模块进入单场战斗规则的唯一 public facade。
- UI、Actor 和测试入口不能直接修改 `BattleState`。
- 外部只提交 `FBattleCommand`，读取 `FBattleSnapshot`、`FBattleEvent`、`FKnockdownChoiceView` 和 `FBattleResultPacket`。
- `BattleState`、resolver、dispatcher 和 helper 都属于 `WacomBattle/Private` 内核实现。

核心对象：

| 对象 | 职责 |
|---|---|
| `UBattleSession` | 单场战斗 public facade；持有状态、事件并委托 private helper |
| `BattleState` | Private 可变规则状态 |
| `FBattleCommand` | 玩家或系统提交的规则命令 |
| `FBattleSnapshot` | UI / 测试读取的当前只读状态 |
| `FBattleEvent` | 结算过程事件流；不是真正规则状态 |
| `FBattleResultPacket` | BattleEnd 后给 Run 层消费的战后包 |
| `BattleCommandPipeline` | 拦截 BattleEnd、分派 resolver、维护版本兜底和击倒请求 |
| `BattleResolver` | PlayCard / Wait / EndTurn / KnockdownChoice 调度入口 |
| `CardEffectDispatcher` | Target 映射、条件、Magnitude 和 Effect 执行分发 |
| `EffectExecutor` | 按 `EffectTag -> Handler` 注册制执行效果 |
| `HandZoneService` | 手牌区域、上限和腾挪规则 |
| `EnemyIntentSelector` | 按 BehaviorDefinition / phase / intent set / selector rule 刷新敌方部位当前意图 |
| `EnemyPartActionResolver` | 敌方部位行动子流程 |
| `BattleResultPacketBuilder` | 从 `BattleState` 构造战后包 |

## §2 Public Contract

| 契约 | 当前用途 |
|---|---|
| `UBattleSession::Initialize()` | 根据 `FBattleInitParams` 创建单场战斗，发 `BattleStarted / TurnStarted`，进入首个玩家回合 |
| `UBattleSession::SubmitCommand()` | 唯一命令入口：`PlayCard / Wait / EndTurn / KnockdownChoice` |
| `UBattleSession::BuildSnapshot()` | 输出当前只读状态；UI 和测试读取，不作为事件历史 |
| `UBattleSession::ConsumeEvents()` | 输出并清空自上次消费以来的事件流 |
| `UBattleSession::BuildPendingKnockdownChoiceView()` | 输出当前击倒选择 ViewData；UI 不解析事件 `Count` 位掩码 |
| `UBattleSession::BuildCardTargetPreview()` | 输出单张手牌对候选目标的只读目标预览 facts；不提交命令、不改 BattleState |
| `UBattleSession::BuildResultPacket()` | BattleEnd 后输出战后包；具体 Run 结算见 [WacomRun §10](./WacomRun.md#wacomrun-battle-settlement) |

`SubmitCommand()` 是同步规则结算入口。命令成功后，`BattleState`、Snapshot 派生事实和事件列表立即更新。

规则层不等待 UI 动画。BattleHUD 的 presentation queue、Combat Log、Presentation Stack 和 turn-boundary barrier 都属于表现层；它们只决定何时把玩家意图提交成 `FBattleCommand`，不改变命令本身。

普通玩家命令在 `PlayerAction` 阶段提交。`PendingKnockdownChoice`、`BattleEnd` 和非玩家行动阶段会阻止普通 `PlayCard / Wait / EndTurn`。

Battle 初始化只接受 `FBattleInitParams.EnemySlots` 作为敌人入口。`UEncounterDefinition` 是 WacomData 层的静态 Encounter 合同，不由 BattleSession 直接读取；`ABattleTriggerActor` 在进入战斗前把 Encounter 敌人槽转换成 `FBattleInitParams.EnemySlots`。运行态 `EncounterId` 由场景 Trigger 的 `PersistentId` 提供，而不是 Encounter 资产 ID。

`FBattleSnapshot.Enemies` 是敌人快照的唯一 public 入口。它按初始化 `EnemySlots` 顺序输出 `FEnemySnapshot`，每个 enemy 下再输出 `Parts`。不再提供 `FBattleSnapshot.Enemy` 或“第一个敌人”别名；UI、日志、场景目标绑定和新测试都应遍历 `Enemies`，或在明确单敌人 fixture 中显式读取 `Enemies[0]`。敌方部位长期身份以 `EncounterId + EnemySlotId + PartSlotId` 为准；`PartId` 只保留在静态内容定义和 debug 语义中，不参与运行时目标匹配。

## §3 PlayCard 与目标合同

`FBattleCommand::Type == PlayCard` 时，`CardInstanceId` 必填。目标字段由卡牌 `TargetMode` 决定：

| TargetMode | 命令字段 | 合法性 |
|---|---|---|
| `None / Self / AllEnemyParts` | 不要求额外目标 | 检查源卡在手牌、费用合法 |
| `SingleEnemyPart` | `TargetEnemyPartKey` | 目标 key 必须解析到当前战斗中未破坏的敌方部位 |
| `HandCard` | `TargetCardInstanceId` | 目标必须是另一张当前手牌；拒绝 self、无效 ID、已离开手牌的卡 |

`TargetMode=HandCard` 会把玩家选中的目标手牌作为 `Target.SelectedHandCard` 传给主效果链。基础资格由 `UCardDefinition::HandCardTargetFilter` 决定：

- 显式开启 `bUseExplicitHandCardTargetFilter` 时，`bAllowNormalHandCards / bAllowHandAnchors` 直接决定普通手牌和左右手锚点是否可选。
- 未显式开启时走兼容推断：普通 `HandCard` 默认允许普通手牌和左右手锚点；包含 `Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected + Target.SelectedHandCard` 的源卡默认只允许普通手牌。
- `RequiredTargetKeywords` 要求目标有效关键词全部命中；`BlockedTargetKeywords` 命中任意一个就拒绝。
- 目标有效关键词 = 卡牌定义关键词 + 战斗中的临时关键词。
- 左右手锚点如果被允许，也同样参与 keyword 条件。
- self target 全局禁止，不提供卡牌字段开放。

当前 `Effect.Card.AddCost / Effect.Card.ReduceCost` 可精确作用到目标手牌；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 可把选中的普通手牌移入弃牌堆 / 消耗牌堆。费用、卡牌类型、区域、伙伴 / 食物专用属性等更复杂筛选属于后续扩展方向。

`UBattleSession::ValidateTargetWithCard(CardInstanceId, TargetHandle)` 是拖拽 preview / debug 使用的只读校验入口，返回 `FWacomBattleTargetValidationResult`。调用方读取 `bCanTarget` 判断是否可选，并可使用 `RejectReason / DebugSummary / ResolvedPartKey` 做 UI 反馈和排查；不再保留 bool-only 兼容入口。

`UBattleSession::BuildCardTargetPreview(CardInstanceId, TargetHandle)` 是 Battle 卡牌目标预览的 public 入口。它先复用目标校验，再返回 `FBattleCardTargetPreview`：validation、源卡运行时费用 / 迅捷事实、目标类型、resolved enemy part 或 target hand card，以及每个主效果的 preview magnitude / skip facts。该 API 只读，不触发事件、不修改手牌 / 敌人 / 状态，也不模拟整次 `PlayCard` 事务。

敌人部位目标的 preview 归 `WacomBattle` 计算，复用 private 规则路径：`MagnitudeResolver`、`ConditionResolver`、`MagnitudeModifiers`、武器容量伤害 +3、伤害 clamp，并与正式结算共享最终 Magnitude helper。App / Widget 只能把这些 facts 转成卡面和详情 ViewData，不能复制或重算战斗规则。

手牌目标的 preview 第一版只覆盖主效果摘要：`Effect.Card.AddCost / Effect.Card.ReduceCost` 预测目标卡费用变化；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected / Effect.Card.GainKeyword` 返回结构化动作事实。它不执行真实移动、不写临时关键词、不触发后续事件链。`AllEnemyParts` 在 hover 某个部位时按当前指向的单部位生成 preview；全体目标聚合预览属于后续扩展。

Battle world target 按 handle 上的 `EncounterId + EnemySlotId + PartSlotId` 构造 `FBattleEnemyPartKey` 并定位。`WorldTargetId`（runtime GUID）只作为表现层目标 cue / debug 的运行时校验字段；如果 handle 同时携带 runtime GUID 和稳定 key，两者必须指向同一部位，否则返回 `TargetIdentityMismatch`。`FBattleCommand` 不再接受 runtime part GUID 或 slot 字段作为敌方目标，最终提交统一使用 `TargetEnemyPartKey`。

Validation 只解释“这个目标能不能被这张卡作用”。它不校验费用、UI 状态、动画队列或命令提交时机；最终提交仍由 `PlayCardResolver` 再校验。`FWacomBattleTargetValidationResult` 会回填 `ResolvedPartKey`，并保留 `ResolvedPartInstanceId / ResolvedPartIdentity` 作为表现 cue / debug 投影。

## §4 战斗流程

回合结构：

```text
敌人初始化 -> 起始阶段 -> 执行阶段 -> 结束阶段 -> 若战斗未结束，回到起始阶段
```

PlayCard resolver 当前顺序：

1. 发射 `CardPlayed`。
2. 执行 `ZoneHook.OnPlay`。
3. 记录所有存活敌方部位的出牌前先机。
4. 判断 `PerfectRelease` 候选。
5. 对先机命中的部位执行 `Resistance` 判定。
6. 执行卡牌主动 `Effect`。
7. 执行完美释放效果。
8. 非迅捷卡推进敌方先机，除非被 `ZoneHook.OnPerfectReleaseHit` 跳过。
9. 执行卡牌离开手牌后的去向规则。
10. 若本卡有 Companion 关键词，全局计数 +1。
11. 执行 `Passive.AfterPlayed`。
12. 执行 `Passive.OnCompanionCount`。
13. 结算 `Poison`。
14. 若有部位先机 <= 0，执行敌方部位行动子流程。
15. 检查战斗结束。
16. 若未结束，递增 `StateVersion`，返回执行阶段。

起始阶段：

```text
重置 CurrentWaitValue 为 2
-> 从抽牌堆抽取 5 张普通卡
-> 合并上回合保留普通卡与本回合新抽普通卡
-> 重新随机编排普通卡池
-> 重新插入左右手锚点
-> 执行普通手牌上限
-> 发 CardsDrawn / HandLimitDiscarded / CardDiscarded / HandZoneChanged
-> Phase 切到 PlayerAction
```

结束阶段：

```text
发 TurnEnded
-> 本回合使用牌堆自然进入弃牌堆
-> 非保留普通卡进入弃牌堆
-> 必要时发 HandZoneChanged
-> 敌方行动前 early-exit 检查
-> 执行敌方部位行动子流程
-> 检查战斗结束
-> 若未结束，回到起始阶段
```

等待流程：

```text
所有敌方部位 CurrentInitiative -= CurrentWaitValue
-> 若有部位先机 <= 0，执行敌方部位行动子流程
-> CurrentWaitValue += 1
-> 返回 PlayerAction
```

当前未接入“战斗开始时 / 回合开始时 / 持续到回合开始”效果调用点。`PassiveDispatcher::RunOnTurnStart` 方法存在，但主流程不调用；触发点扩展见 [Roadmap: 被动触发点扩展](./Roadmap.md#roadmap-battle-rules)。

当前未接入“回合结束时 / 持续到回合结束”效果系统；非保留普通卡弃牌发生在结束阶段前半段。若要调整到敌方行动之后，需同步更新规则文档和 [TechDebt.md](./TechDebt.md) 中的时序债。

## §5 手牌区域规则

区域结构：

```text
[左手区] [左手牌] [双手区] [右手牌] [右手区]
```

左手牌和右手牌是手牌区域锚点，也是可打出的卡。它们自带保留，不进入抽牌堆、弃牌堆或消耗牌堆；被打出后进入 `Limbo`，下回合开始重新插回手牌队列。

抽牌和重建：

- 回合开始固定从抽牌堆抽取 5 张普通卡。
- 回合开始把上回合保留普通卡和本回合新抽普通卡合成预备普通卡池。
- 预备普通卡池每回合重新随机编排，不保留上回合 index、相对顺序或区域。
- 两张锚点都有效且普通卡池非空时，两者之间至少一张普通卡。
- 两张锚点都有效但普通卡池为空时，fallback 队列为 `[LeftHand, RightHand]`。
- 战斗中途的 `Effect.Draw` 不重建整条手牌；抽到或回收的普通卡逐张随机插入当前手牌队列，并立即执行普通手牌上限。
- `Effect.Draw` 触发普通手牌上限时，正在打出的抽牌源卡会被排除在本次上限弃牌候选外。

手牌上限：

- 普通卡牌上限 10，不计算左手牌和右手牌。
- 最终手牌队列上限 12。
- 超出上限的普通卡牌立即移动到弃牌堆；中途抽牌、回手和击倒奖励卡入手也会即时检查。
- 每张因普通手牌上限进入弃牌堆的卡都会产生 `HandLimitDiscarded`，并同步记录真正弃牌路径的 `CardDiscarded`。
- `HandLimitDiscardSource` 区分 `TurnStart / EffectDraw / PassiveOnCompanionCount / None`；击倒奖励卡触发上限弃牌时当前为 `None`。
- `HandZoneChanged` 只表示手牌区需要刷新，不承载具体弃牌语义。

`OnDiscard` 表示“本卡被弃掉”，不表示“任何进入弃牌堆”。会触发的路径包括 `Effect.Discard`、`Effect.Card.DiscardSelected`、普通手牌上限弃牌和回合结束非保留普通卡弃牌。普通打出进入本回合使用牌堆、回合结束自然转入弃牌堆、`Effect.Card.ExhaustSelected` 和 `Effect.ExhaustSelf` 不触发 `OnDiscard`。

腾挪会重新放置当前手牌中的普通卡，使其进入不同区域或同一区域随机位置。默认不选择左右手锚点。左手牌或右手牌离开手牌区时，双手区立刻失效；左右手牌都不在时，所有普通卡 `Zone=None`，腾挪不可用。

区域判定只看两张锚点在队列中的相对位置：较小索引左侧为 Left，中间为 Both，较大索引右侧为 Right。当前锚点不被腾挪，物理方向不会颠倒。

## §6 先机、PerfectRelease 与 Resistance

Cost 推进：

- 非迅捷卡打出后，所有未破坏部位 `CurrentInitiative -= RuntimeCost`。
- 迅捷卡不扣减先机。
- 被 `ZoneHook.OnPerfectReleaseHit` 标记跳过时不扣减。

`PerfectRelease` 判定：

```text
出牌前记录所有存活部位的 CurrentInitiative
若 CardRuntimeCost == Part.CurrentInitiativeBeforePlay，则该 Part 命中 PerfectRelease
```

多个部位可同时命中。迅捷卡不触发完美释放。主效果致死的部位不参与完美释放。

`Resistance` 在先机命中时触发，且先于完美释放。卡牌抵抗值来自主效果中首个 `Effect.Damage` 的 FinalMagnitude；无伤害效果则为 0。意图抵抗值来自 `FIntentDefinition::ResistanceValue`；非攻击意图填 0。

当 `CardResistance > IntentResistance` 时，该部位进入 `Status.Stunned`。抵抗不改变伤害、不改变先机、不阻止先机推进。

UI 先机预测、scene part Status Badge 和拖卡 preview 只读取 Snapshot / validation 事实，不提交命令、不模拟完整 resolver。表现合同见 [WacomBattleUI.md](./WacomBattleUI.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) 和 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §7 状态、保留、ZoneHook 与 Passive

`Poison` 触发时机：

- 玩家每打出一张牌后，对敌我双方各结算一次当前中毒层数伤害。
- 敌方部位每行动一次后，对敌我双方各结算一次当前中毒层数伤害。

`Poison` 造成等于当前中毒层数的生命伤害，穿透护盾，层数不因结算而减少。`Effect.Heal` 治疗玩家时，移除治疗量 10% 的中毒层数，向下取整；层数为 0 时移除 `Status.Poison`。

敌方每个部位和玩家本体各自持有状态层数。`Status.Shield` 是护盾数值入口，直接写入 `Shield` 字段，不进入 `StatusStacks`。

保留规则：

- 拥有 `Card.Keyword.Retain` 的卡牌回合结束时不弃掉。
- 左手牌和右手牌自带保留。
- 双手区可以保留普通卡：回合结束时左右手牌都仍在手牌区，则双手区普通卡保留到下一回合。
- 保留只保留卡牌仍在下一回合手牌池中，不保留 index、相对顺序或区域。
- 保留牌如果主动打出，仍按普通打出处理，除非卡牌文本另有说明。

`ZoneHook.OnPlay` 在 `CardPlayed` 之后、记录出牌前先机之前触发，用于执行该区域 ExtraEffects。`ZoneHook.OnPerfectReleaseHit` 在完美释放命中时触发，当前只作为“跳过先机推进”标记。

当前 `Passive` 触发点：

| Trigger | 触发时机 | 当前语义 |
|---|---|---|
| `OnCompanionCount` | 全局 Companion 计数达阈值 | 触发后计数清零；典型效果为回手并执行手牌上限 |
| `OnTwilightTriggered` | 暮气施加成功时 | 发 `PassiveTriggered` 事件；当前不改 Magnitude |
| `AfterPlayed` | 本卡打出完成后 | 典型效果为自腾挪到随机区域 |

## §8 Effect Executor 与制作边界

Effect executor 按 `EffectTag -> Handler` 注册制分派。新增效果类型应注册 handler，不改调度逻辑。

| EffectTag | Handler | 当前语义 |
|---|---|---|
| `Effect.Damage` | DamageHandler | 造成伤害 |
| `Effect.Heal` | HealHandler | 恢复玩家 HP，并按治疗量移除玩家中毒 |
| `Effect.ApplyStatus.Poison` | ApplyStatusHandler | 施加中毒 |
| `Effect.ApplyStatus.Slow` | ApplyStatusHandler | 记录层数；数值效果未接入 |
| `Effect.ApplyStatus.Freeze` | ApplyStatusHandler | 记录层数；行动时走跳过意图分支 |
| `Effect.ApplyStatus.Twilight` | ApplyStatusHandler | 记录层数，并触发 `OnTwilightTriggered` |
| `Effect.Shuffle.Random` | ShuffleHandler | 随机腾挪 |
| `Effect.Shuffle.FromBothToOther` | ShuffleHandler | 从双手区腾挪到其他区域 |
| `Effect.Shuffle.ToRandomZone` | ShuffleHandler | 腾挪到随机区域 |
| `Effect.Draw` | DrawHandler | 从 Draw / Discard / Exhaust 入手并随机插入当前手牌 |
| `Effect.Discard` | DiscardHandler | 随机弃掉普通手牌，不弃左右手锚点 |
| `Effect.ExhaustSelf` | ExhaustSelfHandler | 给源卡加临时消耗关键词，出牌去向阶段进消耗牌堆 |
| `Effect.Card.AddCost` | CostModHandler | 增加目标手牌费用 |
| `Effect.Card.ReduceCost` | CostModHandler | 降低目标手牌费用 |
| `Effect.Card.DiscardSelected` | SelectedHandCardZoneMoveHandler | 要求 `Target.SelectedHandCard`；目标普通手牌移入弃牌堆并触发 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | SelectedHandCardZoneMoveHandler | 要求 `Target.SelectedHandCard`；目标普通手牌移入消耗牌堆，不触发 `OnDiscard` |
| `Effect.GainKeyword` | GainKeywordHandler | 给目标手牌临时添加 `MetaTag` 指定关键词 |
| `Effect.RemoveStatus` | RemoveStatusHandler | 移除目标 `MetaTag` 指定状态的若干层 |
| `Effect.ModifyInitiative` | ModifyInitiativeHandler | 直接修改目标部位当前先机 |
| `Status.Shield` | ShieldHandler | 写入护盾字段 |

效果字段和 DataAsset 契约见 [WacomData.md](./WacomData.md)，GameplayTag 字典见 [WacomGameplayTags.md](./WacomGameplayTags.md)，当前可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)。

`FWacomBattleRuleContentContract` 是 resolver / dispatcher 的只读 authoring matrix。它供编辑器校验读取，不执行规则，不改变 `UBattleSession::SubmitCommand()` 同步结算，也不让 `WacomData` 依赖 `WacomBattle`。

GameplayTag 已声明不等于已可制作。能否进入 DataAsset，以 `FWacomBattleRuleContentContract`、Data Validation 和 `WacomDataAuthoring` authoring matrix 为准。

新增 Effect、Target、MagnitudeSource、Condition 或 Passive 触发点时，应同时更新 runtime resolver、authoring matrix、Data Validation、生成内容测试和相关文档。

敌方 Intent 当前只允许 `Target.Player` 和 `Target.Self`，不支持手牌目标、全体敌方部位目标或卡牌专用效果。正式内容通过 `UEnemyBehaviorDefinition` 的 phase / intent set / selector rule 选择意图；`UEnemyPartDefinition` 只承载部位静态数值与奖励，不再承载行为序列。

## §9 敌方部位行动子流程

触发来源：

- 等待使部位先机 <= 0。
- 打牌推进先机后部位先机 <= 0。
- 结束阶段，所有存活且可行动的部位行动。
- 其他卡牌、状态或事件强制行动。

行动流程：

```text
收集本次需要行动的部位
-> 移除已破坏部位
-> 移除不能行动部位
-> 按部位顺序逐个结算
   -> 若部位处于晕厥，跳过当前意图
   -> 否则执行该部位当前意图
   -> 刷新该部位意图
   -> 将 CurrentInitiative 设置为新意图先机
   -> 结算 Poison
   -> 检查玩家失败、部位破坏、敌人死亡和 BattleEnd
```

当前意图刷新由 `EnemyIntentSelector` 负责：

- 初始化时，Battle 从 `UEnemyDefinition.DefaultBehavior` 或 `FEnemyPartSlot.BehaviorOverride` 取得行为资产，设置 `CurrentPhaseId / PreferredIntentSetId`，并刷新首个当前意图。
- `Sequence` intent set 会按 authored 顺序选择下一条可用意图；`Weighted` 使用战斗 RNG 在有效 rule 中确定性选择；`PriorityFirst` 选择最高优先级有效 rule。
- selector condition 当前支持自身 HP 阈值、同单位任意部位 HP 阈值、部位已破坏、当前 phase、自身状态、玩家状态和冷却可用。
- 每次部位行动后，无论执行还是因晕厥 / 冻结跳过，都会刷新到下一条当前意图，并把 `CurrentInitiative` 设置为新意图先机。
- Snapshot 暴露每个部位当前 `CurrentPhaseId / CurrentIntentSetId / CurrentIntentId`，以及当前意图的 `IntentId / DisplayName / Initiative / ResistanceValue`。
- 初始化和行动后意图刷新会发 `EnemyIntentSelected` 事件；初始化 phase 会发 `EnemyPhaseChanged` 事件。当前还没有 phase transition resolver，因此运行中 phase 变化事件只预留给后续 phase 切换规则。

晕厥以层数模型记录。每次该部位行动时，无论执行意图还是跳过意图，都消耗 1 层；层数归零时移除 `Status.Stunned`。

冻结状态当前共享“跳过意图 + 消耗 1 层”分支。无论部位是否执行意图，只要完成本次行动结算，就刷新到下一个意图，并把当前先机设为新意图先机。

## §10 BattleState

`BattleState` 是战斗内核的可变状态，位于 `WacomBattle/Private`，外部模块不可见。

```text
BattleState
├── Meta
│   ├── Phase
│   ├── TurnNumber
│   ├── CurrentWaitValue
│   ├── Outcome
│   ├── StateVersion
│   └── Rng
├── 战内 -> 战外回传 flag
│   ├── HighHpThreshold / LowHpThreshold
│   ├── bCrossedHighHpThreshold / bCrossedLowHpThreshold
│   ├── bMutualDestruction
│   ├── PendingKnockdownExpGains[]
│   ├── PendingKnockdownEvents[]
│   ├── PendingKnockdownChoices[]
│   ├── PendingGainedCards[]
│   └── DestroyedParts[]（内部 identity 投影）
├── PlayerState
│   ├── CurrentHp / MaxHp
│   ├── Shield
│   ├── StatusStacks
│   └── CompanionPlayedCount
├── CardContainers
│   ├── LeftHandInstanceId / RightHandInstanceId
│   ├── Hand
│   ├── DrawPile
│   ├── PlayedPile
│   ├── DiscardPile
│   ├── ExhaustPile
│   └── Limbo
└── EnemyState
    ├── EnemySlots[]
    ├── Parts[]
    └── PartIndexByKey
```

卡牌离开手牌后的去向：

| 条件 | 去向 |
|---|---|
| 普通牌 | 本回合使用牌堆 |
| 消耗牌 | 消耗牌堆 |
| 左手牌 / 右手牌 | `Limbo` |
| 连击牌 | 留在原位置 |
| 保留牌打出 | 本回合使用牌堆；保留只影响回合结束弃牌 |

本回合使用牌堆（`PlayedPile`）只保存本回合自然打出的普通牌。抽牌堆耗尽时只把弃牌堆洗回抽牌堆，`PlayedPile` 不参与同回合洗牌。玩家回合结束时，`PlayedPile` 整体自然转入弃牌堆；这个转移不发 `CardDiscarded`，也不触发 `OnDiscard`。

## §11 BattleEvent

`FBattleEvent` 是结算过程记录流，供 UI 播表现、Combat Log 和自动化测试使用。事件不是真正规则状态；恢复、刷新和权威读取仍以 `BattleState` / `FBattleSnapshot` 为准。

| 类型 | 当前用途 |
|---|---|
| `BattleStarted` | `Initialize` 成功后发出 |
| `TurnStarted` | `Initialize` 成功后首回合发出 |
| `CardsDrawn` | 回合开始抽牌或 `Effect.Draw` 成功入手 |
| `HandZoneChanged` | 手牌队列、区域或上限弃牌后需要 UI 刷新 |
| `CardPlayed` | 玩家打出卡牌 |
| `InitiativeHit` | 出牌前先机命中部位 |
| `ResistanceResolved` | 抵抗判定完成 |
| `PerfectReleaseResolved` | 完美释放效果完成 |
| `DamageDealt` | 实际扣血 |
| `StatusApplied` | 状态层数施加 |
| `InitiativePushed` | 非迅捷卡推进敌方先机 |
| `WaitPerformed` | 玩家等待 |
| `EnemyPartActed` | 敌方部位行动 |
| `EnemyPartHpEmptied` | 部位 HP 归零并进入破坏态 |
| `KnockdownChoiceRequested` | 击倒事件等待玩家选择 |
| `KnockdownChoiceMade` | 玩家完成击倒选择 |
| `TurnEnded` | 玩家结束回合 |
| `PassiveTriggered` | 被动触发通知；部分被动只发事件不改数值 |
| `HandLimitDiscarded` | 普通手牌上限导致某张卡弃掉 |
| `CardDiscarded` | 卡牌因弃牌规则从手牌进入弃牌堆 |
| `CardExhausted` | 卡牌因消耗规则从手牌进入消耗牌堆 |
| `CardGained` | 战斗中获得新卡 |
| `BattleEnded` | 战斗进入结束态 |

`EnemyKnockdown` enum 保留在公共类型中，但当前击倒路径使用 `EnemyPartHpEmptied + KnockdownChoiceRequested + KnockdownChoiceMade`。不要把 `EnemyKnockdown` 当作活跃事件依赖。

中文文案、tone、icon、Combat Log、Toast 和玩家可读命令块属于表现层，见 [WacomBattleUI.md](./WacomBattleUI.md) 与 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。它们不改变 `FBattleEvent` 协议，也不新增规则层 command batch id。

<a id="wacombattle-result"></a>
## §12 战斗结束与 ResultPacket

`FBattleRules::CheckAndApplyBattleEnd` 在每次状态变更后调用：

| 条件 | Outcome |
|---|---|
| 敌方部位全死，玩家 HP > 0 | `Victory` |
| 敌方部位全死，玩家 HP = 0 | `Victory`，并置 `bMutualDestruction=true` |
| 敌方未全死，玩家 HP = 0 | `Defeat` |

任一情况都会把 `Phase` 切到 `BattleEnd` 并发射 `BattleEnded`。

击倒事件优先级：如果敌方全破、玩家未死亡，但 `PendingKnockdownEvents` 仍非空，战斗不会立刻进入 `BattleEnd`。玩家必须先处理击倒三选一；最后一个存活部位被击倒时，撤离不可选，只能 Aid 或 Destroy 后进入 Victory。玩家死亡路径例外，失败优先级高于待处理击倒事件。

战斗结束时，`UBattleSession::BuildResultPacket()` 委托 `BattleResultPacketBuilder` 构造 `FBattleResultPacket` 给 Run 层：

| Packet 字段 | Battle 来源 | 说明 |
|---|---|---|
| `Outcome` | `BattleState.Outcome` | `Victory / Defeat / Undetermined` |
| `bCrossedHighHpThreshold` | `BattleState.bCrossedHighHpThreshold` | 玩家 HP 首次低于 High 阈值 |
| `bCrossedLowHpThreshold` | `BattleState.bCrossedLowHpThreshold` | 玩家 HP 首次低于 Low 阈值 |
| `bMutualDestruction` | `BattleState.bMutualDestruction` | 玩家 HP=0 与敌方全破同时发生 |
| `bWithdrawn` | `KnockdownChoices` 含 Withdraw | 撤离以 `Outcome=Victory + bWithdrawn=true` 表达 |
| `KnockdownExpGains[]` | `BattleState.PendingKnockdownExpGains` | 部位破坏经验记账 |
| `KnockdownChoices[]` | `BattleState.PendingKnockdownChoices` | 玩家击倒三选一选择列表 |
| `GainedCards[]` | `BattleState.PendingGainedCards` | 战斗中获得、战后可能进入 Run 的卡牌 |
| `DestroyedPartKeys[]` | `BattleState.DestroyedParts -> FBattleEnemyPartKey` | 本场截至结束已破坏部位的稳定公开 key，用于 Run 撤离重入 |
| `DestroyedParts[]` | `BattleState.DestroyedParts` | 内部 identity 投影，用于 runtime 汇总和 debug；Battle 外规则真相使用 `DestroyedPartKeys[]` |

Battle 只负责产出战后包。疲劳、伤口、经验、获得卡、撤离进度、节点消耗等战外处理见 [WacomRun §10](./WacomRun.md#wacomrun-battle-settlement)。

击倒事件补充：

- Aid / Destroy 是击倒事件分支，不依赖左 / 右手牌当前是否仍在手牌区，也不消耗左右手牌。
- 如果被击倒部位配置 `KnockdownRewardCard`，选择 Aid 或 Destroy 会立刻创建战斗内 runtime card，随机插入当前手牌，并发出 `CardGained`；Withdraw 不触发奖励卡。
- `CardGained.CardInstanceId` 是新建战斗内卡实例，`ActorInstanceId` 是来源部位实例，`CardDefinition` 是奖励卡定义，`Count` 记录本次 `EKnockdownChoice`。
- 奖励卡选择时即写入 `FBattleResultPacket.GainedCards`。后续即使这张战内卡被打出、弃掉、消耗或被上限弃牌，战后 Victory 仍会进入 Run。
- 撤离只在敌人仍有存活部位时可选；如果本次击倒后敌人所有部位都已清空，必须 Aid 或 Destroy。
- `UBattleSession::BuildPendingKnockdownChoiceView()` 输出当前击倒事件的 `FKnockdownChoiceView`。`DisabledReason` 当前使用 `None / NoLivingEnemyPart`；`LeftHandMissing / RightHandMissing` 已预留但未被当前规则触发。
- `KnockdownChoiceRequested` 事件只通知 UI 需要展示选择面板；`FBattleEvent.Count` 的旧位掩码仅保留日志兼容，不作为 UI 读取合同。

`Outcome=Undetermined` 不结算压力；它只用于异常路径或玩家取消。

部位 HP 归零瞬间会记一条 `FKnockdownExpGain` 到 `BattleState.PendingKnockdownExpGains`。每个部位只记一次，未填 `ExperienceReward` 的部位仍记录 `ExpAmount=0`，让 Run 层有完整的被破坏部位列表。
