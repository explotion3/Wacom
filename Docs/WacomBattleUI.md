---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-07-07
tags:
  - wacom/ui
  - wacom/battle
  - wacom/hud
---

# Wacom Battle UI 文档

> [!info] 本文职责
> 本文记录 Battle UI 当前事实：BattleHUD 命令出口、HUD state、targeting、presentation flow、Combat Log、Presentation Stack、event presentation helper、开发诊断入口、共享状态控件和测试入口。

> [!warning] 边界
> 本文只记录 UI 表现和命令出口。战斗规则真相见 [WacomBattle.md](./WacomBattle.md)，场景 target authoring 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，WBP 槽位见 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

## §1 BattleHUD 职责

`UBattleHUD` 是战斗 UI Screen façade。它保留 WBP 绑定、CommonUI 生命周期、`SetSession()`、Blueprint/public 命令入口、BattleEnd 广播、Authoring 参数和 GC-owned Widget 引用。

`UBattleHUD` 不直接实现战斗规则，也不直接持有 command、targeting、snapshot presentation、hand、scene enemy、detail 或 combat log 的业务状态。运行时状态收口到 `WacomApp/Private/UI/Battle/FWacomBattleHUDRuntime`；`FWacomBattleHUDRuntimeHost` 是唯一读取 HUD 私有 WBP / UPROPERTY / GC 引用的 adapter。

BattleHUD 和表现层读取敌人状态时只使用 `FBattleSnapshot.Enemies`。Targeting controller、Combat Log、Debug HUD 和 SceneEnemyHost bridge 都按 enemy slot 遍历部位；不再存在 `Snapshot.Enemy` 第一敌人兼容别名，也不再暴露“Primary scene enemy host”作为 HUD 绑定入口。

| 区域 | 当前 owner | 边界 |
|---|---|---|
| HUD façade / lifecycle | `UBattleHUD` | CommonUI 生命周期、WBP 绑定、Blueprint 入口、Authoring 参数、GC Widget 引用 |
| Runtime state / ownership | `FWacomBattleHUDRuntime` | HUD UI state、pending target、input gate、battle-end guard、last snapshot、controller 生命周期 |
| HUD private adapter | `FWacomBattleHUDRuntimeHost` | 唯一读取 HUD 私有 WBP / UPROPERTY / GC 引用的端口 |
| Snapshot presentation | `FWacomBattleHUDSnapshotPresenter` | Snapshot 刷新顺序、pile count、battle end 清理、child fanout 前后协作 |
| 命令提交 | `FWacomBattleHUDCommandController` | 把玩家意图转为 BattleSession command，不写规则细节；统一 AfterCommand |
| 目标选择 | `FWacomBattleHUDTargetingController` | 维护 TargetSelect UI state、pending card 和点击入口 |
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Trigger Host registry 的 PartActor bridge 和 cue |
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier、EndTurn phase plan |
| Combat Log | `FWacomBattleHUDCombatLogController` | history、trim、feed sync、readable log |
| First-person hand | `FWacomBattleHUDFirstPersonHandBridge + FWacomBattleHandPresentationController + FWacomFirstPersonCardLayerPresentationFrame` | runtime hand presentation frame、drag preview/release、Drawn transaction |
| Card Detail | `FWacomBattleHUDCardDetailController + FWacomFirstPersonCardDetailMotionController` | first-person viewport 详情 source guard、共享 motion / cache core |

这些 helper 都是 `WacomApp/Private/UI/Battle` 内部实现，不作为 WBP 或其它模块的制作入口。`FWacomBattleHUDSnapshotPresenter` 已独立到 App-private 文件中，负责 Snapshot fanout 顺序、pile count 复合文本和 BattleEnd 清理；`FWacomBattleHUDCommandController` 已独立到 App-private 文件中，负责把 HUD 玩家意图转换为 `UBattleSession` public command、构造 CombatLog context 并统一执行 AfterCommand；`FWacomBattleHUDTargetingController` 已独立到 App-private 文件中，负责 TargetSelect UI state、pending card 和 target selection view。`FWacomBattleHUDRuntime` 只保留刷新入口、运行时状态和 helper 生命周期。

## §2 命令与 HUD State

玩家意图入口归到 `Wacom|Battle|Commands`：

- `OnCardClickedByUser`
- `OnEnemyPartClickedByUser`
- `OnWaitRequested`
- `OnEndTurnRequested`
- `CancelTargetSelect`
- `OnKnockdownChoiceSelected`

HUD 是命令出口。子 Widget 和 WBP 不直接修改 `UBattleSession`，也不在图里实现出牌、等待、结束回合、击倒选择或目标选择规则。

