---
type: domain-spec
scope: wacom-battle
status: active
updated: 2026-07-11
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
| `UBattleSession` | 单场战斗 public facade；持有状态与事件序号，并委托 private helper |
| `BattleState` | Private 可变规则状态 |
| `FBattleCommand` | 玩家或系统提交的规则命令 |
| `FBattleSnapshot` | UI / 测试读取的当前只读状态 |
| `FBattleEvent` | 结算过程事件流；不是真正规则状态 |
| `FBattleInitializationResult` | 一次初始化的原子结果；绑定 status、opening events 与 post snapshot |
| `FBattleResolution` | 一条 C++ 命令的原子结果；绑定 status、前后版本、events、journal 与 post snapshot |
| `FBattlePresentationJournal` | 单次命令内的 C++ only 只读表现 journal；记录 EndTurn checkpoint 和任意正式命令的有序 Deck Steps |
| `FBattleResultPacket` | BattleEnd 后给 Run 层消费的战后包 |
| `BattleCommandPipeline` | 拦截 BattleEnd、分派 resolver 和触发击倒请求；不拥有版本号 |
| `BattleResolver` | PlayCard / Wait / EndTurn / KnockdownChoice 调度入口 |
| `PlayCardEvaluator` | 唯一 PlayCard Evaluation Implementation；只读求值源卡、目标、费用、当前可提交性和 Prepared PlayCard |
| `PlayCardResolver` | 唯一 PlayCard Transaction Implementation；只消费 Prepared PlayCard，不重复前置规则 |
| `EffectSemanticRegistry / semantic families / CardEffectChain` | Effect descriptor 唯一事实、按 Combatant/CardMovement/CardRuntime/Initiative 分组的 Implementation，以及运行时 chain scratch |
| `BattleStatusSemanticsModule` | Status Semantics 的唯一 Private Implementation；解释宿主、生命周期、消费、Poison cadence 与 Pending Hand Affliction |
| `BattleCardRuntimeStateModule` | 单卡状态、费用组成、冻结限制和卡牌状态事件的唯一 Private Implementation |
| `BattleInitiativeTimelineModule` | 正常运行时 `CurrentInitiative` 的唯一写入口，返回逐部位实际变化 facts |
| `HandZoneService` | 手牌区域、上限和腾挪规则 |
| `CardZoneAggregate` | 所有 runtime card 注册、定位容器、`Location` 与同区顺序的唯一 Private 写入口 |
| `BattleCardZoneTransition` | 消费 typed zone transition facts，统一弃置/消耗、HandLimit、奖励与 Companion 的事件和被动顺序 |
| `BattleTurnLifecycleModule` | Turn Lifecycle 的唯一 Private Implementation；集中首回合、TurnStart、TurnEnd、双 BattleEnd gate、敌方行动和 checkpoint 顺序 |
| `EnemyIntentSelector` | 按 BehaviorDefinition / phase / intent set / selector rule 刷新敌方部位当前意图 |
| `EnemyPartActionResolver` | 敌方部位行动子流程 |
| `BattleResultPacketBuilder` | 从 `BattleState` 构造战后包 |

## §2 Public Contract

