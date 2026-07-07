---
type: interaction-contract
scope: wacom-world-interaction
status: active
updated: 2026-07-08
tags:
  - wacom/app
  - wacom/world-interaction
  - wacom/targeting
  - wacom/authoring
---

# Wacom World Interaction 文档

> [!info] 本文职责
> 本文记录 WacomApp 世界交互和 target system 的当前事实：Run world interactable Actor、Battle scene target、Run menu drop target、target handle、debug / authoring / validation 约定。

> [!warning] 边界
> App 只负责命中、路由、提示和表现反馈。Battle / Run 规则真相仍在 `UBattleSession` 与 `URunSession`；静态 Definition 字段见 [WacomData.md](./WacomData.md)，内容校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)；UI 总入口见 [WacomUI.md](./WacomUI.md)，Battle UI 和 first-person hand 细节见 [WacomBattleUI.md](./WacomBattleUI.md) 与 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §1 总原则

世界交互入口分为 E 键、鼠标 hover、鼠标点击、first-person card drop 和 Run menu zone drop。所有入口最终都回到明确的领域出口：

- 普通探索交互：`IWacomWorldInteractable::TryInteract()`。
- Battle target click / drag：`UBattleHUD` 将玩家意图转为 Battle command 或 target selection。
- Run world card drop：`UWacomRunWorldCardDropReceiverComponent -> URunSession::Validate/SubmitRunWorldCardInteraction()`。
- Run menu zone drop：owning menu 解析和提交；RunEvent 支付进入 `URunSession::ChooseRunEventOptionWithPaidCardResult()`。

Actor 和 Widget 可以提供 target handle、debug view、hover prompt 和 preview feedback，但不能直接修改 Battle / Run 状态真相。

## §2 Run World Interactable Actor

探索期 `E` 交互使用 `IWacomWorldInteractable`。进入范围时注册到 PlayerController，离开范围或销毁时反注册；多个候选重叠时按 `GetInteractLocation()` 选最近且 `CanInteract=true` 的对象。

正式可放置入口：

| Actor | 用途 | 规则出口 |
|---|---|---|
| `ABattleTriggerActor` | 场景敌人战斗入口 | `AWacomGameMode::EnterBattle()` |
| `AWacomShopTriggerActor` | 场景商店入口 | `URunSession::BeginShopVisit()` + Shop Screen flow |
| `AWacomRunEventTriggerActor` | 场景探索事件入口 | `URunSession::BeginRunEvent()` + RunEvent Screen flow |
| `AWacomRunPickupActor` | 金币 Pickup 快速入口 | `URunSession::CollectGoldPickup()` |
| `AWacomRunCardPickupActor` | 固定单卡 Pickup 快速入口 | `URunSession::CollectCardPickup()` |
| `AWacomRunRewardPickupActor` | 数据驱动 Pickup 推荐入口 | Gold / Card pickup 规则入口 |
| `AWacomRunKeyChestActor` | Run world card interaction 验证入口 | `URunSession::SubmitRunWorldCardInteraction()` |

关卡实例必须使用场景级唯一 `PersistentId` 作为运行时状态 key。静态 Definition 的 `EncounterDefinitionId / ShopId / EventId / PickupId / InteractionId` 是内容 ID、debug ID 或资产语义，不替代场景 `PersistentId`。

### Authoring 口径