键盘 `IA_Wait` / `IA_EndTurn` 进入 BattleHUD 前由 `AWacomPlayerController` 做 first-person hand 输入仲裁：如果当前卡牌层存在 active gesture，快捷键先取消该手势并被消费；只有卡牌层处于 idle / cancelled 时才调用 `OnWaitRequested` 或 `OnEndTurnRequested`。

`BattleInputReady` 是 BattleHUD 级玩家命令 gate，不属于 `UBattleSession` 规则阶段。进入战斗镜头 staging 期间它会临时为 false：CommandBar 按钮禁用，`CanSubmitPlayerActionCommand()` 返回 false，first-person hand release / Wait / EndTurn 等普通玩家命令不会提交；HUD 仍可刷新 Snapshot、同步场景敌人和播放非交互表现。镜头完成并激活 Battle camera look 后，GameMode 再把它恢复为 true。

EndTurn phase plan 运行期间，`CanSubmitPlayerActionCommand()` 返回 false，避免阶段化弃牌、保留、敌人行动和抽牌被新的玩家命令插入。普通 target cue queue 的旧节奏不因此改变；它仍可作为非阻塞表现队列服务出牌后的轻量反馈。

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
| Presentation coordinator | `FWacomBattleHUDPresentationCoordinator` | TargetCue、短暂停顿、击倒 modal、BattleEnd signal、card stack boundary、EndTurn phase plan |
| Presentation Stack | `UBattlePresentationStackWidget` | 已提交但表现仍在追赶的卡牌小堆叠 |
| Combat Log | `UBattleCombatLogFeedWidget + UWacomBattleCombatLogBuilder` | 玩家可读命令块和事件 detail line |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 是当前正式 BattleHUD 玩家日志命令块 Builder。它把一次成功 HUD command 后消费到的事件批次聚合成 `FWacomBattleCombatLogBlockView`。规则层不新增 batch id；当前 UI 事实是一次成功 HUD 命令后的事件批次就是一个 combat log block。

`UBattleCombatLogFeedWidget` 是 BattleHUD 内部常驻滚动记录，默认承接本场最近玩家可读命令块。`UBattleCombatLogBlockWidget` 显示单个命令块和 detail line。正式 Details 配置位于 `Wacom|Battle|Combat Log|Authoring`。

`UBattlePresentationStackWidget` 是只读小卡表现 backlog，不是规则栈或交互入口。它用完整 `UWacomCardView` 作为 mini card，显示已提交但表现边界尚未释放的卡牌。正式 Details 配置位于 `Wacom|Battle|Presentation Stack|Authoring`。

Presentation Stack 小卡使用出牌前 `FHandCardSnapshot` 构造卡牌 runtime presentation context，因此显示的是提交时的 `RuntimeCost`、可用状态和 RuntimeCost-based 徽章，不在表现期间回读后续手牌状态。带目标的出牌命令会在提交前捕获 `FBattleCardTargetPreview` 并放入 command context；小卡可使用这份 preview facts 显示提交时的目标修正后徽章，避免源卡离开手牌后再回查失败。

Wait / EndTurn 请求遇到表现栈未清空时会进入 pending turn-boundary；CommandBar 通过 `FWacomBattleCommandBarViewData` 显示 pending 文案并禁用按钮，coordinator 等 stack 和 queue 清空后再提交等待或结束回合。

EndTurn 命令成功后，BattleHUD 会消费 `FBattlePresentationJournal`。当 journal 能生成有效 phase plan 时，`FWacomBattleHUDPresentationCoordinator` 接管本次 EndTurn 表现，不再把整批事件直接压成一帧 hand hints，也不立即把 first-person hand 刷到最终抽牌态。v1 phase 顺序固定为：

1. `TurnEndDiscard`：用 discard checkpoint snapshot 播放非保留普通手牌 `Discarded` 离场。
2. `TurnEndRetain`：用 retain checkpoint snapshot 播放普通保留手牌和仍在手牌中的左右手 anchor `Retained` feedback；`CardsRetained` 规则事件本身仍只记录普通手牌。
3. `EnemyAction`：复用现有 battle event presentation queue 播放敌人行动相关 cue / delay / battle end / knockdown modal。
4. `TurnStartDraw`：用 draw checkpoint snapshot 播放新回合 `Drawn` 入场。
5. `TurnStartHandAnchorEnter`：如果 draw checkpoint 中出现了上一手牌 checkpoint 没有的左/右手 anchor，则在普通抽牌后提交完整 hand snapshot，并播放 `HandAnchorEntered` 生成入手。