| 契约 | 当前用途 |
|---|---|
| `UBattleSession::Initialize()` | C++ 初始化入口；原子返回 `FBattleInitializationResult`，失败时保留当前旧战斗 |
| `UBattleSession::ResolveCommand()` | 唯一命令入口；原子返回 `FBattleResolution` |
| `UBattleSession::BuildSnapshot()` | 输出当前只读状态；UI 和测试读取，不作为事件历史 |
| `UBattleSession::BuildPendingKnockdownChoiceView()` | 输出当前击倒选择 ViewData；UI 不解析事件 `Count` 位掩码 |
| `UBattleSession::BuildCardTargetPreview()` | 输出单张手牌对候选目标的只读目标预览 facts；不提交命令、不改 BattleState |
| `UBattleSession::BuildCardActionPreview()` | 输出松手后可确定的 projected player / enemy part values；不展开随机结果、不改 BattleState |
| `UBattleSession::BuildResultPacket()` | BattleEnd 后输出战后包；具体 Run 结算见 [WacomRun §10](./WacomRun.md#wacomrun-battle-settlement) |

`Initialize()` 和 `ResolveCommand()` 都以原子结果作为唯一输出 Seam。初始化在 fresh working state / event bus / referenced-assets 上执行，失败时丢弃 working data，旧 State、RNG、事件序号和资产引用均不改变；成功时作为新战斗一次性 commit，`PostSnapshot.Version == 1`，开场事件序号从 `0` 开始。`FBattleInitializationResult.Events` 只属于本次初始化，第一条成功命令的事件序号紧接其后。

`ResolveCommand()` 在 working-state / transaction event bus 上执行，失败时丢弃 working copy，保证 State、RNG、Event、Journal 和版本零变化；成功时统一 commit，且 `VersionAfter == VersionBefore + 1`。`Events / PresentationJournal / PostSnapshot` 都属于同一次命令。BattleSession 不保存等待外部拉取的事件或 journal 队列，App 和测试必须直接消费具名结果。

规则层不等待 UI 动画。BattleHUD 的 presentation queue、Combat Log、Presentation Stack 和 turn-boundary barrier 都属于表现层；它们只决定何时把玩家意图提交成 `FBattleCommand`，不改变命令本身。

`FBattlePresentationJournal` 也是 C++ only 只读表现合同，不是规则状态，也不是 WBP 制作面。它记录一次成功命令结算中的关键 checkpoint snapshot、相关卡实例 ID，以及从同次正式事件流派生的有序 Deck Steps，让 App 层后续表现计划能读取精确中间态；恢复、存档、规则判断和权威 UI 刷新仍以 `FBattleSnapshot` 为准。EndTurn checkpoint 包含 `TurnEndDiscardResolved`、`TurnEndRetainResolved`、`TurnStartDrawResolved`；`DeckSteps` 可由 EndTurn 或 `Effect.Draw` 等任意正式命令产生，Action Preview 不生成这组事实。

普通玩家命令在 `PlayerAction` 阶段提交。`PendingKnockdownChoice`、`BattleEnd` 和非玩家行动阶段会阻止普通 `PlayCard / Wait / EndTurn`。

Battle 初始化只接受 `FBattleInitParams.EnemySlots` 作为敌人入口。`UEncounterDefinition` 是 WacomData 层的静态 Encounter 合同，不由 BattleSession 直接读取；`ABattleTriggerActor` 在进入战斗前把 Encounter 敌人槽转换成 `FBattleInitParams.EnemySlots`。运行态 `EncounterId` 由场景 Trigger 的 `PersistentId` 提供，而不是 Encounter 资产 ID。

战斗抽牌堆来源只接受两级 contract：`FBattleInitParams.BattleDeckEntries` 非空时使用 entries，并保留 Run 传入的 `CapacityEffectTags`；entries 为空时回退 `Character->StarterDeck`，仅服务 direct fixture / debug fallback。旧的 definition-only `BattleDeckOverride` 入口已删除，Run 正式入口必须通过 `URunSession::BuildInitParamsForBattle()` 生成 entries。

`FBattleSnapshot.Enemies` 是敌人快照的唯一 public 入口。它按初始化 `EnemySlots` 顺序输出 `FEnemySnapshot`，每个 enemy 下再输出 `Parts`。不再提供 `FBattleSnapshot.Enemy` 或“第一个敌人”别名；UI、日志、场景目标绑定和新测试都应遍历 `Enemies`，或在明确单敌人 fixture 中显式读取 `Enemies[0]`。敌方部位长期身份以 `EncounterId + EnemySlotId + PartSlotId` 为准；`PartId` 只保留在静态内容定义和 debug 语义中，不参与运行时目标匹配。

## §3 PlayCard 与目标合同

`FBattleCommand::Type == PlayCard` 时，`CardInstanceId` 必填。目标字段由卡牌 `TargetMode` 决定：

| TargetMode | 命令字段 | 合法性 |
|---|---|---|
| `None / Self / AllEnemyParts` | 不要求额外目标 | 检查源卡在手牌、费用合法 |
| `SingleEnemyPart` | `TargetEnemyPartKey` | 目标 key 必须解析到当前战斗中未破坏的敌方部位 |
| `HandCard` | `TargetCardInstanceId` | 目标必须是另一张当前手牌；拒绝 self、无效 ID、已离开手牌的卡 |

Private `FPlayCardEvaluator` 是上述前置规则的唯一 Implementation。Target Probe 和 Preview Candidate 只读求值源卡与结构性目标，不校验阶段或费用；正式提交与 Action Preview 共享 Commit Evaluation，按 `Phase -> Command 形状 -> Source 存在 -> Hand -> Definition -> TargetMode/目标 -> Cost` 的固定顺序产生 Prepared PlayCard。目标类型不匹配时优先返回 `UnsupportedWorldTarget / UnsupportedCardTarget`，然后才进入具体 ID、self、区域和关键词资格判断；源卡已离手时优先于缺少 Definition 拒绝。

Prepared PlayCard 只保存状态版本、规范化命令、运行时费用、Anchor / Swift / Combo 和目标稳定身份，不携带可变 runtime 指针。它只能交给 `FPlayCardResolver` 执行；若 `StateVersion` 已变，会在发布事件、消耗 RNG 或修改状态前以 `InvalidState / StalePlayCardEvaluation` 拒绝。

`TargetMode=HandCard` 会把玩家选中的目标手牌作为 `Target.SelectedHandCard` 传给主效果链。基础资格由 `UCardDefinition::HandCardTargetFilter` 决定：

- 显式开启 `bUseExplicitHandCardTargetFilter` 时，`bAllowNormalHandCards / bAllowHandAnchors` 直接决定普通手牌和左右手锚点是否可选。
- 未显式开启时走兼容推断：普通 `HandCard` 默认允许普通手牌和左右手锚点；包含 `Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected + Target.SelectedHandCard` 的源卡默认只允许普通手牌。
- `RequiredTargetKeywords` 要求目标有效关键词全部命中；`BlockedTargetKeywords` 命中任意一个就拒绝。
- 目标有效关键词 = 卡牌定义关键词 + 战斗中的临时关键词。
- 左右手锚点如果被允许，也同样参与 keyword 条件。
- self target 全局禁止，不提供卡牌字段开放。

当前 `Effect.Card.AddCost / Effect.Card.ReduceCost` 可精确作用到目标手牌；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 可把选中的普通手牌移入弃牌堆 / 消耗牌堆。费用、卡牌类型、区域、伙伴 / 食物专用属性等更复杂筛选属于后续扩展方向。

`UBattleSession::ValidateTargetWithCard(CardInstanceId, TargetHandle)` 是拖拽 preview / debug 使用的只读 Target Probe 入口，返回 `FWacomBattleTargetValidationResult`。它严格回答一个具体显式目标是否与源卡兼容：`None / Self` 不接受显式对象，`AllEnemyParts` 仅接受可解析的存活敌人部位。调用方读取 `bCanTarget` 判断是否可选，并可使用 `RejectReason / DebugSummary / ResolvedPartKey` 做 UI 反馈和排查；不再保留 bool-only 兼容入口。

`UBattleSession::BuildCardTargetPreview(CardInstanceId, TargetHandle)` 是 Battle 卡牌目标预览的 public 入口。它消费 PlayCard Evaluation 生成的 Preview Candidate，再返回 `FBattleCardTargetPreview`：validation、源卡运行时费用 / 迅捷事实、规范化执行绑定、可选 Preview Focus，以及每个主效果的 preview magnitude / skip facts。该 Interface 只读，不触发事件、不修改手牌 / 敌人 / 状态，也不模拟整次 `PlayCard` 事务。

敌人部位目标的 preview 归 `WacomBattle` 计算，复用 Effect Semantics 的 target / magnitude plan、`ConditionResolver`、`MagnitudeModifiers`、武器容量伤害 +3 和伤害 clamp。App / Widget 只能把这些 facts 转成卡面和详情 ViewData，不能复制或重算战斗规则。

手牌目标的 preview 第一版只覆盖主效果摘要：`Effect.Card.AddCost / Effect.Card.ReduceCost` 预测目标卡费用变化；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected / Effect.Card.GainKeyword` 返回结构化动作事实。`BattleCardTargetPreviewBuilder` 只组装候选与 Public 输出，具体 Effect 投影和连续费用 scratch 由同一份 Effect Semantics 承担。它不执行真实移动、不写临时关键词、不触发后续事件链。`None / Self / AllEnemyParts` 的规范化命令始终清空显式目标字段；`None / Self` 不生成 Preview Focus，`AllEnemyParts` 仅把有效存活敌人部位作为单部位表现摘要的 Focus，无效 Focus 被忽略且不改变结构性合法性。全体目标聚合预览属于后续扩展。

`UBattleSession::BuildCardActionPreview(CardInstanceId, TargetHandle)` 是 Battle 数值预览入口。它内嵌 `FBattleCardTargetPreview`，并与 Target Preview 共享内部 Preview Candidate 求值；随后执行与正式提交相同的 Commit Evaluation，在复制的 `BattleState` 上把 Prepared PlayCard 交给 `PlayCardResolver`，并使用 deterministic preview adapter。因此正式提交和 Action Preview 只维护一份前置规则与一份 PlayCard Transaction 顺序。Action Preview 只有在当前阶段是 `PlayerAction`，且源卡、目标和费用等完整提交前置条件全部通过时才成立；非 `PlayerAction` 阶段仍可保留独立 Target Preview，但 `bHasPreview=false` 且不返回 projected values。

Action Preview 的 projected facts 来自副本事务完成后的 Snapshot diff。返回内容包括玩家 HP / Shield / runtime statuses、手牌顺序 / 区域 / RuntimeCost / Card Status，以及敌人部位 HP / Shield / Initiative / runtime statuses / destroyed；preview 从副本事件流中的 `EnemyPartActed` 识别所有会行动的部位，不限于鼠标当前指向的目标。会行动部位的显示先机固定为 `0`，并保留行动前的当前意图，不预测或展示行动后刷新出的下一意图。

Action Preview 只投影当前规则层已经能确定的事实。Private `BattleEffectSemanticsModule` 是 Effect handler、authoring support、目标/参数解释和 preview determinism 的唯一 Implementation；`Damage`、`Heal`、`Status.Shield`、`ApplyStatus.*`、`RemoveStatus`、`ModifyInitiative` 以及确定性的选中手牌操作通过正式 `CardEffectChain` 作用到副本。`Draw`、随机 `Discard`、三类随机 Shuffle 和未知 Effect 会由 preview adapter 在 handler 与随机选择之前跳过，并写入 `bHasUnresolvedFacts / UnresolvedEffectTypes`；该 adapter 会继续透传到 `OnDiscard` 等嵌套被动，避免随机后续漏过过滤。`Passive.Trigger.OnCompanionCount` 的随机回手在发现实际候选后整体标记为未决并停止，不执行随机插牌、手牌上限弃牌或由此产生的 `OnDiscard` 后续；这类非 Effect 直接规则可令 `bHasUnresolvedFacts=true` 而不向 legacy 命名的 `UnresolvedEffectTypes` 填入伪造 EffectTag。UI 只能消费 `FBattleCardActionPreview`，不能在 `WacomApp` 或 Widget 中重新计算伤害、护盾、先机、状态或敌人行动。

Battle world target 按 handle 上的 `EncounterId + EnemySlotId + PartSlotId` 构造 `FBattleEnemyPartKey` 并定位。稳定 key 是执行权威；`WorldTargetId`（runtime GUID）只作为表现层目标 cue / debug 的运行时校验字段。如果 runtime GUID 解析到另一个当前部位，返回 `TargetIdentityMismatch`；无法解析的 runtime GUID 不覆盖有效稳定 key。`FBattleCommand` 不再接受 runtime part GUID 或 slot 字段作为敌方目标，最终提交统一使用 `TargetEnemyPartKey`。

Target Probe 只解释“这个显式对象能不能被这张卡作用”，不校验费用、UI 状态、动画队列或命令提交时机。`FWacomBattleTargetValidationResult` 会回填 `ResolvedPartKey`，并保留 `ResolvedPartInstanceId / ResolvedPartIdentity` 作为表现 cue / debug 投影。`BuildCardActionPreview` 使用 Commit Evaluation 补齐阶段和费用等完整提交条件：费用不足时把内嵌 validation 标记为 `NotEnoughInitiative`；非 `PlayerAction` 等其它不可提交状态保留独立目标合法性，但不生成 Action Preview projected values。

## §4 战斗流程

`FBattleTurnLifecycleModule` 是回合顺序的唯一 Implementation。`BattleInitializer` 在初始敌方 phase / intent facts 后提交首回合启动，`EndTurnResolver` 只把合法 EndTurn 命令交给该 Module；两者不再分别拼接抽牌、弃牌、敌方行动或 checkpoint。

回合结构：

```text
敌人初始化 -> 起始阶段 -> 执行阶段 -> 结束阶段 -> 若战斗未结束，回到起始阶段
```

Commit Evaluation 在任何事件或状态修改前完成；`FPlayCardResolver` 只执行 Prepared PlayCard 以下顺序：

1. 发射 `CardPlayed`。
2. 执行 `ZoneHook.OnPlay`。
3. 记录所有存活敌方部位的出牌前先机。
4. 判断 `PerfectRelease` 候选。
5. 对先机命中的部位执行 `Resistance` 判定。
6. 执行卡牌主动 `Effect`。
7. 执行完美释放效果。
8. 非迅捷卡推进敌方先机，除非被 `ZoneHook.OnPerfectReleaseHit` 跳过。
9. 执行卡牌离开手牌后的去向规则，并紧接着发射携带最终 `ECardLocation` 的 `CardPlayDestinationResolved`。
10. 若本卡有 Companion 关键词，全局计数 +1。
11. 执行 `Passive.AfterPlayed`。
12. 执行 `Passive.OnCompanionCount`。
13. 消费本卡 Twilight、解除出牌前相邻卡 Freeze，并结算 `Poison`。
14. 若有部位先机 <= 0，执行敌方部位行动子流程。
15. 检查战斗结束。
16. 返回执行阶段；`BattleSession` 在整条命令成功后统一 commit 并只递增一次 `StateVersion`。

起始阶段：

```text
首回合先发 TurnStarted（后续回合不发）
-> Phase 切到 TurnStart
-> 重置 CurrentWaitValue 为 2
-> 按剩余普通手牌容量，从抽牌堆最多抽取 5 张普通卡
-> 合并上回合保留普通卡与本回合新抽普通卡
-> 重新随机编排普通卡池
-> 重新插入左右手锚点
-> 物化 Pending Hand Affliction 到当前完整手牌
-> 发 CardsDrawn / HandZoneChanged
-> Phase 切到 PlayerAction
```

结束阶段：

```text
发 TurnEnded
-> 清除所有卡牌回合级 Slow / Freeze
-> 本回合使用牌堆自然进入弃牌堆
-> EndTurn Hand Card Zone Transition 从同一份迁移前 Hand 求值 retained / discarded facts
-> 非保留普通卡全部进入弃牌堆
-> 按逆向 Hand 顺序逐卡发 CardDiscarded 并执行 OnDiscard
-> 有真实弃牌时发一个批次 HandZoneChanged，并记录 TurnEndDiscardResolved checkpoint
-> 有明确保留普通牌时发 CardsRetained，并记录 TurnEndRetainResolved checkpoint
-> 敌方行动前 early-exit 检查
-> 执行敌方部位行动子流程
-> 检查战斗结束
-> 若未结束，回到起始阶段，并在回合开始抽牌后记录 TurnStartDrawResolved checkpoint
```

等待流程：

```text
所有敌方部位 CurrentInitiative -= CurrentWaitValue
-> 若有部位先机 <= 0，执行敌方部位行动子流程
-> CurrentWaitValue += 1
-> 返回 PlayerAction
```

当前未接入通用“战斗开始时 / OnTurnStart / OnDraw”效果调用点。玩家侧手牌状态物化是明确的 typed lifecycle stage，固定在抽牌与 Hand 重建后、末尾 `HandZoneChanged` 前，不代表开放通用 callback registry。未来 `OnTurnStart` 仍只能在 `Phase=TurnStart` 后、等待值重置与抽牌前接入；`OnDraw` 必须另行定义回合抽牌与 Effect.Draw 是否共享语义。

当前未接入“回合结束时 / 持续到回合结束”效果系统。`OnTurnEnd` 继续是 Reserved 制作语义；未来固定在 `TurnEnded` 后、牌堆与手牌清理前接入，until-turn-end expiry 固定在 retain facts 后、第一道 BattleEnd gate 前接入。非保留普通卡仍发生在敌方行动前；若要调整顺序，需同步更新规则文档和 [TechDebt.md](./TechDebt.md) 中的时序债。

## §5 手牌区域规则

区域结构：

```text
[左手区] [左手牌] [双手区] [右手牌] [右手区]
```

左手牌和右手牌是手牌区域锚点，也是可打出的卡。它们自带保留，不进入抽牌堆、弃牌堆或消耗牌堆；被打出后进入 `Limbo`，下回合开始重新插回手牌队列。

抽牌和重建：

- 回合开始按剩余普通手牌容量，从抽牌堆最多抽取 5 张普通卡。
- 若普通手牌已到上限，则本次不继续抽牌；未抽出的卡保留在抽牌堆中，不进入弃牌堆，也不发 `CardsDrawn`。
- 回合开始把上回合保留普通卡和本回合新抽普通卡合成预备普通卡池。
- 预备普通卡池每回合重新随机编排，不保留上回合 index、相对顺序或区域。
- 两张锚点都有效且普通卡池非空时，两者之间至少一张普通卡。
- 两张锚点都有效但普通卡池为空时，fallback 队列为 `[LeftHand, RightHand]`。
- 战斗中途的 `Effect.Draw` 不重建整条手牌；抽到或回收的普通卡逐张随机插入当前手牌队列。
- `Effect.Draw` 同样按剩余普通手牌容量截断。正在打出的抽牌源卡会被排除出容量占用，因为它将在本次结算末尾离开手牌。
- `Effect.Draw` 从抽牌堆、弃牌堆或消耗牌堆移动卡牌时，只移动能放进普通手牌容量的数量；放不下的卡保持在原源牌堆，不会因为手牌上限转入弃牌堆。

手牌上限：

- 普通卡牌上限 10，不计算左手牌和右手牌。
- 最终手牌队列上限 12。
- 抽牌类入口不会把超出容量的卡牌抽出后弃掉，而是在移动前截断数量，未抽 / 未回收的卡保留在源牌堆。
- 回手、击倒奖励卡入手等非抽牌式直接入手仍会即时检查普通手牌上限；超出上限的普通卡牌立即移动到弃牌堆。
- 每张因普通手牌上限进入弃牌堆的卡都会产生 `HandLimitDiscarded`，并同步记录真正弃牌路径的 `CardDiscarded`。
- `HandLimitDiscardSource` 当前活跃来源主要是 `PassiveOnCompanionCount / None`；`TurnStart / EffectDraw` 保留为兼容枚举，不再表示普通抽牌达到上限后的自动弃牌。
- `HandZoneChanged` 只表示手牌区需要刷新，不承载具体弃牌语义。

`OnDiscard` 表示“本卡被弃掉”，不表示“任何进入弃牌堆”。会触发的路径包括 `Effect.Discard`、`Effect.Card.DiscardSelected`、普通手牌上限弃牌和回合结束非保留普通卡弃牌。普通打出进入本回合使用牌堆、回合结束自然转入弃牌堆、`Effect.Card.ExhaustSelected` 和 `Effect.ExhaustSelf` 不触发 `OnDiscard`。

`CardZoneAggregate` 是 runtime card 注册、Draw / Hand / Played / Discard / Exhaust / Limbo membership、`RuntimeCard.Location` 与容器内顺序的唯一写入口。跨区移动和同区重排都先验证当前权威位置，再原子更新并返回 `FCardZoneTransitionFact`；初始化完成后的 invariant 要求每张卡恰好属于一个定位容器且 Location 一致。`BattleCardZoneTransition` 消费这些 facts，统一 `Effect.Discard / Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected`、EndTurn、HandLimit、奖励和 Companion 的事件与被动。批量弃牌先完成整批状态迁移，再逐张执行 `CardDiscarded -> OnDiscard`，最后只发一次 `HandZoneChanged`；显式消耗按 `CardExhausted -> HandZoneChanged` 发布，不执行 `OnDiscard`。Draw、洗牌、PlayedPile 自然清理和 Limbo 返回由各自规则 Module 编排，但状态写入仍委托 aggregate。

腾挪会重新放置当前手牌中的普通卡，使其进入不同区域或同一区域随机位置。默认不选择左右手锚点。左手牌或右手牌离开手牌区时，双手区立刻失效；左右手牌都不在时，所有普通卡 `Zone=None`，腾挪不可用。

区域判定只看两张锚点在队列中的相对位置：较小索引左侧为 Left，中间为 Both，较大索引右侧为 Right。当前锚点不被腾挪，物理方向不会颠倒。

## §6 先机、PerfectRelease 与 Resistance

Cost 推进：

- 非迅捷且未被 ZoneHook 跳过的卡会尝试以 `RuntimeCost` 推进每个未破坏部位；敌方 Freeze 可让对应部位本次实际变化为 0 并消耗 1 层。
- 迅捷卡不扣减先机。
- 被 `ZoneHook.OnPerfectReleaseHit` 标记跳过时不扣减。
- 迅捷、ZoneHook 跳过和 `RuntimeCost=0` 都不消费 Freeze，因为没有真实推进尝试。

`PerfectRelease` 判定：

```text
出牌前记录所有存活部位的 CurrentInitiative
若 CardRuntimeCost == Part.CurrentInitiativeBeforePlay，则该 Part 命中 PerfectRelease
```

多个部位可同时命中。对于本次会真实推进先机的卡，出牌开始时已冻结的部位不产生 InitiativeHit / Resistance / PerfectRelease；当前卡新施加的 Freeze 从下一张真实推进卡开始生效。迅捷卡不触发完美释放。主效果致死的部位不参与完美释放。

`Resistance` 在先机命中时触发，且先于完美释放。卡牌抵抗值来自主效果中首个 `Effect.Damage` 的 FinalMagnitude；无伤害效果则为 0。意图抵抗值来自 `FIntentDefinition::ResistanceValue`；非攻击意图填 0。

当 `CardResistance > IntentResistance` 时，该部位进入 `Status.Stunned`。抵抗不改变伤害、不改变先机、不阻止先机推进。

UI 先机预测、scene part Status Badge 和拖卡 preview 只读取 Snapshot / validation 事实，不提交命令、不模拟完整 resolver。表现合同见 [WacomBattleUI.md](./WacomBattleUI.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) 和 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §7 状态、保留、ZoneHook 与 Passive

`Poison` 触发时机：

- 玩家每打出一张牌后，对敌我双方各结算一次当前中毒层数伤害。
- 敌方部位每行动一次后，对敌我双方各结算一次当前中毒层数伤害。

`Poison` 造成等于当前中毒层数的生命伤害，穿透护盾，层数不因结算而减少。`Effect.Heal` 治疗玩家时，移除治疗量 10% 的中毒层数，向下取整；层数为 0 时移除 `Status.Poison`。

状态不共享一个万能存储。玩家/敌方部位的持久层数存于 Combatant `StatusStacks`；单卡层数存于 `FRuntimeCardInstance.StatusStacks`；尚未选择具体卡牌的玩家侧控制存于 `PendingHandAfflictions`。每种宿主都只有一份可变真相，Snapshot 的 `Statuses` 从正层数 facts 投影。当前不实现 timed status，Effect 的 `Duration` 不创建平行状态实例。

正式状态语义：

- 敌方 Slow 是即时操作：施加 `y` 时当前意图 `CurrentInitiative += y`，不保留冗余敌方 Slow 层数。
- 玩家 Slow 在下个玩家回合抽牌并重建 Hand 后，随机不重复选择 `x` 张牌，各增加 `y` 层并计入 RuntimeCost；TurnEnd 清除。
- 敌方 Twilight 保留层数；安装下一意图基础先机后增加当前层数，再将层数变为 `floor(stacks / 2)`。
- 玩家 Twilight 在下个玩家回合物化到当前整手牌，层数计入 RuntimeCost；成功打出该牌后减半，并随卡跨 Hand / Played / Discard / Draw 持久化。
- 敌方 Freeze 每层拦截一次下一张会真实推进先机的卡，并只阻止对应部位；不再跳过敌方行动。
- 玩家 Freeze 在下个玩家回合随机不重复选择 `x` 张牌；冻结卡不能 Commit / Action Preview，打出其出牌前左或右邻牌会解除全部 Freeze，TurnEnd 仍未解除的 Freeze 清除。

玩家侧 `x` 与 `y` 分离：`FIntentEffect.HandAffliction.TargetCardCount=x`，`Magnitude=y`。Twilight 固定使用当前整手牌。选择消耗 `BattleState.Rng`，Action Preview 使用复制状态，不泄漏正式 RNG。

`Status.Shield` 是护盾数值入口，写入独立 `Shield` 字段，不进入 `StatusStacks`。Combatant 状态施加或玩家 pending 投递发布 `StatusApplied`；单卡物化、减半、解冻和回合清理发布 `CardStatusChanged`。逐部位实际先机变化发布 `EnemyInitiativeChanged`，`InitiativePushed` 只保留本次 RuntimeCost 推进尝试摘要。

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
| `OnDiscard` | 本卡真实从手牌进入弃牌堆后 | 发 `CardDiscarded` 后执行本卡 Effect Chain；PlayedPile 自然清理和 Exhaust 不触发 |

## §8 Effect Semantics、Effect Chain 与制作边界

Private `FEffectSemanticRegistry` 保存不可变 `FEffectSemanticDescriptor`，是 EffectTag、Card / Intent 支持、目标政策、MagnitudeSource、typed 参数、determinism、handler 与 Target Preview projector 的共同事实来源。descriptor 的注册按 `CombatantMutation`、`CardMovement`、`CardRuntimeMutation`、`Initiative` 四个 semantic family 分文件维护；`BattleEffectSemanticsModule` 只负责 effect chain 执行、目标展开、magnitude 和 preview 编排，不再同时承担制作支持表。`FIntentEffect` 的窄类型 `HandAffliction` 与 `FCardEffect.TargetZone` 的 legacy decode 约定保持不变。

`FCardEffectChain` 是 scratch 的词法所有者。Main 独立一条；所有匹配 OnPlay ZoneHook 共享一条；PerfectRelease 每个命中部位独立一条；AfterPlayed 的匹配 passive 共享一条；OnDiscard 每个 passive 独立一条。Handler 失败只表示当前 invocation 未应用，card chain 继续；Intent chain 也不因 handler 失败停止，只在玩家死亡时停止剩余效果。`OnTurnStart / OnTurnEnd / OnDraw` 当前没有运行时 Implementation，其 chain 生命周期在正式开放制作语义时再定义。

| EffectTag | Handler | 当前语义 |
|---|---|---|
| `Effect.Damage` | DamageHandler | 造成伤害 |
| `Effect.Heal` | HealHandler | 恢复玩家 HP，并按治疗量移除玩家中毒 |
| `Effect.ApplyStatus.Poison` | ApplyStatusHandler | 施加中毒 |
| `Effect.ApplyStatus.Slow` | ApplyStatusHandler | 敌方当前意图即时延迟；玩家创建下回合随机手牌减速 |
| `Effect.ApplyStatus.Freeze` | ApplyStatusHandler | 敌方拦截下一次卡牌推进；玩家创建下回合随机冻结卡 |
| `Effect.ApplyStatus.Twilight` | ApplyStatusHandler | 敌方延迟下一意图并减半；玩家污染下回合整手牌；继续触发 `OnTwilightTriggered` |
| `Effect.Shuffle.Random` | ShuffleHandler | 随机腾挪 |
| `Effect.Shuffle.FromBothToOther` | ShuffleHandler | 从双手区腾挪到其他区域 |
| `Effect.Shuffle.ToRandomZone` | ShuffleHandler | 腾挪到随机区域 |
| `Effect.Draw` | DrawHandler | 按普通手牌剩余容量，从 Draw / Discard / Exhaust 入手并随机插入当前手牌 |
| `Effect.Discard` | DiscardHandler | 随机弃掉普通手牌，不弃左右手锚点 |
| `Effect.ExhaustSelf` | ExhaustSelfHandler | 给源卡加临时消耗关键词，出牌去向阶段进消耗牌堆 |
| `Effect.Card.AddCost` | CostModHandler | 增加目标手牌费用 |
| `Effect.Card.ReduceCost` | CostModHandler | 降低目标手牌费用 |
| `Effect.Card.DiscardSelected` | SelectedHandCardZoneMoveHandler | 要求 `Target.SelectedHandCard`；目标普通手牌移入弃牌堆并触发 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | SelectedHandCardZoneMoveHandler | 要求 `Target.SelectedHandCard`；目标普通手牌移入消耗牌堆，不触发 `OnDiscard` |
| `Effect.GainKeyword` | GainKeywordHandler | 给目标手牌临时添加 decoded Keyword 参数 |
| `Effect.RemoveStatus` | RemoveStatusHandler | 移除目标持久 Combatant Status；不允许移除即时敌方 Slow |
| `Effect.ModifyInitiative` | ModifyInitiativeHandler | 通过 Initiative Timeline 修改目标部位当前先机 |
| `Status.Shield` | ShieldHandler | 写入护盾字段 |

效果字段和 DataAsset 契约见 [WacomData.md](./WacomData.md)，GameplayTag 字典见 [WacomGameplayTags.md](./WacomGameplayTags.md)，当前可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)。

`FWacomBattleRuleContentContract` 是 WacomEditor 可见的只读 authoring Adapter。它直接投影同一份 Private descriptor registry，并可枚举当前 Card / Intent Effect 集合供矩阵测试；它不执行规则，不让 `WacomData` 依赖 `WacomBattle`。

GameplayTag 已声明不等于已可制作。能否进入 DataAsset，以 `FWacomBattleRuleContentContract`、Data Validation 和 `WacomDataAuthoring` authoring matrix 为准。

新增 Effect 时，应在对应 semantic family 中集中完成目标、参数、MagnitudeSource、determinism、handler 和必要的 Target Preview projector，并同步 GameplayTag、测试和文档；不再修改各调用方、Target Preview Builder 或 ContentContract 的 EffectType 分支。新增 Target、MagnitudeSource、Condition 或 Passive 触发点仍需按其独立规则扩展相应 Module。

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
   -> 若部位处于晕厥，跳过当前意图并消耗 1 层
   -> 否则执行该部位当前意图
   -> 刷新该部位意图
   -> 安装新意图基础先机，并结算该部位 Twilight
   -> 结算 Poison
   -> 检查玩家失败、部位破坏、敌人死亡和 BattleEnd
```

当前意图刷新由 `EnemyIntentSelector` 负责：

- 初始化时，Battle 从 `UEnemyDefinition.DefaultBehavior` 或 `FEnemyPartSlot.BehaviorOverride` 取得行为资产，设置 `CurrentPhaseId / PreferredIntentSetId`，并刷新首个当前意图。
- `Sequence` intent set 会按 authored 顺序选择下一条可用意图；运行时 `BehaviorSequenceCursor` 表示“下一次 Sequence 选择的起始索引”，初始化选中第一条后推进到第二条，第一次敌人行动后因此选择第二条。`Weighted` 使用战斗 RNG 在有效 rule 中确定性选择；`PriorityFirst` 选择最高优先级有效 rule。
- selector condition 当前支持自身 HP 阈值、同单位任意部位 HP 阈值、部位已破坏、当前 phase、自身状态、玩家状态和冷却可用。
- 每次部位行动后，无论执行还是因晕厥跳过，都会刷新到下一条当前意图；Freeze 不参与跳过行动。
- Snapshot 暴露每个部位当前 `CurrentPhaseId / CurrentIntentSetId / CurrentIntentId`，以及当前意图的 `IntentId / DisplayName / Initiative / ResistanceValue`。
- 初始化和行动后意图刷新会发 `EnemyIntentSelected` 事件；初始化 phase 会发 `EnemyPhaseChanged` 事件。当前还没有 phase transition resolver，因此运行中 phase 变化事件只预留给后续 phase 切换规则。

晕厥以层数模型记录。每次该部位行动时，无论执行意图还是跳过意图，都消耗 1 层；层数归零时移除 `Status.Stunned`。

冻结与晕厥已经分离：`Stunned` 是唯一跳过敌方行动的控制；`Freeze` 只在卡牌先机推进阶段按部位消费。

## §10 BattleState

`BattleState` 是战斗内核的可变状态，位于 `WacomBattle/Private`，外部模块不可见。

Private `BattleCombatantMutationModule` 是运行时 HP、Shield、stack status、玩家 HP 阈值和敌方部位破坏边沿的唯一 Implementation。调用方只提交 Player / EnemyPart handle 与 typed mutation intent；Module 不接收 Operation Adapter，也不决定 Effect Chain、Poison 触发时机、击倒阶段或 Battle End。

普通伤害按 `Shield -> HP` 结算，Poison 显式选择绕过护盾。致死部位的固定顺序是：完成 HP mutation 并发布 `DamageDealt`，随后置 `bDestroyed=true / CurrentInitiative=0`，发布 `EnemyPartHpEmptied`，最后写经验、`DestroyedParts` 和 pending knockdown。`KnockdownChoiceRequested` 仍由命令管线在事务结束后发布。初始化恢复预破坏部位使用无 EventBus 的专用入口，因此不会重复发布事件、经验或击倒选择。

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
│   ├── AllCards[]（RuntimeCostModifier + Card StatusStacks）
│   ├── Hand
│   ├── DrawPile
│   ├── PlayedPile
│   ├── DiscardPile
│   ├── ExhaustPile
│   └── Limbo
├── PendingHandAfflictions[]
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
| 连击牌 | 通过 Card Zone Transaction 按出牌前邻居 / index 返回原位置；显式腾挪优先 |
| 保留牌打出 | 本回合使用牌堆；保留只影响回合结束弃牌 |

本回合使用牌堆（`PlayedPile`）只保存本回合自然打出的普通牌。抽牌请求按可追踪步骤执行：先从当前抽牌堆形成连续 `DrawBatch`；抽牌堆为空且弃牌堆非空时，形成 `DiscardPileReshuffledIntoDraw`，把弃牌堆全部移动并沿用现有 Fisher–Yates / RNG 顺序洗牌；随后再形成新的 `DrawBatch`。规则立即结算且最终顺序不变，但事件严格保持“前一批 `CardsDrawn` → `DiscardPileReshuffledIntoDraw` → 后一批 `CardsDrawn`”。洗回事件携带本次实际移动的完整 `CardInstanceIds / Count` 和该步后的 Draw / Discard 数量；初始牌库随机、手牌区 Shuffle 和 PlayedPile 清理不发该事件。`PlayedPile` 不参与同回合洗牌。玩家回合结束时，`PlayedPile` 整体自然转入弃牌堆；这个转移不发 `CardDiscarded`，也不触发 `OnDiscard`。

## §11 BattleEvent

`FBattleEvent` 是结算过程记录流，供 UI 播表现、Combat Log 和自动化测试使用。事件不是真正规则状态；恢复、刷新和权威读取仍以 `BattleState` / `FBattleSnapshot` 为准。

| 类型 | 当前用途 |
|---|---|
| `BattleStarted` | `Initialize` 成功后发出 |
| `TurnStarted` | `Initialize` 成功后首回合发出 |
| `CardsDrawn` | 回合开始抽牌或 `Effect.Draw` 成功入手 |
| `DiscardPileReshuffledIntoDraw` | 抽牌堆耗尽后，弃牌堆整体洗回抽牌堆；携带全部真实 ID、数量和步骤后牌堆计数 |
| `CardsRetained` | 玩家回合结束明确保留普通手牌 |
| `HandZoneChanged` | 手牌队列、区域或上限弃牌后需要 UI 刷新 |
| `CardPlayed` | 玩家打出卡牌 |
| `CardPlayDestinationResolved` | 本次成功打出后的最终卡区事实；紧跟本次去向解析发布，`CardDestination` 为最终 `ECardLocation` |
| `InitiativeHit` | 出牌前先机命中部位 |
| `ResistanceResolved` | 抵抗判定完成 |
| `PerfectReleaseResolved` | 完美释放效果完成 |
| `DamageDealt` | 实际扣血 |
| `StatusApplied` | 状态层数施加 |
| `CardStatusChanged` | 单卡状态物化、消费或清理；Amount=delta，Count=变更后层数 |
| `CardRuntimeCostChanged` | 单卡运行时费用修正已提交；`CardInstanceId` 为目标卡，`ActorInstanceId` 为来源卡或规则主体，`Tag` 为来源效果，`Amount` 为本次原始 modifier delta，`Count` 为变更后的有效 `RuntimeCost` |
| `EnemyInitiativeChanged` | 单个敌方部位实际先机变化或 Freeze 抑制事实 |
| `InitiativePushed` | 非迅捷卡的 RuntimeCost 推进尝试摘要 |
| `WaitPerformed` | 玩家等待 |
| `EnemyPartActed` | 敌方部位行动 |
| `EnemyPartHpEmptied` | 部位 HP 归零并进入破坏态 |
| `KnockdownChoiceRequested` | 击倒事件等待玩家选择 |
| `KnockdownChoiceMade` | 玩家完成击倒选择 |
| `TurnEnded` | 玩家结束回合 |
| `PassiveTriggered` | 被动触发通知；部分被动只发事件不改数值 |
| `HandLimitDiscarded` | 普通手牌上限导致某张卡弃掉 |
| `CardDiscarded` | 卡牌因弃牌规则从手牌进入弃牌堆；同一迁移批次的逐张事件共享完整有序 `CardInstanceIds`、稳定 `HandCardZoneMoveBatchSequence` 与批次后 `DiscardPileCountAfter`，逐张事件和 `OnDiscard` 顺序不变 |
| `CardExhausted` | 卡牌因消耗规则从手牌进入消耗牌堆 |
| `CardGained` | 战斗中获得新卡 |
| `BattleEnded` | 战斗进入结束态 |

`DamageDealt.Amount` 是本次实际 HP 损失，不是进入伤害流程的名义数值：护盾完全吸收时仍发布事件但 `Amount=0`；部分吸收只记录穿盾后的 HP 损失；overkill 只记录目标受击前剩余 HP。普通卡牌伤害继续填写 `CardInstanceId`，Poison 继续填写 `Tag=Status.Poison`，敌方意图不伪造来源字段。Combat Log、伤害表现 cue 和 Action Preview 都消费这一口径。

App 层的世界伤害像素反馈只读取上述 `DamageDealt.Amount`，并按平方根映射视觉强度；不会把粒子强度、Cue 时长或表现 Seed 写回 BattleState。`FWacomBattlePresentationTargetCue.Seed` 是 App presentation 的稳定装饰随机合同：Damage 由事件 Sequence、目标稳定部位 key 和 Amount 构造，TargetConfirmed 由来源卡实例和目标稳定部位 key 构造；不得复用为规则 RNG、伤害方向或存档事实。`EnemyPartHpEmptied` 仍只占用 Destroyed 高优先级 Cue，本轮不新增规则事件。

伤害导致部位破坏时，事件顺序固定为 `DamageDealt -> EnemyPartHpEmptied`；击倒请求由命令管线稍后追加。`StatusApplied.Amount` 仍表示本次新增层数，普通 Effect ApplyStatus 保持空来源卡，Resistance Stun 保持 `ResistanceResolved -> StatusApplied` 并携带来源卡。Combatant 状态移除继续静默；Card Status 的物化、减半、解冻与清理统一使用 `CardStatusChanged`。

`CardRuntimeCostChanged` 是“费用 modifier 已成功写入”的显式规则事实，不替代 Snapshot。Clamp 后可见费用可能没有变化，因此 App 只有在同时看到该事件（或确实可能改费的 `CardStatusChanged`）以及前后已展示 Snapshot 的 `RuntimeCost` 差异时，才播放费用重写反馈。普通 Snapshot 刷新、Target Preview 和取消 Preview 不得从数值差异单独反推这项语义；Target Preview 仍可显示预测后的费用，因此正式重写 Hint 必须保存前后 Snapshot 的权威费用对，不能依赖提交时屏幕上恰好显示的 Sprite。

`CardsDrawn` 的公共合同是“本批真实入手卡实例列表”：`CardInstanceIds` 按规则抽取 / 从弃牌堆或消耗牌堆移入手牌的顺序记录卡实例 ID，`Count` 始终等于 `CardInstanceIds.Num()`，仅作为旧 debug / 测试读取的兼容计数字段。因普通手牌容量不足而未移动的卡不会进入 `CardInstanceIds`，也不会触发 `CardsDrawn`。单卡事件继续使用 `CardInstanceId`，批量抽牌不要让 UI 再从前后 `FBattleSnapshot` 差异猜测抽到的是哪几张牌。

`CardsRetained` 的公共合同是“本次回合结束明确保留的普通手牌实例列表”：`CardInstanceIds` 只包含普通手牌，不包含左手 / 右手锚点，`Count` 始终等于 `CardInstanceIds.Num()`。它只提供保留事实，不改变保留规则本身，也不绑定具体 first-person transition。

EndTurn presentation journal 的 checkpoint 生成时机只由 Turn Lifecycle 维护。`TurnEndDiscardResolved.CardInstanceIds` 是本阶段因回合结束进入弃牌堆的普通手牌；其 range 包含全部 `CardDiscarded`、嵌套 `OnDiscard` 与最终批次 `HandZoneChanged`，Snapshot 位于完整被动链后。`TurnEndRetainResolved` 的 range 只包含 `CardsRetained`；`TurnStartDrawResolved` 的 Snapshot 已处于下一回合 `PlayerAction`，range 包含 `CardsDrawn`、Pending Hand Affliction 产生的 `CardStatusChanged` 与末尾 `HandZoneChanged`。若某阶段没有相关 ID，或战斗在敌方行动前 / 后结束，则不生成对应 checkpoint。

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
| `DestroyedParts[]` | `BattleState.DestroyedParts` | 内部 identity 投影，用于 runtime 汇总和 debug；Battle 外规则真相使用 `DestroyedPartKeys[]`。外部消费者需要计数时使用 packet 的 key-first 入口，只有旧数据 / 手写 packet 缺有效 key 时才 fallback 到该投影 |

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