- 正式关卡推荐使用 Definition 驱动的入口：`ABattleTriggerActor.EncounterDefinition`、`AWacomShopTriggerActor.ShopDefinition`、`AWacomRunEventTriggerActor.EventDefinition`、`AWacomRunRewardPickupActor.PickupDefinition`、`AWacomRunKeyChestActor.CardInteractionDefinition`。
- `ABattleTriggerActor` 必须通过 `EncounterDefinition` 进入战斗，并用 `SceneEnemyHostSlots` 完成 Encounter enemy slot 到场景 Host 的映射。
- `AWacomShopTriggerActor` 可选配置 `ShopEntryViewpoint`。配置后只改变 App 层镜头 / hand staging：玩家先移动到商店第一人称 View Pose，再打开 ShopScreen；关闭后先退 UI，再回 RunTunnel。它不改变 `PersistentId`、`ShopDefinition`、`Offers`、库存持久化或购买规则。
- `AWacomRunEventTriggerActor` 可选配置 `RunEventEntryViewpoint`。配置后只改变 App 层镜头 / hand staging：玩家先移动到事件第一人称 View Pose，再打开 RunEventScreen；关闭后先退 UI，再回 RunTunnel。已完成事件仍只显示完成提示，不会触发 staging；该字段不改变 `PersistentId`、`EventDefinition`、完成状态或选项结算规则。
- Definition 字段语义见 [WacomData.md](./WacomData.md)；生成样例和资产 validator 口径见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。
- 专用 Pickup Actor 只作为快速验证入口保留；正式关卡优先使用 Definition 驱动入口，不把快速入口扩展成新的长期制作主线。
- Blueprint 只放默认外观和可见 primitive，不写 EventGraph 规则逻辑、不直接调用 RunSession。
- Prototype sample button 只用于 PIE / 开发验证；它们只改当前 Actor 配置和 facade 同步，不修改 RunState、不生成资产。

### Data Validation 口径

摆放实例应通过 Validate Map/Level 校验：

- 缺 `PersistentId` 通常是 error。
- 同 World 内重复 `PersistentId` 是 warning，因为它会共享运行时状态。
- 缺必要 Definition、无可用 reward / offer / event、无正向卡牌筛选、无效金币数或缺卡定义等配置错误是 error。
- BP 默认资产 / CDO 允许保持空配置，避免推荐摆放 BP 被误判为 invalid。

## §3 Click / Hover Target Contract

Run world click / hover 使用显式 opt-in：

- Actor 必须同时实现 `IWacomWorldInteractable` 和 `UWacomRunWorldClickableInteractable`。
- Actor 必须通过 `UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 暴露 `Interaction.Target.Run.Object`。
- `ClickBounds` 只阻挡 `Visibility` trace，不产生 overlap；`TriggerSphere` 仍只服务 E 键近距离候选注册。
- PlayerController resolver 会校验 handle、source actor、clickable contract、world interactable contract 和 bridge，再把左键命中转回现有 `TryInteract()`。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，Run world hover/click 不穿透场景。菜单 first-person card drag/drop 正在处理时，也会清理普通 Run world hover prompt 和 probe preview。

`UWacomRunWorldClickableInteractable` 提供 hover prompt 和通用 debug view。常见 reject reason 包括缺少 world interactable contract、缺少 clickable contract、缺少 bridge、目标 tag 不匹配或当前 flow 不允许交互。

## §4 Run World Card Drop

没有 active GameMenu / menu lease 时，探索期 first-person 卡牌可以拖到 Run world target。

提交要求：

- release 命中 `Interaction.Target.Run.Object`。
- 目标 Actor 有 `UWacomRunWorldCardDropReceiverComponent`。
- receiver 提供正向筛选：`AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 至少一个非空。
- `URunSession::ValidateRunWorldCardInteraction()` 接受精确 `SourceCardInstanceId`。

成功提交由 `SubmitRunWorldCardInteraction()` 在 RunSession 事务内完成：可选消耗源卡，按 reward payload 发放 Gold / Card，写入 `CompletedRunWorldInteractionIds`，最后广播一次 Run state changed。

失败反馈属于 App 表现层：preview 阶段只更新轻量有效 / 无效反馈；release 命中过目标且提交失败时才发 AppToast。文案优先来自 receiver failure contract，没有 receiver 可询问时由 PlayerController 提供通用配置异常 fallback。

KeyChest 是当前第一条 Run world card interaction 验证入口。普通 E 键或左键点击只显示“需要钥匙 / 宝箱已打开”提示，不直接结算奖励；拖卡成功后才提交 RunSession 事务。

## §5 Battle Scene Target

