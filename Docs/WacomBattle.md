---
type: domain-spec
scope: wacom-battle
status: active
updated: 2026-06-02
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
> UI 表现见 [[WacomUI]]，静态数据字段见 [[WacomData]]，战后 Run 结算见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement)，后续方向见 [Roadmap: 战斗规则](./Roadmap.md#roadmap-battle-rules)。

## §1 模块职责

WacomBattle 是**战斗内核**，负责单场战斗的唯一规则真相。

**负责**：
- 战斗生命周期管理
- 命令结算（PlayCard / Wait / EndTurn）
- 手牌区域规则
- 卡牌效果执行
- 敌方部位行动
- 快照、事件和战后包输出

**不负责**：
- UI 展示
- Run 探索
- 关卡交互
- 存档
- 静态资产生成和内容校验

**边界约束**：UI、Actor 和测试入口都不应该直接改 `BattleState`。它们只能提交 `FBattleCommand`，读取 `FBattleSnapshot` / `FBattleEvent` / `FBattleResultPacket`。

---

## §2 核心对象

| 对象 | 职责 |
|---|---|
| `UBattleSession` | 一场战斗的 Public facade（对外唯一入口，只持有状态、事件并委托规则 helper）|
| `BattleState` | 当前战斗状态（Private，外部不可见）|
| `BattleInitializer` | 初始化规则 helper（Private）：灌入参数、创建运行时卡牌 / 部位、应用预破坏部位并启动首回合 |
| `BattleCommandPipeline` | 命令外壳 helper（Private）：统一拦截 BattleEnd、分派 Resolver、维护版本兜底和首次击倒请求 |
| `KnockdownFlowService` | 击倒请求流 helper（Private）：构造并发射 `KnockdownChoiceRequested` 事件 |
| `BattleResultPacketBuilder` | 战后包 helper（Private）：从 `BattleState` 拷贝战后字段并派生 `bWithdrawn` |
| `BattleResolver` | 统一结算命令的调度器 |
| `BattleTurnFlow` | 回合开始流程：抽牌、重建手牌、执行手牌上限 |
| `FBattleSnapshot` | 给 UI 读取的只读状态快照 |
| `FBattleEvent` | 给 UI、日志和测试读取的事件流 |
| `FBattleResultPacket` | 战斗结束后给 Run 层消费的战后包 |
| `FBattleCommand` | 玩家或系统输入的操作命令 |
| `CardEffectDispatcher` | 卡牌效果分发（Target 映射 + 条件评估 + Magnitude 计算 + 执行）|
| `EffectExecutor` | 效果注册制执行器（按 EffectTag → Handler）|
| `MagnitudeResolver` | Magnitude 计算注册制（Literal / RuntimeCost / 扩展）|
| `ConditionResolver` | 效果/被动条件评估注册制 |
| `InitiativeResolver` | 先机命中 / 抵抗 / 完美释放 |
| `ZoneHookResolver` | ZoneHook 消费（OnPlay / OnPerfectReleaseHit）|
| `PassiveDispatcher` | 被动触发调度（AfterPlayed / OnCompanionCount / OnTwilightTriggered）|
| `PoisonResolver` | 中毒结算 |
| `HandZoneService` | 手牌区域和腾挪规则 |
| `EnemyPartActionResolver` | 敌方部位行动子流程 |
| `KnockdownChoiceAvailability` | 击倒三选一可用性计算 |
| `BattleCardGrantService` | 战斗内获得新卡并插入手牌 |

### Public Contract

| 契约 | 当前用途 |
|---|---|
| `UBattleSession::Initialize()` | 根据 `FBattleInitParams` 创建单场战斗，发 `BattleStarted / TurnStarted`，并进入首个玩家回合 |
| `UBattleSession::SubmitCommand()` | 唯一命令入口：PlayCard、Wait、EndTurn、KnockdownChoice |
| `UBattleSession::BuildSnapshot()` | 输出当前只读状态；UI 和测试读取，不作为事件历史 |
| `UBattleSession::ConsumeEvents()` | 输出并清空自上次消费以来的事件流 |
| `UBattleSession::BuildPendingKnockdownChoiceView()` | 输出当前击倒选择 ViewData；UI 不再解析事件 `Count` 位掩码 |
| `UBattleSession::BuildResultPacket()` | 战斗结束后输出战后包；具体 Run 结算见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement) |

