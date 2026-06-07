---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-06-07
tags:
  - wacom/ui
  - wacom/battle
  - wacom/hud
---

# Wacom Battle UI 文档

> [!info] 本文职责
> 本文记录 Battle UI 当前事实：BattleHUD 命令出口、HUD state、targeting、presentation flow、Combat Log、Presentation Stack、兼容 event presentation、legacy / fallback UI、共享状态控件和测试入口。

> [!warning] 边界
> 本文只记录 UI 表现和命令出口。战斗规则真相见 [WacomBattle.md](./WacomBattle.md)，场景 target authoring 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，WBP 槽位见 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

## §1 BattleHUD 职责

`UBattleHUD` 是战斗 UI Screen coordinator。它保留 WBP 绑定、CommonUI 生命周期、`SetSession()`、Snapshot fanout、玩家意图 public 入口、BattleEnd 广播、配置值和 GC 引用。

`UBattleHUD` 不直接实现战斗规则。玩家命令、目标选择、事件消费、日志、击倒弹窗、表现队列、场景敌人、first-person hand 和卡牌详情运行时状态，分别收口到 `WacomApp/Private/UI/Battle` 的 flow / coordinator / controller helper。

BattleHUD 和表现层读取敌人状态时只使用 `FBattleSnapshot.Enemies`。Targeting flow、Combat Log、Debug HUD 和 SceneEnemyHost bridge 都按 enemy slot 遍历部位；不再存在 `Snapshot.Enemy` 第一敌人兼容别名，也不再暴露“Primary scene enemy host”作为 HUD 绑定入口。

| 区域 | 当前 owner | 边界 |
|---|---|---|
| 命令提交 | `FWacomBattleHUDCommandFlow` | 把玩家意图转为 BattleSession command，不写规则细节 |
| 目标选择 | `FWacomBattleHUDTargetingFlow` | 维护 TargetSelect UI state 和点击入口 |
| 事件消费 | `FWacomBattleHUDEventFlow` | 消费 `UBattleSession::ConsumeEvents()` 并 fanout |
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Trigger Host registry 的 PartActor bridge 和 cue |
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier |
| Combat Log | `FWacomBattleHUDCombatLogController` | history、trim、feed sync、readable log |
| First-person hand | `FWacomBattleHUDFirstPersonHandBridge` | runtime hand、drag preview/release、transition hint |
| Card Detail | `FWacomBattleHUDCardDetailController` | first-person viewport 详情 motion / source guard |

## §2 命令与 HUD State

玩家意图入口归到 `Wacom|Battle|Commands`：

- `OnCardClickedByUser`
- `OnEnemyPartClickedByUser`
- `OnWaitRequested`
- `OnEndTurnRequested`
- `CancelTargetSelect`
- `OnKnockdownChoiceSelected`

HUD 是命令出口。子 Widget 和 WBP 不直接修改 `UBattleSession`，也不在图里实现出牌、等待、结束回合、击倒选择或目标选择规则。

HUD 状态入口：

| 分类 | 内容 |
|---|---|
| `Wacom|Battle|HUD State` | `GetUIState`、`BP_OnUIStateChanged` |
| `Wacom|Battle|Targeting` | TargetSelect 查询、pending card、target selection view |
| `Wacom|Battle|Presentation Flow` | presentation busy、command gate、pending turn-boundary 查询 |

`UWacomBattleWidgetBase` 是 Battle UI 基类，只负责 `UBattleSession` 引用、Snapshot fanout 和 WBP 表现刷新钩子。`SetSession / GetSession` 属于 `Wacom|Battle|Widget Session`，`BP_OnRefreshedFromSnapshot` 属于 `Wacom|Battle|Snapshot Refresh`。

## §3 Combat Log 与 Presentation Stack

战斗反馈分四层：

| 层 | 类型 | 用途 |
|---|---|---|
| Presentation coordinator | `FWacomBattleHUDPresentationCoordinator` | TargetCue、短暂停顿、击倒 modal、BattleEnd signal、card stack boundary |
| Presentation Stack | `UBattlePresentationStackWidget` | 已提交但表现仍在追赶的卡牌小堆叠 |
| Combat Log | `UBattleCombatLogFeedWidget + UWacomBattleCombatLogBuilder` | 玩家可读命令块和事件 detail line |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 是当前正式 BattleHUD 玩家日志命令块 Builder。它把一次成功 HUD command 后消费到的事件批次聚合成 `FWacomBattleCombatLogBlockView`。规则层不新增 batch id；当前 UI 事实是一次成功 HUD 命令后的事件批次就是一个 combat log block。