Battle scene enemy 视觉绑定正式入口是 `ABattleTriggerActor.SceneEnemyHostSlots + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；战斗规则敌人列表正式入口是 `ABattleTriggerActor.EncounterDefinition`：

- Trigger 进入战斗时把 `EncounterDefinition.EnemySlots` 转换为 `FBattleInitParams.EnemySlots`。
- Trigger 进入战斗时把 `SceneEnemyHostSlots` 中的 `EnemySlotId -> SceneEnemyHost` 映射按 `EncounterDefinition.EnemySlots` 顺序传给 `UBattleHUD`；没有 Host 映射的 Trigger 会被编辑器验证阻止，不再走旧单 Host 兼容入口。
- 配置 `EncounterDefinition` 的正式入口必须配置 `SceneEnemyHostSlots` 并覆盖每个有效 `EncounterDefinition.EnemySlots[].EnemySlotId`；缺 Host / 漏映射 / 多余 EnemySlotId 都是摆放配置错误。
- Trigger Details 中可执行 `SyncSceneEnemyHostSlotsFromEncounter()`，它会按 Encounter enemy slot 顺序补齐 `SceneEnemyHostSlots`，保留已填写的 Host 引用，并把不在 Encounter 中的多余 slot 追加保留到尾部供人工确认删除。
- 显式 `SceneEnemyHostSlots.EnemySlotId` 必须填写且不重复，并对应 `EncounterDefinition.EnemySlots[].EnemySlotId`。进入战斗 / 刷新时 Trigger 会用 slot 上的 `EnemySlotId` 覆盖对应 Host 的临时 `EnemySlotId`，再由 Host 注入到子 PartActor；Host 自己的默认值不参与多敌人身份推断。
- Host 作为敌人 prefab 根 Actor，只从自身蓝图 / 子 Actor 层级自动扫描 `AWacomBattleEnemyPartActor`；蓝图视口里的 ChildActorComponent PartActor 是新制作主线。
- HUD 只同步当前 Host registry 中的 PartActor bridge；多敌人 Encounter 可同时注册多个 Host，同关卡其它 Trigger/Host 的同名 `PartId` 部位不参与当前战斗。
- 每个 PartActor 必须配置静态 `PartId` 用于内容校验和 debug；运行时目标绑定只由 `EncounterId + EnemySlotId + PartSlotId` 解析并写回当前 `PartInstanceId`。
- `SceneEnemyHostSlots.EnemySlotId` 是 Host 的运行时敌人槽身份；空 `EnemySlotId` 是摆放错误，不做旧单 Host 兜底。
- PartActor 的 `PartSlotId` 是 Host 内局部槽位 ID，必须显式填写并对应 `EnemyDefinition.Parts[].PartSlotId`。Bridge 绑定 Snapshot 时只用 `EncounterId + EnemySlotId + PartSlotId`。
- 敌人视觉有两条正式制作模式。普通小怪走 Host 整体视觉：Host 的 `Presentation|Host Visual` 生成一个无碰撞 `UPaperSpriteComponent` 或 `UPaperFlipbookComponent`，子 PartActor 只负责身份、`HitBounds`、bridge、预测和状态 Badge；精英 / Boss 走 PartActor 独立 `VisualLayers`，每个部位可以有自己的多层图和局部 flipbook。
- PartActor 同时是“规则部位 + 命中盒 + 可选视觉层容器”：`HitBounds` 决定 hover、点击和拖卡命中；Host visual、`VisualLayers`、sprite 尺寸、透明区域和排序都不改变目标身份、透明区域命中或 `BattleSession` 规则。
- 当 Host 整体视觉有效，且 PartActor 没有 `VisualLayers` 时，PartActor 会进入 `HitOnly` 视觉模式：自身不生成可见体，缺少独立部位图不算错误，`HitBounds`、InteractionTarget、WorldTargetBridge、prediction/status Badge 仍正常工作。
- `VisualLayers` 非空时，PartActor 会在 `VisualLayersRoot` 下按层生成 `UPaperSpriteComponent` 或 `UPaperFlipbookComponent`，并优先于 Host 的 `HitOnly` 语境。`VisualLayers` 为空且没有 Host 整体视觉时进入 `None / MissingVisualResource` 诊断，该部位只有命中体和调试信息可见。
- 每个视觉层通过 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`。静态层使用 `Sprite`；Flipbook 层使用 `Flipbook`，并可配置 `FlipbookPlayRate`、是否循环、初始播放时间和是否自动播放。普通小怪整体 idle 可以先用 Host Flipbook；精英 / Boss 的局部 idle / 摆动推荐先用 PartActor Flipbook；复杂动画状态机后续再接 PaperZD / Animator，不进入 Battle 规则层。
- `SortOrder` 映射到生成组件的 `TranslucentSortPriority`，`Tint.A` 作为透明度；缺少当前模式对应资源的层不会生成组件，但会进入 debug / validation。`UWacomBattleEnemyPartPresentationComponent` 的反馈缩放优先作用到 PartActor 的 `VisualLayersRoot`，因此 hover、drag preview、伤害和确认 cue 会缩放对应部位视觉组，不缩放 Host 整体图。
- Host 整体视觉和 PartActor `VisualLayers` 都会在构造 / Details 刷新 / 显式刷新时重建，并在 PIE / runtime `BeginPlay` 再刷新一次，保证蓝图视口中的配置能在实机运行中生成同样的 Paper 组件。生成组件只在 Actor 拥有有效 `World` 时注册；蓝图模板、CDO 或无 World 的 Details 预览场景只更新配置与诊断，不创建已注册运行时组件。
- Details 制作分组以正式路径为主：Host 的 `Identity` 配 `EnemyDefinition / EnemySlotId`，`Presentation|Host Visual` 配普通怪整体图，PartActor 的 `Identity` 配 `PartId / PartSlotId`，`Collision` 配 `HitBoundsExtent`，精英 / Boss 独立美术配 `Visual Layers`。PartActor 不再提供旧 StaticMesh 原型可见体入口。
- Host / PartActor Details 都有 `Authoring Status` 只读诊断区，并和 debug view / summary 使用同一套事实。Host `AuthoringState=Ready` 表示定义、部位和 slot 对齐，并显示 Host visual mode / 生成组件数 / 已注册组件数 / 可见组件数；PartActor `VisualAuthoringMode=VisualLayers` 表示正式 2D 层路径，`HitOnly` 表示由 Host 整体视觉承载显示，`None` 表示没有独立视觉也没有 Host 整体视觉，并显示 VisualLayers 的生成 / 已注册 / 可见组件数。排查编辑器配置时先看 Details `Authoring Status`，再看 `UnknownPartSlotIds / MissingDefinitionPartSlotIds / DuplicatePartSlotIds`；需要完整一行串时再执行日志按钮。
- 子 PartActor 顺序只影响 Host registry 和 badge stagger 表现，不改变 `UBattleSession` 的规则部位顺序。
- BattleHUD 不再保留敌方 2D fallback；缺 Host 时不会注册场景敌人目标，且 `EncounterDefinition` 正式入口缺 Host 会被编辑器验证判为 invalid。Trigger debug view / summary 会报告 `MissingSceneEnemyHostSlotIds` 和 `ExtraSceneEnemyHostSlotIds`，用于排查同步后仍需手动补 Host 或删除多余 slot 的情况。