`SubmitCommand()` 当前是同步规则结算入口：命令成功后 `BattleState`、Snapshot 和事件列表立即更新。V0-CL 后，UI 层的战斗事件表现队列和 `BattlePresentationStack` 只是视觉播放 backlog，不参与规则结算。普通 PlayerAction 阶段可以在旧卡牌表现仍播放时继续提交 `PlayCard`；新卡牌会追加到表现栈底部，最早待完成表现的卡留在栈顶。`Wait / EndTurn` 是 UI 层回合边界屏障：如果表现栈仍有卡，HUD 先记录 pending 命令并锁住继续出牌、目标选择和 first-person drop，等栈和队列清空后重新读取 Snapshot，仍处于 `PlayerAction` 才提交对应命令。`PendingKnockdownChoice`、`BattleEnd` 和非玩家行动阶段仍会阻止普通玩家行动命令。

### PlayCard 目标合同

`FBattleCommand` 仍是唯一战斗命令入口。`Type == PlayCard` 时，`CardInstanceId` 必填，目标字段按卡牌 `TargetMode` 填写：

| TargetMode | 命令字段 | 合法性 |
|---|---|---|
| `None / Self / AllEnemyParts` | 不要求额外目标 | 只检查源卡在手牌、费用合法 |
| `SingleEnemyPart` | `TargetPartInstanceId` | 目标必须是当前战斗中未破坏的敌方部位 |
| `HandCard` | `TargetCardInstanceId` | 目标必须是另一张当前手牌；拒绝 self、无效 ID、已离开手牌的卡；普通手牌 / 左右手锚点资格由 `UCardDefinition::HandCardTargetFilter` 或兼容推断决定 |

`TargetMode=HandCard` 的主动打牌会把玩家选中的目标手牌作为 `Target.SelectedHandCard` 传给主效果链。V0-AH/V0-AI 后，“哪些手牌可被选中”的基础资格收口到卡定义上的 `FWacomHandCardTargetFilter`：

- 显式开启 `bUseExplicitHandCardTargetFilter` 时，`bAllowNormalHandCards / bAllowHandAnchors` 直接决定普通手牌和左右手锚点是否可选
- 未显式开启时走兼容推断：普通 `HandCard` 默认允许普通手牌和左右手锚点；包含 `Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected + Target.SelectedHandCard` 的源卡默认只允许普通手牌
- `RequiredTargetKeywords` 要求目标有效关键词全部命中；`BlockedTargetKeywords` 命中任意一个就拒绝。目标有效关键词 = 卡牌定义关键词 + 战斗中的临时关键词
- 左右手锚点如果被 `bAllowHandAnchors` 允许，也同样参与 keyword 条件
- self target 继续全局禁止，不提供卡牌字段开放
- `ValidateTargetWithCard()` 与 `PlayCardResolver` 共用同一套 eligibility helper，保证 preview 和真正提交一致

当前已验证 `Effect.Card.AddCost / Effect.Card.ReduceCost` 可以精确作用到该目标，且测试卡显式允许普通手牌和左右手锚点；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 可以精确把选中的普通手牌移入弃牌堆 / 消耗区，测试卡显式拒绝左右手锚点。Keyword filter 已可用于“只作用伙伴”“不能作用武器”等第一版卡对卡筛选；费用、卡牌类型、区域、伙伴 / 食物专用属性等更复杂筛选留后续扩展。

**目录结构**：

```
WacomBattle/
├── Public/
│   ├── Session/BattleSession.h
│   ├── Commands/BattleCommand.h
│   ├── Snapshots/{BattleSnapshot.h, HandSnapshot.h, EnemySnapshot.h}
│   ├── Events/BattleEvent.h
│   └── Runtime/{RuntimeCardInstance.h, RuntimeEnemyPart.h, RuntimeStatus.h}
└── Private/
    ├── Session/, Core/, Commands/, Deck/, Hand/
    ├── Enemy/, Status/, Events/, Snapshots/
    ├── Effects/, Resolution/, Passives/
    └── ...
```

---

## §3 战斗流程

### 回合结构

```
敌人初始化 → 起始阶段 → 执行阶段 → 结束阶段 → 若战斗未结束，回到起始阶段
```

### 16 步流程概要（PlayCardResolver::Resolve）

1. 发射 `CardPlayed` 事件
2. 执行 ZoneHook（OnPlay）：若本卡在某区域且该区域有 OnPlay Hook，执行 Hook 的 ExtraEffects
3. 记录所有存活敌方部位的出牌前当前先机
4. 判断先机命中的部位（RuntimeCost == 部位出牌前先机）
5. 对先机命中的部位执行抵抗判定（先于完美释放）
6. 执行卡牌主动效果（伤害可能使部位 HP 归零，立即进入破坏态）
7. 执行完美释放效果（迅捷卡跳过；主效果致死的部位不参与）
8. 先机推进：非迅捷卡且未被 ZoneHook（OnPerfectReleaseHit）跳过时，所有未破坏部位当前先机 -= RuntimeCost
9. 执行卡牌离开手牌后的去向规则
10. Companion 计数累加：若本卡有 Companion 关键字，全局计数 +1
11. 执行 AfterPlayed 被动（如烁光蝶自腾挪）
12. 执行 OnCompanionCount 被动（如拂晓飞蛾回手）
13. 中毒结算：对敌我双方各结算一次
14. 若有部位当前先机 ≤ 0，执行敌方部位行动子流程
15. 检查战斗结束（敌人全破 / 玩家 HP ≤ 0）
16. 若未结束，递增 StateVersion，返回执行阶段

