---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-06-06
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

| 区域 | 当前 owner | 边界 |
|---|---|---|
| 命令提交 | `FWacomBattleHUDCommandFlow` | 把玩家意图转为 BattleSession command，不写规则细节 |
| 目标选择 | `FWacomBattleHUDTargetingFlow` | 维护 TargetSelect UI state 和点击入口 |
| 事件消费 | `FWacomBattleHUDEventFlow` | 消费 `UBattleSession::ConsumeEvents()` 并 fanout |
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Host 的 PartActor bridge 和 cue |
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
| Enemy 2D fallback | `UEnemyInfoBar / UEnemyPartWidget` | 缺 `SceneEnemyHost` 时的 2D fallback/debug |

旧 2D hand 的 `UHandPanel / UCardWidget / WBP_HandPanel / WBP_CardWidget` 已删除。正式战斗手牌主线是 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §6 Scene Enemy UI

场景敌人正式入口是 `ABattleTriggerActor.SceneEnemyHost + AWacomBattleEnemyActor.PartSlots + AWacomBattleEnemyPartActor`。进入战斗时 GameMode 把当前 Trigger 的 Host 传给 BattleHUD，HUD 只同步该 Host registry 中的 PartActor bridge。

`PartSlots` 是 Host 绑定合同：`Slot.PartId` 对应 `UEnemyPartDefinition::PartId`，是权威 authored id；Host 刷新时会同步到 `PartActor.PartId`、`InteractionTarget` 和 `WorldTargetBridge`。`PartSlots` 为空时保留 attached PartActor 扫描作为旧地图 fallback，但新制作不再依赖 attached 层级表达战斗语义。slot 顺序只影响 registry / badge stagger 表现，不改变 BattleSession 规则部位顺序。

每个 PartActor 通过 `UWacomBattleEnemyPartWorldTargetBridgeComponent` 接收：

- hover target 和 TargetSelect 可选提示。
- first-person drag preview。
- `TargetConfirmed`、`DamageDealt`、`EnemyPartHpEmptied` cue。
- 只读预测 Widget 和常驻状态 Badge 的 view。

有 SceneEnemyHost 时，BattleHUD 默认隐藏旧 `EnemyInfoBar`；缺 Host 时，`EnemyInfoBar / EnemyPartWidget` 继续作为 2D fallback/debug。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、敌方 fallback、ActionPanel、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand。

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