Debug 蛇 Host 样例流程：创建一个 `AWacomBattleEnemyActor` Host 蓝图，在 Host 蓝图视口中放置三个 `AWacomBattleEnemyPartActor` 子 Actor / ChildActorComponent，命名建议包含 `Head / Body / Tail`；然后在 Host 上执行 `ConfigureDebugSnakeHostSample()`。该入口会绑定 `DA_Enemy_Snake`，扫描运行时子 Actor 和蓝图 ChildActorComponent 的子 Actor 模板，把已有三个部位配置为 `Snake.Head / Snake.Body / Snake.Tail`，局部槽位为 `Head / Body / Tail`，并写入示例相对位置和 badge stagger。它不会自动生成缺失部位 Actor，也不会修改 `BattleSession` 或创建正式美术资产；缺失部位会继续通过 Details `Authoring Status`、debug view 和 validation 暴露。`AuthoringDebugSummary` 是 Details 里的只读摘要缓存；需要写入日志时，在 Host Details 中执行 `LogBattleSceneEnemyDebugSummary()` 后，到 Output Log 搜索 `[WacomBattleEnemyActor]` 查看。

蛇 Host prefab 的正式配置不依赖样例按钮：推荐资产路径 `/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug`，Host `EnemyDefinition` 绑定 `DA_Enemy_Snake`，`EnemySlotId=Enemy`，普通蛇美术在 Host `Presentation|Host Visual` 配整体 sprite / flipbook，子部位分别使用 `PartId=Snake.Head / Snake.Body / Snake.Tail` 和 `PartSlotId=Head / Body / Tail`。第一版推荐相对位置是 Head `(96,-6,16)`、Body `(0,0,0)`、Tail `(-92,16,-8)`；推荐 `HitBoundsExtent` 分别是 Head `(42,38,42)`、Body `(62,46,42)`、Tail `(48,34,34)`。普通小怪不需要给每个 PartActor 配独立视觉层；`PartId` 是静态部位定义 ID，`PartSlotId` 是敌人内局部槽位 ID；Host validation 会对两者分别生成 debug / warning，避免 `PartId` 正确但 `PartSlotId` 无法绑定运行时 slot 的隐性错误。