手牌 phase 的完成条件由 first-person card layer 的 production playback 状态提供：仍有 active enter、exit outgoing 或 retained feedback 时保持 phase busy；播放结束后才进入下一 phase，并带 timeout 兜底。没有 journal 或 journal 无有效 phase 时，非 EndTurn / fallback 路径继续使用原来的 loose event hints 与 event queue。

## §4 Event Presentation Helper

`UWacomBattleEventPresentationBuilder`、`FBattleEventPresentationView` 和 `EWacomBattleEventVisualTone` 是 UI-only 单事件展示词汇。它们被 Combat Log detail line 复用，用于生成玩家可读中文文案、tone 和 icon；新的 BattleHUD WBP 不应直接消费 raw `FBattleEvent`。

分类口径：

| 类型 | 分类 | 语义 |
|---|---|---|
| `BuildEventPresentationView / FormatEventForPlayer` | `Wacom|Battle|Event Presentation` | 单条事件展示文案 |
| `FormatCardName / FormatStatusName / FormatKnockdownChoice / FormatHandLimitDiscardSource` | shared text helper | 只生成 UI 文案，不写规则状态 |

## §5 Debug / Development UI

以下公开面只作为调试或开发验证入口，不作为新的正式 BattleHUD 制作主线。

| 区域 | 类型 | 当前口径 |
|---|---|---|
| Debug text HUD | `UDebugBattleHUD` | Snapshot 文本诊断 HUD，不是正式 BattleHUD 父类 |

旧 Battle event log drawer / entry / toast 的 `UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast` 已删除；正式日志只走 `CombatLogFeed + BattleCombatLogBlock`。旧 2D hand 的 `UHandPanel / UCardWidget / WBP_HandPanel / WBP_CardWidget` 已删除。旧敌方 2D fallback 的 `UEnemyInfoBar / UEnemyPartWidget` 也已删除；缺少 `SceneEnemyHost` 时不会再显示 2D 敌人条，只会缺少场景敌人 hover / prediction / cue / 拖卡目标绑定。正式战斗手牌主线是 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §6 Scene Enemy UI

敌人常驻状态阅读已经收敛到敌人 Host 头顶的聚合面板：`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 承载 `UWacomBattleEnemyPanelWidget`，HUD 只从 `FBattleSnapshot.Enemies` 向场景敌人派发只读 view data。面板内部用 `UWacomBattleEnemyPartEntryWidget` 渲染每个部位的 HP、护盾、先机、意图和状态。普通部位 hover 也复用所属敌人的聚合面板响应；`AWacomBattleEnemyPartActor` / `UWacomBattleEnemyPartPresentationComponent` 不再创建部位级常驻状态 Badge。

场景敌人视觉绑定正式入口是 `ABattleTriggerActor.SceneEnemyHostSlots + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；规则敌人列表由 `ABattleTriggerActor.EncounterDefinition` 转换成 `FBattleInitParams.EnemySlots`。新制作应把敌人做成 Host 蓝图 prefab：在 Host 蓝图视口中通过子 Actor / ChildActorComponent 摆放 `AWacomBattleEnemyPartActor`，然后在 Trigger 选好 `EncounterDefinition` 后执行 `SyncSceneEnemyHostSlotsFromEncounter()`，再在生成的 `SceneEnemyHostSlots` 中按 `EnemySlotId` 绑定对应 Host。

Trigger 显式 `SceneEnemyHostSlots.EnemySlotId` 必须填写且不重复，并对应 `EncounterDefinition.EnemySlots[].EnemySlotId`。配置 `EncounterDefinition` 的正式 Trigger 必须用 `SceneEnemyHostSlots` 覆盖每个有效 EnemySlotId；缺 Host、漏映射或多余 EnemySlotId 都是编辑器验证错误。进入战斗时 GameMode 把当前 Trigger 的 Host 列表传给 BattleHUD，HUD 只同步当前 Host registry 中扫描到的 PartActor bridge。HUD registry 是 Host 列表，不维护“主 Host”兼容缓存。

敌人视觉有两条正式制作模式：