### 起始阶段流程

```
起始阶段开始
→ 当前等待值重置为 2
→ 从抽牌堆抽取 5 张普通卡
→ 合并上回合保留普通卡与本回合新抽普通卡
→ 重新随机编排普通卡池
→ 重新插入左右手锚点
→ 超出普通手牌上限的普通卡牌移至弃牌区
→ 发 CardsDrawn / HandLimitDiscarded / CardDiscarded / HandZoneChanged
→ Phase 切到 PlayerAction
→ 起始阶段结束
```

当前没有接入“战斗开始时 / 回合开始时 / 持续到回合开始”这类效果调用点。`PassiveDispatcher::RunOnTurnStart` 方法存在，但主流程未调用；后续触发点扩展见 [Roadmap: 被动触发点扩展](./Roadmap.md#roadmap-battle-rules)。

### 结束阶段流程

```
结束阶段开始
→ 发 TurnEnded
→ 非保留普通卡进入弃牌区
→ 若弃牌导致手牌变化，发 HandZoneChanged
→ 判断敌方全部或我方生命值归零（敌方行动前 early-exit）
→ 执行敌方部位行动子流程（所有存活且可行动的部位）
→ 判断战斗结束
→ 若未结束，回到起始阶段
```

当前没有接入“回合结束时 / 持续到回合结束”效果系统；非保留普通卡弃牌暂放在结束阶段前半段执行。若策划要求它在敌方行动之后发生，需要调整 `FEndTurnResolver` 时序，见 [TechDebt.md](./TechDebt.md) 的回合结束保留 / 弃牌时序条目。

### 等待流程

```
执行等待
→ 所有敌人部位当前先机减少当前等待值
→ 判断是否有部位先机 ≤ 0
→ 若有，执行敌方部位行动子流程
→ 当前等待值 +1
→ 返回执行阶段
```

---

## §4 手牌区域规则

### 区域结构

```
[左手区] [左手牌] [双手区] [右手牌] [右手区]
```

左手牌和右手牌是手牌区域锚点。普通卡牌的位置根据它们相对左右手牌的位置，被判定为左手区、双手区或右手区。

### 左手牌与右手牌

- 是卡牌，可以被打出
- 是手牌区域锚点
- 自带保留效果
- 不会进入抽牌堆、弃牌堆、消耗区
- 被打出后本回合离开手牌，不进入任何卡牌区域（Limbo）
- 下个回合开始时，只要 `LeftHandInstanceId / RightHandInstanceId` 仍有效，左右手锚点会重新插回手牌队列；当前实现不要求它们上回合结束时仍在 Hand

### 抽牌规则

- 回合开始固定从抽牌堆抽取 5 张普通卡牌
- 回合开始统一生成预备普通卡池：上回合保留在手牌中的普通卡 + 本回合新抽普通卡
- 预备普通卡池每回合重新随机编排，不保留上回合 index、相对顺序或区域
- 然后重新插入有效左右手牌；两张左右手牌都有效且普通卡池非空时，两者之间至少一张普通卡牌
- 两张左右手牌都有效但普通卡池为空时，fallback 队列为 `[LeftHand, RightHand]`，无法保证两者之间有普通卡
- 只有一张有效锚点时，只插入该锚点；没有有效锚点时只保留普通卡队列
- 战斗中途的 `Effect.Draw` 不重建整条手牌；抽到或回收的普通卡逐张随机插入当前手牌队列，并立即执行普通卡手牌上限
- `Effect.Draw` 触发普通手牌上限时，正在打出的抽牌源卡会被排除在本次上限弃牌候选外

### 手牌上限

- 普通卡牌上限 10，不计算左手牌和右手牌
- 最终手牌队列上限 12
- 超出上限的普通卡牌立即移动到弃牌区；中途抽牌/回手也会即时检查
- 每张因普通手牌上限进入弃牌区的卡都会产生一条 `HandLimitDiscarded` 事件；这是 UI/日志表现事件，不改变战斗规则真相
- `HandLimitDiscarded.CardInstanceId` 是被上限弃掉的卡；`ActorInstanceId` 只在 `EffectDraw` 来源时填写触发抽牌的源卡；`HandLimitDiscardSource` 区分 `TurnStart / EffectDraw / PassiveOnCompanionCount / None`
- 击倒奖励卡触发上限弃牌时，当前 `HandLimitDiscardSource=None`
- `CardDiscarded` 同步记录所有真正弃牌路径；手牌上限路径会同时保留 `HandLimitDiscarded` 兼容表现事件
- `HandZoneChanged` 仍只表示手牌区需要刷新，不承载具体哪张牌因上限被弃掉的语义
- 击倒奖励卡进入手牌后同样立即执行普通卡手牌上限；若因此弃牌，会按逐张 `HandLimitDiscarded` 事件通知表现层

### OnDiscard 与手牌区移动事件

- `Passive.Trigger.OnDiscard` 表示“本卡被弃掉”，不表示“任何进入弃牌堆”
- 会触发 `OnDiscard` 的路径：`Effect.Discard`、`Effect.Card.DiscardSelected`、普通手牌上限弃牌、回合结束非保留普通卡弃牌
- 不会触发 `OnDiscard` 的路径：打出后自然进入弃牌堆、`Effect.Card.ExhaustSelected`、`Effect.ExhaustSelf` 导致的打出后消耗
- 弃牌状态移动先完成，再发 `CardDiscarded`，随后触发该卡自身的 `OnDiscard` 被动，最后按批次发一次 `HandZoneChanged`
- 消耗状态移动先完成，再发 `CardExhausted` 和一次 `HandZoneChanged`；消耗不触发 `OnDiscard`

### 交互目标校验

- `UBattleSession::ValidateTargetWithCard(CardInstanceId, TargetHandle)` 是拖拽 preview / debug 使用的只读校验入口，返回 `FWacomBattleTargetValidationResult`
- `CanTargetWithCard()` 保留为 bool 入口，内部转调 validation，确保 UI preview 和旧调用点共享同一套结构性合法性判断
- Card target 校验会区分：同源卡目标、源卡不是 `TargetMode=HandCard`、目标 id 无效、目标不在手牌、手牌目标 filter 拒绝普通手牌、手牌目标 filter 拒绝左右手锚点、缺少 required keyword、命中 blocked keyword
- World target 校验会区分：目标 id 无效、敌方部位不存在或已破坏、源卡 target mode 不支持 world target
- Validation 只解释“这个目标能不能被这张卡作用”，不校验费用、UI 状态、动画队列或命令提交时机；最终提交仍由 `PlayCardResolver` 再校验

### 腾挪

- 重新放置当前手牌中的卡牌，使其进入不同区域或在同一区域内改变位置
- 默认范围：当前手牌中的普通卡牌（不选择锚点）
- 腾挪到目标区域时，插入随机位置
- 随机目标区域等于当前区域时仍发生区域内重新放置
- "从双手区腾挪到其他区域"：目标排除双手区

### 双手区失效

- 左手牌或右手牌离开手牌区时，双手区立刻失效
- 原双手区卡牌仍在手牌中，Zone = None
- 腾挪可用区域缩减为仅 Left
- 左右手牌都不在时，所有普通卡牌 Zone = None，腾挪不可用

### 单锚点语义

实现按"两锚点在队列中的位置"切三段，不区分哪个是 LeftHandInstanceId 哪个是 RightHandInstanceId。即"位置较小的锚点左侧 = Left 区，中间 = Both 区，位置较大的锚点右侧 = Right 区"。第一阶段锚点不被腾挪，物理方向不会颠倒。

---

## §5 先机与完美释放

### Cost 扣减

- 非迅捷卡打出后，所有未破坏部位当前先机 -= RuntimeCost
- 迅捷卡不扣减先机
- 被 ZoneHook（OnPerfectReleaseHit）标记跳过时不扣减

### 完美释放判定

```
出牌前记录所有存活部位的 CurrentInitiative
若 CardRuntimeCost == Part.CurrentInitiativeBeforePlay → 该 Part 命中 PerfectRelease
```

- 多个部位可同时命中
- 迅捷卡不触发完美释放
- 主效果致死的部位不参与完美释放

### 抵抗

- 先机命中时触发（先于完美释放）
- 卡牌抵抗值：主效果中首个 `Effect.Damage` 的 FinalMagnitude；无伤害效果则为 0
- 意图抵抗值：`FIntentDefinition::ResistanceValue`；非攻击意图填 0
- `if CardResistance > IntentResistance → 该部位进入 Status.Stunned`
- 抵抗不改变伤害、不改变先机、不阻止先机推进

### UI 先机预测口径

- V0-CQ 后，场景 `AWacomBattleEnemyPartActor` 会通过只读预测 Widget 显示 `FEnemyPartSnapshot.CurrentInitiative` 和当前源卡的 `FHandCardSnapshot.RuntimeCost / bIsSwift`。这些是 UI 预测输入，不是规则来源。
- 普通 hover 没有源卡时显示当前先机；TargetSelect hover 或 first-person 拖卡 preview 命中部位时，非迅捷卡显示 `CurrentInitiative -> CurrentInitiative - RuntimeCost`，迅捷卡显示先机不变。
- 若非迅捷卡 `RuntimeCost == CurrentInitiative`，UI 标记先机命中 / 完美释放候选；若非迅捷卡预计结果 `<= 0`，UI 标记部位行动风险。
- V1 不模拟主效果、ZoneHook、破坏、中毒、改先机、抵抗最终结果或任何后续被动。最终是否合法和实际结果仍以 `UBattleSession` resolver 提交结果为准。
- V0-CS 后，场景 `AWacomBattleEnemyPartActor` 还会通过只读 Status Badge 常驻显示当前 `FEnemyPartSnapshot` 事实：部位名、HP/MaxHP、护盾、状态层数摘要、`CurrentInitiative`、当前意图和破坏态。Status Badge 只读 snapshot，不提交命令、不修改状态，也不参与预测计算；预测 Widget 仍只在 hover、TargetSelect hover 或 first-person 拖卡 preview 时临时显示。

---

## §6 中毒

### 触发时机

- 玩家每打出一张牌后，对敌我双方各结算一次当前中毒层数伤害
- 敌方部位每行动一次后，对敌我双方各结算一次当前中毒层数伤害

### 结算方式

- 对拥有中毒的一方造成等于当前中毒层数的伤害
- **穿透护盾**，直接扣除生命值
- 层数不因结算而减少（持续存在直到被治疗或其他效果移除）

### 层数衰减

- `Effect.Heal` 治疗玩家时，移除治疗量 10% 的中毒层数（向下取整）
- 移除后层数为 0 时移除 `Status.Poison`
- 第一阶段仅玩家治疗路径接入；敌方部位治疗、状态清除等更复杂路径待规则确认

### 归属

- 敌人有多个部位时，每个部位可以各自拥有中毒
- 玩家本体也可以拥有中毒

---

## §7 保留

### Retain 标签

- 拥有 `Card.Keyword.Retain` 的卡牌回合结束时不弃掉
- 左手牌和右手牌自带保留

### 区域条件

- 虫妹的双手区可以保留手牌：当回合结束时左右手牌都仍在手牌区，双手区中的普通卡牌可以保留到下一回合
- 保留只保留卡牌仍在下一回合的手牌池中，不保留 index、相对顺序或区域；下回合开始会与新抽卡一起重新编排位置
- 保留牌如果被主动打出，仍按普通打出处理（进入弃牌区），除非卡牌文本另有说明

---

## §8 ZoneHook

### OnPlay

- 触发时机：本卡打出时（CardPlayed 事件之后、"记录出牌前先机"之前）
- 若本卡在某区域且该区域有 OnPlay Hook，执行 Hook 的 ExtraEffects
- 典型卡：朝光暮蝶右手区（费用转移）

### OnPerfectReleaseHit

- 触发时机：本卡完美释放命中时
- 不执行 ExtraEffects，仅作为"跳过先机推进"标记
- 典型卡：朝光暮蝶左手区（不推先机）

### 触发顺序

1. `OnPlay` 发生在 `CardPlayed` 事件之后、"记录出牌前先机"之前
2. `OnPerfectReleaseHit` 的判定紧跟在抵抗判定之后，作为先机推进的 skip 条件
3. 不同区域的 Hook 不会同时命中（卡只在一个区域）

---

## §9 被动系统

### OnCompanionCount

- 触发时机：全局 Companion 计数达阈值
- 典型卡：拂晓飞蛾（阈值 3，回手）
- 触发后计数清零
- 当前实现：触发时随机插入当前手牌，并立即执行普通卡手牌上限；超限卡进入弃牌堆

### OnTwilightTriggered

- 触发时机：暮气施加成功时
- 典型卡：暮蛉（使一张中毒卡牌效果 +1）
- 第一阶段只发 `PassiveTriggered` 事件，不改 Magnitude

### AfterPlayed

- 触发时机：本卡打出完成后（卡牌去向之后）
- 典型卡：烁光蝶（自腾挪到随机区域）

---

## §10 效果执行器

### Registry 模式

效果执行器按 `EffectTag → Handler` 注册制分派。新增效果类型只需注册新 Handler，不改调度逻辑。

### EffectTag → Handler 清单

| EffectTag | Handler | 现状 |
|---|---|---|
| `Effect.Damage` | DamageHandler | 已实现 |
| `Effect.Heal` | HealHandler | 已实现（恢复玩家 HP，并移除治疗量 10% 的中毒层数，向下取整） |
| `Effect.ApplyStatus.Poison` | ApplyStatusHandler | 已实现 |
| `Effect.ApplyStatus.Slow` | ApplyStatusHandler | 占位（仅记录层数）|
| `Effect.ApplyStatus.Freeze` | ApplyStatusHandler | 占位（行动时跳过）|
| `Effect.ApplyStatus.Twilight` | ApplyStatusHandler | 占位（仅记录层数）|
| `Effect.Shuffle.Random` | ShuffleHandler | 已实现 |
| `Effect.Shuffle.FromBothToOther` | ShuffleHandler | 已实现 |
| `Effect.Shuffle.ToRandomZone` | ShuffleHandler | 已实现 |
| `Effect.Draw` | DrawHandler | 已实现；从 Draw/Discard/Exhaust 入手并随机插入当前手牌，不支持直接抽到指定 HandZone |
| `Effect.Discard` | DiscardHandler | 已实现；随机弃掉手牌中的普通卡，不弃左右手锚点 |
| `Effect.ExhaustSelf` | ExhaustSelfHandler | 已实现；给源卡加临时消耗关键词，出牌去向阶段进消耗区 |
| `Effect.Card.AddCost` | CostModHandler | 已实现 |
| `Effect.Card.ReduceCost` | CostModHandler | 已实现 |
| `Effect.Card.DiscardSelected` | SelectedHandCardZoneMoveHandler | 已实现；要求 `Target.SelectedHandCard`，目标必须是普通手牌，成功后移入弃牌堆，发 `CardDiscarded / HandZoneChanged` 并触发目标卡 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | SelectedHandCardZoneMoveHandler | 已实现；要求 `Target.SelectedHandCard`，目标必须是普通手牌，成功后移入消耗区并发 `CardExhausted / HandZoneChanged`，不触发 `OnDiscard` |
| `Effect.GainKeyword` | GainKeywordHandler | 已实现；给目标手牌临时添加 `MetaTag` 指定关键词 |
| `Effect.RemoveStatus` | RemoveStatusHandler | 已实现；移除目标 `MetaTag` 指定状态的若干层 |
| `Effect.ModifyInitiative` | ModifyInitiativeHandler | 已实现；直接修改目标部位当前先机 |
| `Status.Shield` | ShieldHandler | 已实现 |

效果字段、Target、Magnitude 和 GameplayTag 的静态数据契约见 [WacomData.md](./WacomData.md)。

V0-CU 后，战斗内容制作口径由 `FWacomBattleRuleContentContract` 提供给编辑器校验使用。它只是当前 resolver / dispatcher 的只读 authoring matrix，不执行规则、不改变 `UBattleSession::SubmitCommand()` 同步结算，也不让 `WacomData` 依赖 `WacomBattle`。新增 Effect、Target、MagnitudeSource、Condition 或 Passive 触发点时，应同时更新运行时 resolver、`FWacomBattleRuleContentContract`、数据校验测试和 [WacomData.md](./WacomData.md) 的矩阵。

重要边界：
- GameplayTag 已声明不等于已可制作；能否进入 DataAsset 以 `FWacomBattleRuleContentContract` 和 `WacomData` authoring matrix 为准。
- `Status.Shield` 是护盾数值入口，直接写入 `Shield` 字段，不进入 `StatusStacks`；不要用于 `Condition.Target.HasStatus` 或 `Magnitude.Source.TargetStatusStacks`。
- 敌方 Intent V1 只允许 `Target.Player` 和 `Target.Self`，不支持手牌目标、全体敌方部位目标或卡牌专用效果。

---

## §11 敌方部位行动子流程

### 触发来源

- 等待使部位先机 ≤ 0
- 打牌推进先机后部位先机 ≤ 0
- 结束阶段（所有存活且可行动的部位）
- 其他卡牌/状态/事件强制行动

### 子流程

```
收集本次需要行动的部位
→ 移除已破坏部位
→ 移除不能行动部位
→ 按部位顺序逐个结算：
    → 若部位处于晕厥，执行晕厥处理并跳过当前意图
    → 否则执行该部位当前意图
    → 刷新该部位意图
    → 将当前先机设置为新意图先机
    → 中毒结算（对敌我双方各结算一次）
    → 检查玩家失败、部位破坏、敌人死亡、战斗结束
```

### 晕厥处理

- 晕厥以层数模型记录
- 每次该部位行动（无论执行意图还是因晕厥跳过意图）都消耗 1 层
- 层数归零时晕厥状态移除
- 冻结状态第一阶段共享同一"跳过意图 + 消耗 1 层"分支

### 行动后刷新

无论部位是执行意图还是因晕厥跳过意图，只要完成本次行动结算，就刷新到下一个意图，并将当前先机设置为新意图的先机值。

---

<a id="wacombattle-result"></a>
## §12 战斗结束 / 同归于尽

### 战斗结束判定

`FBattleRules::CheckAndApplyBattleEnd` 在每次状态变更后被调用：

- 敌方部位全死 + 玩家 HP > 0 → Outcome = **Victory**
- 敌方部位全死 + 玩家 HP = 0 → Outcome = **Victory**，置 `BattleState.bMutualDestruction = true`（同归于尽，GDD §9.2）
- 敌方未全死 + 玩家 HP = 0 → Outcome = **Defeat**

任一情况都把 `Phase` 切到 `BattleEnd` 并发射 `BattleEnded` 事件。

击倒事件优先级：如果敌方全破、玩家未死亡，但 `PendingKnockdownEvents` 仍非空，`CheckAndApplyBattleEnd` 不会立刻进入 `BattleEnd`。玩家必须先处理队列中的击倒三选一；最后一个存活部位被击倒时，撤离不可选，只能选择 Aid 或 Destroy 后再进入 Victory。玩家死亡路径例外，失败优先级高于待处理击倒事件。

### 战内 → 战外回传

战斗结束时 `UBattleSession::BuildResultPacket()` 委托 Private `BattleResultPacketBuilder` 构造 `FBattleResultPacket` 给 Run 层：

| Packet 字段 | Battle 来源 | 说明 |
|---|---|---|
| `Outcome` | `BattleState.Outcome` | Victory / Defeat / Undetermined |
| `bCrossedHighHpThreshold` | `BattleState.bCrossedHighHpThreshold` | 玩家 HP 首次低于 High 阈值 |
| `bCrossedLowHpThreshold` | `BattleState.bCrossedLowHpThreshold` | 玩家 HP 首次低于 Low 阈值 |
| `bMutualDestruction` | `BattleState.bMutualDestruction` | 玩家 HP=0 与敌方全破同时发生；Outcome 仍是 Victory |
| `bWithdrawn` | `KnockdownChoices` 含 Withdraw | 撤离以 `Outcome=Victory + bWithdrawn=true` 表达 |
| `KnockdownExpGains[]` | `BattleState.PendingKnockdownExpGains` | 部位破坏经验记账 |
| `KnockdownChoices[]` | `BattleState.PendingKnockdownChoices` | 玩家击倒三选一选择列表 |
| `GainedCards[]` | `BattleState.PendingGainedCards` | 战斗中获得、战后可能进入 Run 的卡牌 |
| `DestroyedPartIds[]` | `BattleState.DestroyedPartIds` | 本场截至结束已破坏部位 ID，用于撤离重入 |

Battle 只负责产出战后包；疲劳、伤口、经验、获得卡、撤离进度、节点消耗等战外处理见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement)。