`UBattleCombatLogFeedWidget` 是 BattleHUD 内部常驻滚动记录，默认承接本场最近玩家可读命令块。`UBattleCombatLogBlockWidget` 显示单个命令块和 detail line。正式 Details 配置位于 `Wacom|Battle|Combat Log|Authoring`。

`UBattlePresentationStackWidget` 是只读小卡表现 backlog，不是规则栈或交互入口。它用完整 `UWacomCardView` 作为 mini card，显示已提交但表现边界尚未释放的卡牌。正式 Details 配置位于 `Wacom|Battle|Presentation Stack|Authoring`。

Wait / EndTurn 请求遇到表现栈未清空时会进入 pending turn-boundary；ActionPanel 显示 pending 文案并禁用按钮，coordinator 等 stack 和 queue 清空后再提交等待或结束回合。

## §4 Event Presentation Compatibility

`UWacomBattleEventPresentationBuilder`、`FBattleEventPresentationView` 和 `EWacomBattleEventVisualTone` 是 UI-only 单事件展示词汇。它们仍被 Combat Log detail line 和 legacy 组件复用，但新的 BattleHUD WBP 不应直接消费 raw `FBattleEvent`。

分类口径：

| 类型 | 分类 | 语义 |
|---|---|---|
| `BuildEventPresentationView / FormatEventForPlayer` | `Wacom|Battle|Event Presentation|Compatibility` | 单条事件兼容文案 |
| `FormatCardName / FormatStatusName / FormatKnockdownChoice / FormatHandLimitDiscardSource` | shared text helper | 只生成 UI 文案，不写规则状态 |
| `BuildLegacyEventBlock()` | `Wacom|Battle|Combat Log|Legacy Compatibility` | 旧 event log panel 把单条事件镜像成 block |

## §5 Legacy / Fallback UI

以下公开面保留为兼容、fallback 或开发验证入口，不作为新的正式 BattleHUD 制作主线。

| 区域 | 类型 | 当前口径 |
|---|---|---|
| Legacy event log | `UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast` | 旧 WBP / PIE 对照保留；正式日志走 CombatLogFeed + CombatLogBlock |
| Debug text HUD | `UDebugBattleHUD` | Snapshot 文本诊断 HUD，不是正式 BattleHUD 父类 |

旧 2D hand 的 `UHandPanel / UCardWidget / WBP_HandPanel / WBP_CardWidget` 已删除。旧敌方 2D fallback 的 `UEnemyInfoBar / UEnemyPartWidget` 也已删除；缺少 `SceneEnemyHost` 时不会再显示 2D 敌人条，只会缺少场景敌人 hover / prediction / cue / 拖卡目标绑定。正式战斗手牌主线是 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §6 Scene Enemy UI