- 普通小怪推荐走 Host 整体视觉：在 `AWacomBattleEnemyActor` 的 `Presentation|Host Visual` 配整体 `PaperSprite` 或 `PaperFlipbook`，Head / Body / Tail 等子 `AWacomBattleEnemyPartActor` 只负责 `PartId / PartSlotId / HitBounds`、target bridge、Presentation、预测和状态 Badge。PartActor 没有 `VisualLayers` 时进入 `HitOnly` 视觉模式，自身不生成可见体；反馈、Badge 和预测仍跟随该部位自身的 Presentation。
- 精英 / Boss 走 PartActor 独立 `VisualLayers`：PartActor 是单个规则部位的表现容器。`HitBounds` 是唯一 hover、点击和拖卡命中范围；`VisualLayers` 只负责多张图的相对位置、旋转、缩放、排序、颜色、显隐和轻量序列帧播放。每层用 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`。`VisualLayers` 非空时优先于 Host 的 HitOnly 语境；为空且没有 Host 整体视觉时进入 `None / MissingVisualResource` 诊断。

Host 整体视觉的 `HostVisualMaterialOverride / bHostVisualCastShadow` 和每个 VisualLayer 的 `MaterialOverride / bCastShadow` 会应用到动态生成的 `UPaperSpriteComponent / UPaperFlipbookComponent`。需要 Sprite 投射阴影时，材质覆盖应使用 Paper2D 的 `MaskedLitSpriteMaterial` 或等效 lit masked 材质，并确保场景光源开启阴影；默认字段为空 / false 时保持无材质覆盖、无投影。

Host 整体视觉和 PartActor `VisualLayers` 的生成组件会在构造 / Details 刷新 / 显式刷新时重建，并在 PIE / runtime `BeginPlay` 再刷新一次，保证蓝图视口和实际运行一致。生成的 PaperSprite / PaperFlipbook 组件只在 Actor 拥有有效 `World` 时注册；蓝图模板、CDO 或 Details 预览这类无 World 场景只更新配置和诊断缓存。

Details 制作分组按主路径整理：Host 的 `Identity` 配置 `EnemyDefinition / EnemySlotId`，`Presentation|Host Visual` 配普通怪整体 sprite / flipbook，`Presentation|Badge Layout` 配部位 Badge 错开，`Authoring` 提供刷新按钮，`Authoring Status` 显示只读制作诊断缓存，`Debug Sample` 只放开发样例按钮。PartActor 的 `Identity` 配置 `PartId / PartSlotId`，`Collision` 配置 `HitBoundsExtent`；普通怪 PartActor 可以不配独立视觉，精英 / Boss 正式美术走 `Visual Layers`。PartActor 不再提供旧 StaticMesh 原型可见体入口。

Host 和 PartActor 的 Details `Authoring Status`、debug view 和 summary 使用同一套制作诊断事实。Host `AuthoringState=Ready` 表示 `EnemyDefinition`、子 PartActor、`PartId` 和 `PartSlotId` 对齐；常见异常包括 `MissingEnemyDefinition`、`NoPartActors`、`DuplicatePartSlotIds`、`PartSlotMismatch` 和 `PartDefinitionMismatch`。PartActor `AuthoringState` 会区分 `UsingVisualLayers`、`HitOnly`、`MissingIdentity`、`InvalidHitBounds` 和 `MissingVisualResource`，并通过 `VisualAuthoringMode=VisualLayers / HitOnly / None` 显示当前视觉路径。排查编辑器配置时先看 Details `Authoring Status`；需要完整一行串时再执行 Host / PartActor 的 `LogBattleSceneEnemyDebugSummary()`。

Host 整体视觉和 `VisualLayers` 都是表现层合同，不接管动画状态机。普通小怪可以先用 Host `StaticSprite` 或 Host `Flipbook` 做整体 idle；精英 / Boss 推荐用 PartActor `StaticSprite` 做主体、阴影、前景遮挡，用 PartActor `Flipbook` 做尾巴摆动、眼睛眨动或局部表现。`SortOrder` 映射到 `TranslucentSortPriority`，数值越大越靠前。`HitBounds` 仍是唯一命中范围，sprite / flipbook 的尺寸和透明区域不改变目标身份或 BattleSession 规则。

蛇 Host prefab 的第一版正式口径是：创建 `AWacomBattleEnemyActor` 蓝图 prefab，推荐路径 `/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug`，Host `EnemyDefinition=/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake`、`EnemySlotId=Enemy`；普通蛇美术优先在 Host 的 `Presentation|Host Visual` 配整体 sprite / flipbook。Host 蓝图视口内放置三个 ChildActorComponent，Child Actor Class 都是 `AWacomBattleEnemyPartActor`：

| ChildActor | Relative Location | PartId | PartSlotId | HitBoundsExtent |
|---|---:|---|---|---:|
| `SnakeHeadPart` | `(96,-6,16)` | `Snake.Head` | `Head` | `(42,38,42)` |
| `SnakeBodyPart` | `(0,0,0)` | `Snake.Body` | `Body` | `(62,46,42)` |
| `SnakeTailPart` | `(-92,16,-8)` | `Snake.Tail` | `Tail` | `(48,34,34)` |

`EnemySlotId` 由 Host / Trigger 注入，不在 PartActor 模板里手填。Host validation 会同时检查 `PartId` 与 `PartSlotId`：`PartId` 必须对应 `UEnemyPartDefinition::PartId`，`PartSlotId` 必须对应 `UEnemyDefinition.Parts[].PartSlotId`。蛇的正式绑定身份是 `Enemy + Head/Body/Tail`，不是 `Enemy + Snake.Head/Snake.Body/Snake.Tail`。

Host 的 `RefreshBattleEnemyPartAuthoringState()` 是安全刷新入口：它刷新 Host visual，扫描自身 PartActor，并同步 `EnemySlotId`、Host visual 语境、Badge 诊断和 debug summary。它不会根据 `EnemyDefinition`、Actor 名称或组件名称自动补齐 `PartId / PartSlotId`；缺失或填错的身份必须通过手动配置、蓝图子类默认值或显式样例按钮修正，并会通过 `Authoring Status` 和 Validate Map 暴露。

Trigger 正式单蛇配置使用生成资产 `DA_Encounter_SnakeSingle`：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`；执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。`EncounterDefinition` 正式入口不能缺 Host 映射。