击倒事件补充：

- 援助 / 破坏是击倒事件分支，不依赖左 / 右手牌当前是否仍在手牌区，也不消耗左右手牌。
- 如果被击倒部位配置了 `KnockdownRewardCard`，选择援助或破坏会立刻创建一张战斗内 runtime card，随机插入当前手牌，并发出 `CardGained` 事件；撤离不触发奖励卡。字段配置见 [WacomData §3](./WacomData.md#wacomdata-enemy-part)。
- `CardGained.CardInstanceId` 是新建的战斗内卡实例，`ActorInstanceId` 是来源部位实例，`CardDefinition` 是奖励卡定义，`Count` 记录本次 `EKnockdownChoice`。
- 奖励卡选择时即写入 `FBattleResultPacket.GainedCards`；元素记录 `Definition / SourcePartId / SourceChoice`。后续即使这张战内卡被打出、弃掉、消耗或被上限弃牌，战后 Victory 仍会进入 Run。
- 撤离只在敌人仍有存活部位时可选；如果本次击倒后敌人所有部位都已清空，必须选择援助或破坏来完成最后一次击倒事件并进入胜利结算。
- `UBattleSession::BuildPendingKnockdownChoiceView()` 输出当前击倒事件的 `FKnockdownChoiceView`，包含部位信息与 Aid / Withdraw / Destroy 三个选项的可用性。`DisabledReason` 当前使用 `None / NoLivingEnemyPart`；`LeftHandMissing / RightHandMissing` 已预留但未被当前规则触发。
- `KnockdownChoiceRequested` 事件只负责通知 UI 需要展示选择面板；`FBattleEvent.Count` 的旧位掩码仅保留日志兼容，不再作为 UI 读取契约。
- 击倒 Dialog、中文文案、事件日志和按钮样式属于 UI 表现，见 [WacomUI §8](./WacomUI.md#wacomui-battle-ui) 与 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

`Outcome=Undetermined` 时不结算压力（异常路径 / 玩家取消）。

### 部位破坏经验记账

部位 HP 归零的瞬间（伤害命中 / 中毒结算）记一条 `FKnockdownExpGain` 到 `BattleState.PendingKnockdownExpGains`：

- `PartId`：来自 `UEnemyPartDefinition::PartId`
- `ExpAmount`：来自 `UEnemyPartDefinition::ExperienceReward`

每个部位只记一次（在 `bDestroyed false → true` 边沿记账）。
未填 `ExperienceReward` 的部位仍记一条 `ExpAmount=0`，让 Run 层有完整的"被破坏部位列表"，未来挂副作用更方便。

---

## §13 BattleState 结构

### 概览

BattleState 是战斗内核的可变状态，位于 `WacomBattle/Private/`，外部模块不可见。

### 主要组成

```
BattleState
├── Meta
│   ├── Phase（当前阶段）
│   ├── TurnNumber
│   ├── CurrentWaitValue（每回合重置为 2）
│   ├── Outcome（BattleEnd 时填充）
│   ├── StateVersion（每次命令后递增）
│   └── Rng（FRandomStream，确定性随机）
├── 战内 → 战外回传 flag
│   ├── HighHpThreshold / LowHpThreshold
│   ├── bCrossedHighHpThreshold（玩家受伤后 CheckHpThresholdsCrossed 判跨越）
│   ├── bCrossedLowHpThreshold（同上）
│   ├── bMutualDestruction（同归于尽；CheckAndApplyBattleEnd 维护）
│   ├── PendingKnockdownExpGains[]（部位破坏经验记账）
│   ├── PendingKnockdownEvents[]（待玩家三选一的击倒事件队列）
│   ├── PendingKnockdownChoices[]（玩家已做出的击倒选择）
│   ├── PendingGainedCards[]（战斗中获得、战后归入 Run 的卡牌）
│   └── DestroyedPartIds[]（撤离重入用的已破坏部位列表）
├── PlayerState
│   ├── CurrentHp / MaxHp
│   ├── Shield
│   ├── StatusStacks（TMap<FGameplayTag, int32>）
│   └── CompanionPlayedCount（全局伙伴计数）
├── CardContainers
│   ├── LeftHandInstanceId / RightHandInstanceId
│   ├── Hand（手牌队列，有序）
│   ├── DrawPile（抽牌堆）
│   ├── DiscardPile（弃牌堆）
│   ├── ExhaustPile（消耗区）
│   └── Limbo（左右手牌打出后暂存）
└── EnemyState
    ├── EnemyDefinition
    └── Parts[]（FRuntimeEnemyPart）
        ├── InstanceId / Def
        ├── CurrentHp / Shield
        ├── CurrentIntentIndex / CurrentInitiative
        ├── bDestroyed
        └── StatusStacks
```

### 卡牌离开手牌后的去向

| 条件 | 去向 |
|---|---|
| 普通牌 | 弃牌区 |
| 消耗牌 | 消耗区 |
| 左手牌/右手牌 | Limbo（不入任何区域）|
| 连击牌 | 留在原位置（不离开手牌）|
| 保留牌打出 | 弃牌区（保留只影响回合结束弃牌）|

---

## §14 BattleEvent 口径

`FBattleEvent` 是结算过程的记录流，供 UI 播表现、Combat Log 和自动化测试使用。事件不是真正规则状态；恢复、刷新和权威读取仍以 `BattleState` / `FBattleSnapshot` 为准。

当前事件类型：

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
| `PassiveTriggered` | 被动触发通知；部分被动第一版只发事件不改数值 |
| `HandLimitDiscarded` | 普通手牌上限导致某张卡弃掉 |
| `CardDiscarded` | 卡牌因弃牌规则从手牌进入弃牌堆；可由弃牌效果、手牌上限或回合结束清手产生 |
| `CardExhausted` | 卡牌因消耗规则从手牌进入消耗区 |
| `CardGained` | 战斗中获得新卡 |
| `BattleEnded` | 战斗进入结束态 |

`EnemyKnockdown` enum 仍保留在公共类型中，但当前击倒路径实际使用 `EnemyPartHpEmptied + KnockdownChoiceRequested + KnockdownChoiceMade`，不要把 `EnemyKnockdown` 当作活跃事件依赖。

中文文案、tone、icon、战斗 Combat Log 和 Toast 队列属于表现层，见 [WacomUI §3](./WacomUI.md#wacomui-toast)。V0-CJ 的玩家可读命令块是 `WacomApp` 对一次成功 HUD 命令后事件批次的 UI 聚合，不改变 `FBattleEvent` 协议，也不新增规则层 command batch id。