场景敌人视觉绑定正式入口是 `ABattleTriggerActor.SceneEnemyHostSlots + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；规则敌人列表由 `ABattleTriggerActor.EncounterDefinition` 转换成 `FBattleInitParams.EnemySlots`。新制作应把敌人做成 Host 蓝图 prefab：在 Host 蓝图视口中通过子 Actor / ChildActorComponent 摆放 `AWacomBattleEnemyPartActor`，然后在 Trigger 选好 `EncounterDefinition` 后执行 `SyncSceneEnemyHostSlotsFromEncounter()`，再在生成的 `SceneEnemyHostSlots` 中按 `EnemySlotId` 绑定对应 Host。进入战斗时 GameMode 把当前 Trigger 的 Host 列表传给 BattleHUD，HUD 只同步当前 Host registry 中扫描到的 PartActor bridge。HUD registry 是 Host 列表，不维护“主 Host”兼容缓存；调试摘要最多显示列表中的第一个 Host 名称用于排查。

Trigger 显式 `SceneEnemyHostSlots.EnemySlotId` 必须填写且不重复，并对应 `EncounterDefinition.EnemySlots[].EnemySlotId`。配置 `EncounterDefinition` 的正式 Trigger 必须用 `SceneEnemyHostSlots` 覆盖每个有效 EnemySlotId；缺 Host、漏映射或多余 EnemySlotId 都是编辑器验证错误。同步按钮会补齐并排序 Encounter 中的 slot，保留已填写 Host 引用，多余 slot 不自动删除而是留给人工确认。Trigger debug view / summary 会报告 `MissingSceneEnemyHostSlotIds` 和 `ExtraSceneEnemyHostSlotIds`。Host 刷新时只扫描自身蓝图 / 子 Actor 层级下的 PartActor，并向这些部位注入当前 Host 的 `EnemySlotId`。每个 PartActor 仍必须配置 `PartId`，对应 `UEnemyPartDefinition::PartId`，用于静态内容和 debug；`PartSlotId` 是 Host 内局部槽位身份，空时兼容回退到 `PartId`。Bridge 绑定 Snapshot 时优先匹配 `EncounterId + EnemySlotId + PartSlotId`，first-person world drop 会把 slot identity 放进 target handle，Battle validation / PlayCard resolver 再解析到当前运行时部位。子 PartActor 顺序只影响 registry / badge stagger 表现，不改变 BattleSession 规则部位顺序。

PartActor 是单个规则部位的表现容器。`HitBounds` 是唯一 hover、点击和拖卡命中范围；`VisualLayers` 是 2D 表现层数组，只负责多张图的相对位置、旋转、缩放、排序、颜色、显隐和轻量序列帧播放。每层用 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`：静态层生成 `UPaperSpriteComponent`，Flipbook 层生成 `UPaperFlipbookComponent`，并可配置 `FlipbookPlayRate`、是否循环、初始播放时间和是否自动播放。`VisualLayers` 非空时旧 `PartVisual` 原型网格隐藏；为空时继续显示 `VisualMesh / PartVisual` 作为旧地图兼容 fallback。Bridge 的反馈目标默认是 `VisualLayersRoot`，所以 TargetConfirmed、Damage、Destroyed、hover 和 drag preview 会缩放整组视觉层，而不是某一张 sprite。

`VisualLayers` 是表现层合同，不接管动画状态机。推荐用 `StaticSprite` 做主体、阴影、前景遮挡，用 `Flipbook` 做尾巴摆动、眼睛眨动、局部 idle 这类简单循环；更复杂的攻击 / 受击 / 死亡状态机后续再由 PaperZD 或专门 Animator 组件承接。缺少当前 `LayerMode` 对应资源的层不会生成组件，但会进入 validation warning 和 debug summary；`SortOrder` 映射到生成组件的 `TranslucentSortPriority`，数值越大越靠前。`HitBounds` 仍是唯一命中范围，sprite/flipbook 的尺寸和透明区域不改变目标身份或 BattleSession 规则。

蛇 Host prefab 的第一版正式口径是：创建 `AWacomBattleEnemyActor` 蓝图 prefab，推荐路径 `/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug`，Host `EnemyDefinition=/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake`、`EnemySlotId=Enemy`；在 Host 蓝图视口内放置三个 ChildActorComponent，Child Actor Class 都是 `AWacomBattleEnemyPartActor`。`SnakeHeadPart` 相对位置 `(96,-6,16)`，子 Actor 模板配置 `PartId=Snake.Head`、`PartSlotId=Head`、`HitBoundsExtent=(42,38,42)`、`VisualScale=(0.42,0.38,0.42)`；`SnakeBodyPart` 相对位置 `(0,0,0)`，配置 `PartId=Snake.Body`、`PartSlotId=Body`、`HitBoundsExtent=(62,46,42)`、`VisualScale=(0.62,0.46,0.42)`；`SnakeTailPart` 相对位置 `(-92,16,-8)`，配置 `PartId=Snake.Tail`、`PartSlotId=Tail`、`HitBoundsExtent=(48,34,34)`、`VisualScale=(0.48,0.34,0.34)`。`EnemySlotId` 由 Host / Trigger 注入，不在 PartActor 模板里手填。