`ConfigureDebugSnakeHostSample()` 是制作开发辅助入口：先在 Host 蓝图中放好 Head / Body / Tail 三个 PartActor，再执行该函数。Host 会扫描运行时子 Actor 和蓝图 ChildActorComponent 的子 Actor 模板，尝试写入上述蛇样例配置。它不自动创建缺失部位，也不创建正式美术资产；普通蛇的正式美术由 Host `Presentation|Host Visual` 配整体图，精英 / Boss 才优先在各 PartActor 配 `VisualLayers`。

Host 蓝图视口中的 `SnakeHeadPart / SnakeBodyPart / SnakeTailPart` 是 `ChildActorComponent`；关卡 Outliner 会在 Host 实例下显示它们生成出来的 `WacomBattleEnemyPartActor...` 子 Actor，这是正常现象。运行时和摆放校验优先使用这些已生成的真实子 Actor；只有蓝图模板 / CDO 等没有生成实例的场景才读取 ChildActor 模板，避免同一组 Head / Body / Tail 被重复计入 Host registry。

每个 PartActor 默认携带两类运行时组件：

- `UWacomBattleEnemyPartWorldTargetBridgeComponent`：只负责 `EncounterId / EnemySlotId / PartSlotId -> PartInstanceId` 绑定、当前 HUD registry 标记、targetable 绑定诊断，以及把运行时身份写入 `UWacomInteractionTargetComponent`。
- `UWacomBattleEnemyPartPresentationComponent`：负责 hover target、TargetSelect 可选提示、first-person drag preview、`TargetConfirmed / DamageDealt / EnemyPartHpEmptied` cue、只读预测 Widget 和常驻状态 Badge。

BattleHUD scene enemy coordinator 成对缓存 Bridge 和 Presentation：target handle 解析走 Bridge，表现 target 注册和反馈走 Presentation。target handle 必须携带完整 `EncounterId + EnemySlotId + PartSlotId` 才能命中当前 registry；`SourceObject` 即使指向当前 PartActor，也不会替代或修正错误 key。Host / PartActor debug summary 会合并两者事实用于 PIE 排查，但 passive UI 和 BattleSession 不依赖这个合并 debug。

BattleHUD 不再构建或绑定敌方 2D fallback；点击、hover、drag target handle 全部通过当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge 完成。`EncounterDefinition` 正式入口缺 Host 会被编辑器验证阻止。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、CommandBar、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。轻点、hold inspect、drag/aim、world target release 和 hand-card target release 都经 BattleHUD bridge / command flow 进入 BattleSession。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