Host 的 `RefreshBattleEnemyPartAuthoringState()` 是安全刷新入口：它刷新 Host visual，扫描自身 PartActor，并同步 `EnemySlotId`、Host visual 语境、Badge 诊断和 debug summary。它不会根据 `EnemyDefinition`、组件名或 Actor 名自动补齐 `PartId / PartSlotId`；缺失身份会保持 `MissingIdentity`，错误身份仍由 `Authoring Status` / Validate Map 暴露。

单蛇 BattleTrigger 的正式配置：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`，执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。后续多敌人 Encounter 配置沿用同一路径：`EncounterDefinition` 指向 Encounter 资产，执行同步按钮生成 slots，逐项填写 `EnemySlotId -> SceneEnemyHost`，并确保 `EnemySlotId` 与 `EncounterDefinition.EnemySlots[].EnemySlotId` 一致；Validate Map 会阻止缺 Host、漏 slot 或多余 slot 的 Encounter Trigger。

`UWacomBattleEnemyPartWorldTargetBridgeComponent` 只负责目标绑定：保存 `EncounterId / EnemySlotId / PartSlotId`，从 `FBattleSnapshot` 解析当前 `PartInstanceId`，记录是否进入 BattleHUD 当前 Host registry，并把 runtime id / slot identity 写入同 Actor 的 `UWacomInteractionTargetComponent`。它不播放 hover、drag preview、伤害、确认或破坏表现。

`UWacomBattleEnemyPartPresentationComponent` 负责部位表现：接收 hover probe、TargetSelect pending card 预测、first-person drag preview、`TargetConfirmed`、`DamageDealt`、`EnemyPartHpEmptied` cue，以及预测 Widget / 常驻状态 Badge 的 runtime facts。它只更新表现 / debug，不修改 `BattleSession`。

BattleHUD scene enemy coordinator 扫描当前 Host registry 时会把每个 PartActor 的 Bridge 与 Presentation 成对缓存：Bridge 用于判断 handle 是否属于当前战斗目标，Presentation 用于注册 BattlePresentation target、播放 cue、显示 targetable affordance、hover probe 和 drag preview。PartActor / Host 的综合 debug summary 可以同时展示 Bridge 绑定事实和 Presentation 表现事实，但这只是 PIE 排查入口，不改变职责边界。

Bridge 绑定成功后会把 `PartInstanceId` 写入 `UWacomInteractionTargetComponent.TargetId`，把 `PartId` 写入 `StableTargetId`，并把 `EncounterId / EnemySlotId / PartSlotId` 写入 handle 的 Battle slot identity 字段。First-person world drop 使用完整 handle 提交，Battle validation 会拒绝 runtime id 与 slot identity 不一致的目标。BattleHUD 判断 handle 是否属于当前 scene enemy registry 时只按完整 slot identity 匹配，不再通过 `SourceObject` 反查 Bridge 兜底；`SourceObject` 只保留为命中来源和调试弱引用。

Battle scene target click / probe 的正式实现位于 App-private `FWacomBattleSceneInteractionRouter`。`AWacomPlayerController::TryRouteBattleSceneTargetClick()` 等 public 方法只作为输入 façade 保留，并继续提供 protected trace / flow seam 给自动化测试。Router 通过 cursor trace 命中 Component，经共享 `WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit()` 扫描 `IWacomInteractionTargetProvider` 构建 handle；只有 `TargetKind=World`、`TargetTag=Interaction.Target.Battle.EnemyPart` 且 handle 属于当前 BattleHUD Host registry 时，才转发为 Battle enemy part 点击。

## §6 Run Menu Zone Target

`UWacomRunMenuDropTargetWidget` 是 Run GameMenu 专用 UMG Zone target bridge：

- 配置 `ZoneId / StableTargetId`。
- 构建 `FWacomInteractionTargetHandle(TargetKind=Zone)`。
- 提供 probe / invalid / released-probe / submit-ready / submitted 轻量 preview。
- 不直接调用 `URunSession`，也不参与背包旧 UMG DragDrop 规则提交。

`AWacomPlayerController` 只负责把 menu drop target widget 注册转发给 App-private Run card drop coordinator，并在创建 coordinator 时用显式 context contract 提供 RunSession、active GameMenu、Anchor、toast 和 world target 查询能力。Coordinator 只在 Exploration + active GameMenu + active menu lease 的 first-person card drag 中扫描注册过的 drop target widget，按后注册优先作为最上层命中。

`FWacomRunFirstPersonCardDropCoordinator` 统一解析 preview 和 release，并把 menu zone / world receiver 作为两个 target adapter 路由：

- 默认是 probe-only。
- owning menu 可以声明 `SubmitZoneTarget + ControllerDestroyOwnedCard`，用于 prototype 验证。
- `MenuHandled` 由菜单提交并回填结果；RunEventScreen 使用该模式提交卡牌支付事务。

## §7 Target Handle

`FWacomInteractionTargetHandle` 位于 `WacomCore/Public/Types/WacomInteractionTargetTypes.h`，是纯数据描述层：

| 字段 | 用途 |
|---|---|
| `TargetKind` | None / World / Card / Zone |
| `WorldTargetId` | World 目标运行时 FGuid |
| `CardInstanceId` | Card 目标的精确实例 id |
| `ZoneId` | UMG Zone 目标 FName |
| `TargetTag` | 目标语义 tag，例如 `Interaction.Target.Battle.EnemyPart` |
| `StableTargetId` | 稳定 authored/data id，例如 enemy part `PartId` |
| `EncounterId / EnemySlotId / PartSlotId` | Battle enemy part 的规则槽位身份；其它 World / Card / Zone 目标保持 None |
| `SourceObject` | 命中来源 Component 弱引用；Battle enemy registry 归属不使用它兜底 |
| `WorldLocation / ScreenPosition` | 命中位置 |

Battle 已接入 `UBattleSession::ValidateTargetWithCard()`，用于 TargetSelect、first-person drag/drop world target 和 hand-card target validation。UI 只读取 validation result，不直接解析 BattleState。

命中结果转 `FWacomInteractionTargetHandle` 的组件扫描已收口到 App-private `WacomInteractionTargetHitResolver`，供 Battle scene target 和 Run world target probe 复用。通用 Run target resolver 尚未独立抽象；当前 Run world / Run menu drop 由 App-private Run card drop coordinator 识别 target adapter，再进入明确的 RunSession 事务或 owning menu submit policy。

## §8 Debug / Development Entry Points

- Debug summary / debug view / log 入口只用于 PIE 排查，不作为正式规则入口。
- Actor `ConfigureDebug...Sample` 按 `Wacom|...|Prototype` 分类保留，只用于开发验证样例配置。
- 旧 Battle event log、旧 2D hand 和旧敌方 2D fallback 已删除；正式手牌走 first-person card layer，正式敌方目标走 SceneEnemyHost / PartActor / WorldTargetBridge。
- `TryRouteBattleSceneTargetClick` 等 protected test seam 保留给自动化测试；测试侧应通过 `WacomTests/Private` access wrapper 使用，不扩散生产 public API。