Host validation 会同时检查 `PartId` 与 `PartSlotId`：`PartId` 必须对应 `UEnemyPartDefinition::PartId`，`PartSlotId` 必须对应 `UEnemyDefinition.Parts[].PartSlotId`。蛇的正式绑定身份是 `Enemy + Head/Body/Tail`，不是 `Enemy + Snake.Head/Snake.Body/Snake.Tail`；如果 `PartId` 对但 `PartSlotId` 填成完整 PartId，Validate Map 会给出 `UnknownPartSlotIds / MissingDefinitionPartSlotIds` warning，战斗中该部位可能无法按 slot identity 绑定。

Trigger 正式单蛇配置使用生成资产 `DA_Encounter_SnakeSingle`：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`；执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。`EncounterDefinition` 正式入口不能缺 Host 映射。

`ConfigureDebugSnakeHostSample()` 仍只作为当前蛇敌人 prefab 制作的开发辅助入口：先在 Host 蓝图中放好 Head / Body / Tail 三个 PartActor，再执行该函数。Host 会扫描运行时子 Actor 和蓝图 ChildActorComponent 的子 Actor 模板，尝试写入上述蛇样例配置。它不自动创建缺失部位，也不创建正式 sprite 资产；正式美术仍通过各 PartActor 的 `VisualLayers` 配置。Host 的一行诊断可在 Details 中执行 `LogBattleSceneEnemyDebugSummary()`，然后到 Output Log 查看 `[WacomBattleEnemyActor] BattleSceneEnemy{...}`。

Host 蓝图视口中的 `SnakeHeadPart / SnakeBodyPart / SnakeTailPart` 是 `ChildActorComponent`；关卡 Outliner 会在 Host 实例下显示它们生成出来的 `WacomBattleEnemyPartActor...` 子 Actor，这是正常现象。运行时和摆放校验优先使用这些已生成的真实子 Actor；只有蓝图模板 / CDO 等没有生成实例的场景才读取 ChildActor 模板，避免同一组 Head / Body / Tail 被重复计入 Host registry。

每个 PartActor 通过 `UWacomBattleEnemyPartWorldTargetBridgeComponent` 接收：

- hover target 和 TargetSelect 可选提示。
- first-person drag preview。
- `TargetConfirmed`、`DamageDealt`、`EnemyPartHpEmptied` cue。
- 只读预测 Widget 和常驻状态 Badge 的 view。

BattleHUD 不再构建或绑定敌方 2D fallback；点击、hover、drag target handle 全部通过当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge 完成。`EncounterDefinition` 正式入口缺 Host 会被编辑器验证阻止。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、ActionPanel、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。轻点、hold inspect、drag/aim、world target release 和 hand-card target release 都经 BattleHUD bridge / command flow 进入 BattleSession。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

`FirstPersonCardDetailViewportZOrder / FirstPersonCardDetailAnchorBaseSize` 属于 `Wacom|Battle|First Person Card Layer|Authoring`。旧 `BattleHandInteractionPrototype` 命名只作为兼容层保留，新 C++ 调用使用 `SetBattleHandInteractionEnabled()` / `IsBattleHandInteractionEnabled()`。

## §8 Battle Shared Widgets

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / San Snapshot |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用“标签 + 数量”显示控件；BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数` |
| `UWacomProgressBar` | `Wacom|Common UI|Progress Bar` | 通用数值进度条显示控件 |

BattleHUD 自身配置分类：

| 配置 | 分类 |
|---|---|
| fallback canvas layout | `Wacom|Battle|HUD Fallback Layout|Compatibility` |
| card detail authoring | `Wacom|Battle|Card Detail|Authoring` |
| card detail motion | `Wacom|Battle|Card Detail|Motion` |
| combat log max blocks | `Wacom|Battle|Combat Log|Authoring` |
| presentation stack hold | `Wacom|Battle|Presentation Stack|Authoring` |

## §9 Battle UI 测试入口

Battle UI 回归优先使用 `Source/WacomTests/Private/UI/BattleHUDTestHarness.h` 中的 `FWacomBattleHUDTestHarness` 搭配 `UWacomBattleHUDDetailTest` 装配 HUD、PlayerController、CombatLogFeed、PresentationStack、ActionPanel、first-person character 和 scene enemy Host。

测试不 include BattleHUD 私有 helper header，也不为生产 HUD 增加 Blueprint-visible 测试 API。只读诊断通过 `FWacomBattleHUDAutomationTestView` 聚合；Battle scene target click / probe 通过 `FWacomBattleSceneTargetClickTestAccess` 驱动。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