First-person hand 卡面和 first-person viewport 详情都从 `FHandCardSnapshot` 派生 `FWacomCardPresentationRuntimeContext`，再交给 `UWacomCardPresentationBuilder` 生成 ViewData。基础 runtime context 覆盖本场 `RuntimeCost` 与 `bIsPlayable`：卡面 Cost、disabled overlay、RuntimeCost-based 效果徽章和详情 `Sections` 会显示当前战斗事实。详情面板正式渲染来源是 `FWacomCardDetailViewData.Sections`：Builder 负责按卡牌详情文档顺序组装 section，App-private `WacomCardDetailTextCompiler` 负责把手写正文、效果 tag、数值、图标和 fallback 文本编译成 token；`UWacomCardDetailPanel` 只按 section 顺序渲染，不再按 token kind 或原始 `Description` 推断分区。手写描述可用 `{Effect.N}` 显式把主动效果嵌入描述，例如 `造成 {Effect.0} 伤害。`；拖拽 preview 时占位符内数字直接变化，不额外追加整条主动效果行。`Passive.DisplayText` 使用同一占位符语法，其中 `{Effect.N}` 指向当前 `FCardPassive.Effects[N]`。没有手写描述时才使用自动主动效果行 fallback。旧 `Description`、`ChangeLines`、`PassiveLines`、`TaskLines` 和扁平 `TokenLines` 已从 `FWacomCardDetailViewData` 删除；费用变化、目标手牌 cost preview、被动正文和后续任务/预览内容都应反映到对应卡面数值或正式 `Sections` token。Battle 详情的数据来源和 source guard 仍由 `FWacomBattleHUDCardDetailController` 负责；预热、详情数据缓存、淡入淡出 / scale / follow motion 和稳定换边由 App-private `FWacomFirstPersonCardDetailMotionController` 与 Run first-person 详情共用。

First-person 详情面板的 viewport 生命周期由 `FWacomFirstPersonCardDetailPanelHost` 统一处理：只有拥有真实 local player / LocalPlayer 的 context 才允许 `AddToViewport`，HUD-only 自动化或离屏预热只构建面板对象并验证 ViewData / motion state。详情面板内部的 section、token flow、token line 和 token widget 动态创建收口在 App-private `WacomCardDetailWidgetFactory`；详情 Widget 不应直接用 `GetWorld()` 判断创建路径，避免无 World 的离屏场景污染日志。

被动详情正文的分类由“被动”区块标题承载，正文 token 不再携带 `被动：` 前缀。`Passive.DisplayText` 是被动正文的最高优先级来源，适合表达暮气、选择对象、特殊腾挪等尚未完全结构化的规则；没有手写正文且所有被动效果都能结构化时，Builder 才生成“触发条件：”加效果 token。只生成触发条件不能视为完整被动正文，避免后半句被 fallback 屏蔽。

Battle hand entries 由 `WacomBattleCardPresentation` 从 `FHandCardSnapshot` 构建：`ECardTargetMode` 只在 Battle adapter 内部映射成 first-person card layer 的 `InteractionIntent`，不再写入通用 entry。通用 first-person card layer 公共类型不提供 TargetMode 转换 helper；SlotWidget 只消费 `InteractionIntent` 来决定无目标拖拽或瞄准态，目标合法性和提交仍由 BattleHUD / BattleSession 处理。Hand-card target release、probe-only 状态和 full-hand card affordance 都以 `UBattleSession::ValidateTargetWithCard()` 的 reject reason 为准，first-person hand bridge 不再直接读取卡牌 `TargetMode` 来判断 hand-card 目标规则。

当玩家拖拽手牌并指向敌人部位或目标手牌时，first-person hand bridge 会把当前 `CardInstanceId + TargetHandle` 交给 `UBattleSession::BuildCardTargetPreview()`。Battle 返回的 `FBattleCardTargetPreview` 是只读规则 facts；App 侧随后用 `WacomBattleCardPresentation::BuildTargetPreviewPresentation()` 一次性生成 hand layer entries、源卡详情和可选目标手牌详情。源卡卡面徽章和详情 token 显示目标修正后的主效果预览；若目标是手牌，目标卡自己的卡面费用可以显示预测后的费用，但详情不生成 `[费] before -> after` token。preview 不提交命令、不模拟完整出牌事件链、不修改 Battle state。

拖拽 release、cancel、离开目标、候选目标无效或 UI state 退出时，bridge 会清理 preview entries，恢复基础 hand entries 和当前详情。Scene enemy hover / TargetSelect hover 也先构建同一份 `FWacomBattleCardTargetPreviewPresentation` 再应用：场景目标反馈仍由 enemy presentation component 负责，卡面和详情只消费该 presentation，不在 hover / drag 两条路径里重复拼。Preview semantic state 由 snapshot version、source id、目标身份和 preview facts hash 组成；同一 state 上的高频 hover / drag move 只允许更新指针反馈、敌人 hover 和详情位置，hand layer preview entries 与详情数据必须等 preview semantic state 变化后再重建。Active drag 期间，目标手牌 preview 的生命周期由 bridge 保存的 `ActiveDragView.CurrentTarget` / target preview state 决定，SlotWidget 重建或 hover/unhover 抖动不能作为清理 preview 的权威信号。

`FirstPersonCardDetailViewportZOrder / FirstPersonCardDetailAnchorBaseSize` 属于 `Wacom|Battle|First Person Card Layer|Authoring`。第一人称手牌交互开关由当前 runtime source owner 通过 `SetFirstPersonCardLayerInteractionEnabled()` / `IsFirstPersonCardLayerInteractionEnabled()` 控制；Anchor 上旧的 `bEnableBattleHandInteraction` / BattleHand 命名 getter 只作为旧资产和旧 Blueprint 兼容别名保留。

Battle entry staging 期间，BattleHUD 的 first-person hand bridge 会把 `WacomFirstPersonCardLayerSourceIds::BattleHand()` runtime layer 视为 suppressed：进入 suppression 时先关闭 `BattleHand` presentation gate、清空当前 first-person card layer visual slot，任何 hand sync 都写入 0 entries 的空 `BattleHand` runtime source，不启用 hand interaction，也阻止 Anchor 回退到 preview card layer。suppressed 期间 `FWacomBattleHandPresentationController` 不推进已展示 snapshot baseline；首回合 `CardsDrawn` 作为 entry reveal transaction 保留到 Battle camera look 激活、suppression 解除后的第一次 hand refresh 中，再用空手牌 baseline 生成一次显式 `entries + Drawn hints` presentation frame。若当前 snapshot 中有新出现的左右手 anchor，这个 opening frame 会先临时不提交它们；普通抽牌播放结束后，bridge 再提交完整 snapshot，并对这些 anchor 播放 `HandAnchorEntered` follow-up frame。若 Drawn frame 已经提交给 Anchor 但还没被 Layer tick 消费时又进入 suppression，bridge 会先把该提交恢复成 controller 待播事件，再清空 visual source，避免卡牌在不可见阶段被吞掉或后续直接落位。Battle camera look 激活后，GameMode 解除 suppression 并用当前 Battle snapshot 重新刷新 first-person hand；这次正式 hand sync 会重新打开 presentation gate，Anchor 才允许消费 frame hints。这避免手牌从探索相机位置一路插值到 battle viewpoint，也避免抽牌入场在不可见阶段提前播完。

Battle hand 抽牌表现由 `FWacomBattleHandPresentationController` 事务化交付。Controller 消费 Battle events 时，优先把 `CardsDrawn.CardInstanceIds` 中仍存在于下一帧 hand snapshot 的真实普通卡实例转成 `Drawn` transition hint，并为可见 hint 写入稳定 `SequenceIndex / SequenceCount`；只有旧式 Count-only 事件才回退到 baseline / next snapshot 新增普通卡推断，不把左右手 anchor 纳入抽牌预算。HUD 不计算入场曲线、延迟、弧线或来源位置；这些表现参数由 first-person card layer / Anchor `06 Transition Motion` 统一处理。Bridge 只负责把 controller 给出的 `FWacomFirstPersonCardLayerPresentationFrame` 写入 Anchor，或在没有 pending event 时执行普通 entries refresh。普通 refresh 不会替换 Anchor 里尚未消费的 presentation frame hints；只有新的显式 frame、source clear、suppression 或 battle end 会替换 / 清空 hints，避免状态刷新把抽牌入场表现吞掉。Anchor 还有 source-scoped presentation gate：gate 关闭时 entries 可以刷新，但 pending frame hints 不能被送进 Layer。Layer 收到 frame 后也不会在一次空 slots / unprojected slots 刷新中丢弃 Drawn：只有 gate 已打开、对应 slot 可投影并真正启动入场播放后才消费该 hint，镜头 staging 或 viewport 投影暂不可用时会延迟到后续 hand refresh。

## §8 Battle Shared Widgets

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。玩家和敌人 runtime 状态共用 `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget`：控件只消费 Snapshot / ViewData 中的 `Statuses / StatusStacks`，图标 Brush 由 WBP 变量配置，`Status.Shield` 仍由 HP / Shield UI 单独显示。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / runtime 状态图标 |
| `UBattleCommandBarWidget / UWacomBattleCommandButtonWidget` | `Wacom|Battle|Command Bar|Authoring` | 被动显示 Wait / EndTurn 命令 view data，并把玩家意图广播回 BattleHUD |
| `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget` | `Wacom|Battle|Status Icons|Authoring` | 共享状态图标列表和单个状态图标；玩家状态条正式使用，敌人部位条目可选接入 |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用数量显示控件；牌堆类型由 WBP Image 图标表达，BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数` |
| `UWacomProgressBar` | `Wacom|Common UI|Progress Bar` | 通用数值进度条显示控件 |

CommandBar 的轻量协议定义在 `BattleCommandBarTypes.h`：`EWacomBattleCommandId`、`FWacomBattleCommandButtonView` 和 `FWacomBattleCommandBarViewData` 可以被 HUD / runtime presenter / tests 直接使用；`BattleCommandBarWidget.h` 只承载 UMG Widget 实现与 WBP 制作面。CommandBar ViewData 构建收口在 App-private `FWacomBattleHUDCommandBarPresenter`，`FWacomBattleHUDRuntime` 只保留刷新入口和 command gate 查询。

BattleHUD 自身配置分类：

| 配置 | 分类 |
|---|---|
| C++ 默认 canvas layout | `Wacom|Battle|HUD Default Layout` |
| card detail authoring | `Wacom|Battle|Card Detail|Authoring` |
| card detail motion | `Wacom|Battle|Card Detail|Motion` |
| combat log max blocks | `Wacom|Battle|Combat Log|Authoring` |
| presentation stack hold | `Wacom|Battle|Presentation Stack|Authoring` |

## §9 Battle UI 测试入口

Battle UI 回归优先使用 `Source/WacomTests/Private/UI/BattleHUDTestHarness.h` 中的 `FWacomBattleHUDTestHarness` 搭配 `UWacomBattleHUDDetailTest` 装配 HUD、PlayerController、CombatLogFeed、PresentationStack、CommandBar、first-person character 和 scene enemy Host。

测试不 include BattleHUD 私有 helper header，也不为生产 HUD 增加 Blueprint-visible 测试 API。只读诊断通过 `FWacomBattleHUDAutomationTestView` 聚合；Battle scene target click / probe 通过 `FWacomBattleSceneTargetClickTestAccess` 驱动。

`Source/WacomTests/Private/UI/BattleHUDCommandFlowSpec.cpp` 承载 BattleHUD 命令和目标选择的专题合同测试，覆盖 `FWacomBattleHUDCommandController` / `FWacomBattleHUDTargetingController` 对外表现。`Source/WacomTests/Private/UI/BattleCombatLogSpec.cpp` 承载 Combat Log builder、feed、HUD history 和 `FWacomBattleHUDCombatLogController` 的专题合同测试，统一前缀为 `Wacom.UI.Battle.CombatLog`。`Source/WacomTests/Private/UI/BattlePresentationStackSpec.cpp` 承载 `UBattlePresentationStackWidget` / `UBattlePresentationStackEntryWidget` 的纯展示合同测试。`Source/WacomTests/Private/UI/BattlePresentationQueueSpec.cpp` 承载 BattleHUD presentation queue / turn-boundary / BattleEnd 清理 / knockdown 延迟展示合同测试。`Source/WacomTests/Private/UI/BattleInteractionTargetSpec.cpp` 承载 battle scene enemy part world target bridge 和 scene click / probe 的 `Wacom.UI.Battle.InteractionTarget` 合同测试。`Source/WacomTests/Private/UI/BattleSceneEnemyTargetRegistrySpec.cpp` 承载 battle scene enemy target registry 专题合同测试，覆盖 Trigger scene enemy host slot -> HUD registry、current-host filtering、trigger authoring validation 和 registry-routed cue / hover / drag preview。`Source/WacomTests/Private/UI/BattleSceneEnemyHoverProbeSpec.cpp` 承载 battle scene enemy hover probe 专题合同测试，覆盖 hover visual priority、HUD hover probe bridge、TargetSelect hover prediction、无效目标清理、pending / drag / BattleEnd gate 和 hover debug summary。`Source/WacomTests/Private/UI/BattleHUDFirstPersonSpec.cpp` 承载 BattleHUD first-person hand / first-person card detail 专题合同测试，覆盖 hand bridge clear、Anchor interaction、camera look override、first-person detail host、readability motion 和 inspect hover guard。`Source/WacomTests/Private/UI/BattleSceneEnemyActorSpec.cpp` 承载 battle scene enemy actor 专题合同测试，当前覆盖 hand snapshot swift prediction facts、prediction widget facade、PartActor facade / presentation setup、host visual / hit-only part、host identity / child actor scan、runtime facts / host counts、debug snake child actor authoring、part slot identity / duplicate validation、hover / drag prediction badge、VisualLayers refresh / validation 和 blueprint default authoring 分支。`BattleWidgetSpec.cpp` 保留 fallback layout、event presentation 和其他跨专题旧测试并继续分批收口。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.Battle.CombatLog`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
