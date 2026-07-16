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
- 当 Actor 绑定 Logical Map Node 时，上述 Definition 是复用现有 Trigger / Screen flow 的 façade mirror；`UWacomFloorMapDefinition` typed payload 是规则真相，Scene Validator 会阻止两者不一致。
- 带 `UWacomRunMapNodeBindingComponent` 的世界交互 Actor 只有在绑定 `NodeId` 已成为 Run Snapshot 的当前节点，且节点类型与绑定类型一致时才参与 E 键、hover 和左键路由。未抵达或已经离开的节点 Actor 不显示可交互高亮、不消费点击，也不得启动 Shop / RunEvent 等镜头 staging。没有该绑定的独立原型 Actor 暂时保持原有直接交互语义。
- `ABattleTriggerActor` 必须通过 `EncounterDefinition` 进入战斗，并用 `SceneEnemyHostSlots` 完成 Encounter enemy slot 到场景 Host 的映射。
- `AWacomShopTriggerActor` 可选配置 `ShopEntryViewpoint`。配置后只改变 App 层镜头 / hand staging：玩家先移动到商店第一人称 View Pose，再打开 ShopScreen；关闭后先退 UI，再回当前 Run Path View。它不改变 `PersistentId`、`ShopDefinition`、`Offers`、库存持久化或购买规则。
- `AWacomRunEventTriggerActor` 可选配置 `RunEventEntryViewpoint`。配置后只改变 App 层镜头 / hand staging：玩家先移动到事件第一人称 View Pose，再打开 RunEventScreen；关闭后先退 UI，再回当前 Run Path View。已完成事件仍只显示完成提示，不会触发 staging；该字段不改变 `PersistentId`、`EventDefinition`、完成状态或选项结算规则。
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
- `FWacomRunWorldInteractionRouter` 会校验 handle、source actor、当前 Map Node 资格、clickable contract、world interactable contract 和 bridge，再把左键命中转回现有 `TryInteract()`。节点资格拒绝发生在 Actor 交互入口和任何 viewpoint staging 之前；E 键候选和直接 `TryInteractWithActor()` 使用同一资格 helper。`AWacomPlayerController` 只保留输入 façade、timer、trace seam、E 键 candidate list 和 debug / WBP 入口。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，Run world hover/click 不穿透场景。菜单 first-person card drag/drop 正在处理时，也会清理普通 Run world hover prompt 和 probe preview。

`UWacomRunWorldClickableInteractable` 提供 hover prompt 和通用 debug view。常见 reject reason 包括缺少 world interactable contract、缺少 clickable contract、缺少 bridge、目标 tag 不匹配或当前 flow 不允许交互。

## §4 Run World Card Drop

没有 active GameMenu / menu lease 时，探索期 first-person 卡牌可以拖到 Run world target。

提交要求：

- release 命中 `Interaction.Target.Run.Object`。
- 目标 Actor 有 `UWacomRunWorldCardDropReceiverComponent`。
- receiver 提供正向筛选：`AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 至少一个非空。
- `URunSession::ValidateRunWorldCardInteraction()` 接受精确 `SourceCardInstanceId`。

成功提交由 `SubmitRunWorldCardInteraction()` 返回显式 `FRunTreasureSettlementResult`：在同一 working-state 事务内可选消耗源卡、发放 Gold / Card、写完成 ID、消费 1 Action Point 并 Resolve 当前 Treasure 节点，最后最多广播一次 Run state changed。校验失败或重复提交为 0 成本且全部状态不变；receiver 必须显式检查 result，不得只依赖粗粒度广播猜测成功。

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
- 每个 PartActor 必须有静态 `PartId` 用于内容校验和 debug；制作时主要填写稳定 `PartSlotId`，再由 Host 的显式定义同步从对应 `PartDefinition.PartId` 派生 `PartId`。运行时目标绑定仍只由 `EncounterId + EnemySlotId + PartSlotId` 解析并写回当前 `PartInstanceId`。
- `SceneEnemyHostSlots.EnemySlotId` 是 Host 的运行时敌人槽身份；空 `EnemySlotId` 是摆放错误，不做旧单 Host 兜底。
- PartActor 的 `PartSlotId` 是 Host 内局部槽位 ID，必须显式填写并对应 `EnemyDefinition.Parts[].PartSlotId`。Bridge 绑定 Snapshot 时只用 `EncounterId + EnemySlotId + PartSlotId`。
- Host 的 `HostAuthoringMode` 明确两条正式制作模式。`SimpleHostVisual` 用于普通小怪：Host 的 `Presentation|Host Visual` 生成一个无碰撞 `UPaperSpriteComponent` 或 `UPaperFlipbookComponent`，子 PartActor 只负责身份、`HitBounds`、bridge、预测和状态 Badge；`MultiPartVisualLayers` 用于多部位精英 / Boss：每个 PartActor 可以有自己的多层图和局部 flipbook。模式只控制制作诊断，不会切换时清空 Host visual 或 `VisualLayers`。
- PartActor 同时是“规则部位 + 命中盒 + 可选视觉层容器”：`HitBounds` 决定 hover、点击和拖卡命中；Host visual、`VisualLayers`、sprite 尺寸、透明区域和排序都不改变目标身份、透明区域命中或 `BattleSession` 规则。
- 当 Host 整体视觉有效，且 PartActor 没有 `VisualLayers` 时，PartActor 会进入 `HitOnly` 视觉模式：自身不生成可见体，缺少独立部位图不算错误，`HitBounds`、InteractionTarget、WorldTargetBridge、prediction/status Badge 仍正常工作。
- `VisualLayers` 非空时，PartActor 会在 `VisualLayersRoot` 下按层生成 `UPaperSpriteComponent` 或 `UPaperFlipbookComponent`，并优先于 Host 的 `HitOnly` 语境。`VisualLayers` 为空且没有 Host 整体视觉时进入 `None / MissingVisualResource` 诊断，该部位只有命中体和调试信息可见。
- 每个视觉层通过 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`。静态层使用 `Sprite`；Flipbook 层使用 `Flipbook`，并可配置 `FlipbookPlayRate`、是否循环、初始播放时间和是否自动播放。普通小怪整体 Idle 使用 Host Flipbook；Simple Flipbook Host 可以另外配置 `UWacomBattleEnemyHostAnimationStyle`，用默认 Action、显式 `IntentId -> Clip` 和 Destroyed 描述轻量语义动画。显式映射优先且不做名称推断；精英 / Boss 的局部 idle / 摆动仍使用 PartActor Flipbook，Part/PaperZD 复杂状态机后续再做且不进入 Battle 规则层。
- `SortOrder` 映射到生成组件的 `TranslucentSortPriority`，`Tint.A` 作为透明度；缺少当前模式对应资源的层不会生成组件，但会进入 debug / validation。`UWacomBattleEnemyPartPresentationComponent` 的持久缩放只作用到 PartActor 的 `VisualLayersRoot`，优先级为 Drag Preview、Targetable、Hover、authored base scale；确认、伤害和破坏 Cue 不再修改缩放，也不会缩放 Host 整体图。
- 每个 PartActor 都有原生 `ImpactAnchor`，默认位于 `HitBounds` 中心；`ImpactAnchorRelativeLocation` 以厘米为单位，允许普通 HitOnly 小怪和 VisualLayers 部位分别微调未来命中特效的生成点。锚点无碰撞、不可命中，不进入 target handle 或 BattleSession。表现解析优先使用有效 ImpactAnchor，再回退 HitBounds / Actor 根节点和 Owner 世界位置；非有限偏移会被 Data Validation 拒绝。
- Host 整体视觉和 PartActor `VisualLayers` 都会在构造 / Details 刷新 / 显式刷新时重建，并在 PIE / runtime `BeginPlay` 再刷新一次，保证蓝图视口中的配置能在实机运行中生成同样的 Paper 组件。生成组件只在 Actor 拥有有效 `World` 时注册；蓝图模板、CDO 或无 World 的 Details 预览场景只更新配置与诊断，不创建已注册运行时组件。
- Details 制作分组以正式路径为主：Host 的 `Identity` 配 `EnemyDefinition / EnemySlotId`，`Authoring` 选择 `HostAuthoringMode`，并在 `Authoring Report` 中点击“从 EnemyDefinition 同步部位”；`Presentation|Host Visual` 配普通怪整体图，`Presentation|Host Animation` 配语义动画 Style。PartActor 的 `Identity` 主要维护 `PartSlotId`，`PartId` 由 Editor service 从定义派生，`Collision` 配 `HitBoundsExtent`，精英 / Boss 独立美术配 `Visual Layers`。PartActor 不再提供旧 StaticMesh 原型可见体入口。
- Host Details 的 `Authoring Report` 是实时纯只读结果，并和 Host validator、debug view / summary 使用同一套 evaluator。Host `AuthoringState=Ready` 表示定义、部位和 slot 对齐；报告同时列出待新增、待修正、`UnknownPartSlotIds / MissingDefinitionPartSlotIds / DuplicatePartSlotIds / PartDefinitionMismatchSlotIds / SurplusPartActorNames` 与无效定义槽位。求值和 Validate 永远不刷新视觉、不写 identity、不创建组件且不 dirty package。PartActor 仍显示自己的 `VisualAuthoringMode=VisualLayers / HitOnly / None` 和组件统计。
- 子 PartActor 顺序只影响 Host registry 和 badge stagger 表现，不改变 `UBattleSession` 的规则部位顺序。
- BattleHUD 不再保留敌方 2D fallback；缺 Host 时不会注册场景敌人目标，且 `EncounterDefinition` 正式入口缺 Host 会被编辑器验证判为 invalid。Trigger debug view / summary 会报告 `MissingSceneEnemyHostSlotIds` 和 `ExtraSceneEnemyHostSlotIds`，用于排查同步后仍需手动补 Host 或删除多余 slot 的情况。

Debug 蛇 Host 样例流程：创建一个 `AWacomBattleEnemyActor` Host 蓝图，在 Host 蓝图视口中放置三个 `AWacomBattleEnemyPartActor` 子 Actor / ChildActorComponent；单选 Host，在 Details 的 `Advanced Debug` 折叠区点击“配置 Debug 蛇样例”。Editor-only 操作使用独立事务，绑定 `DA_Enemy_Snake`，扫描 live 子 Actor 和 ChildActor template，把已有三个部位配置为 `Snake.Head / Snake.Body / Snake.Tail`、局部槽位 `Head / Body / Tail`，并写入示例相对位置和 badge stagger。它不会自动生成缺失部位 Actor，不修改 `BattleSession`，也不创建正式美术资产；缺失部位由实时 Authoring Report、debug view 和 validation 暴露。Host 不再保存 `AuthoringDebugSummary` 或“当前 Authoring Status”缓存；完整运行时摘要由 debug view 即时生成。

蛇 Host prefab 的正式配置不依赖样例按钮：推荐资产路径 `/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug`，Host `EnemyDefinition` 绑定 `DA_Enemy_Snake`，`EnemySlotId=Enemy`，`HostAuthoringMode=SimpleHostVisual`；在 Host Details 点击“从 EnemyDefinition 同步部位”生成 Head / Body / Tail 后，普通蛇美术在 Host `Presentation|Host Visual` 配整体 sprite / flipbook。第一版推荐相对位置是 Head `(96,-6,16)`、Body `(0,0,0)`、Tail `(-92,16,-8)`；推荐 `HitBoundsExtent` 分别是 Head `(42,38,42)`、Body `(62,46,42)`、Tail `(48,34,34)`。普通小怪不需要给每个 PartActor 配独立视觉层；`PartId` 是静态部位定义 ID，`PartSlotId` 是敌人内局部槽位 ID；Host validation 会对两者分别生成 debug / warning，避免 `PartId` 正确但 `PartSlotId` 无法绑定运行时 slot 的隐性错误。

Host Details 的“从 EnemyDefinition 同步部位”是通用、显式、可重复执行的制作入口。`WacomEditor` 先为全部选中 Host 求值纯同步计划，再在一次事务中应用；它只用显式 `PartSlotId` 匹配 `EnemyDefinition.Parts`：唯一匹配的已有 PartActor 从 `PartDefinition.PartId` 派生或修正 `PartId`，缺失槽位新增零相对变换和 PartActor 默认 `HitBounds / ImpactAnchor / VisualLayers` 的 ChildActorComponent。Blueprint template 写入 SCS，新增节点标记 structural modified；关卡 Host 实例新增 runtime-safe `UWacomBattleEnemyPartChildActorComponent` 作为 transactional InstanceComponent，由组件在加载或 Undo/Redo 后向重建的 PartActor 重放明确身份，不反向修改来源 Blueprint，也不根据组件名猜身份。已有部位的位置、`HitBoundsExtent`、`ImpactAnchorRelativeLocation` 和 `VisualLayers` 永不被同步覆盖。空、未知或重复占用槽位的 PartActor 进入 surplus，原样保留；无效定义槽位跳过并记录到 Last Sync。无实际变化不创建事务、不 dirty package，失败不会伪装成 `NoChanges`；PIE/game world 拒绝写入并报告 `EditorOnly`。Actor 不再暴露旧同步 UFUNCTION。

Host 的 `RefreshBattleEnemyPartAuthoringState()` 仍是 Construction / 显式表现刷新入口：它刷新 Host visual、扫描 PartActor 并更新 Part 表现状态，但不参与 Validator 或纯 Authoring Report 求值，也不进入 runtime Snapshot sync。它不会创建 PartActor，不会根据 `EnemyDefinition`、组件名或 Actor 名自动补齐 `PartId / PartSlotId`；需要生成或派生时必须由内容人员在 Details 显式执行 Editor 同步。

单蛇 BattleTrigger 的正式配置：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`，执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。后续多敌人 Encounter 配置沿用同一路径：`EncounterDefinition` 指向 Encounter 资产，执行同步按钮生成 slots，逐项填写 `EnemySlotId -> SceneEnemyHost`，并确保 `EnemySlotId` 与 `EncounterDefinition.EnemySlots[].EnemySlotId` 一致；Validate Map 会阻止缺 Host、漏 slot 或多余 slot 的 Encounter Trigger。

`UWacomBattleEnemyPartWorldTargetBridgeComponent` 只负责目标绑定：保存 `EncounterId / EnemySlotId / PartSlotId`，从 `FBattleSnapshot` 解析当前 `PartInstanceId`，记录是否进入 BattleHUD 当前 Host registry，并把 runtime id / slot identity 写入同 Actor 的 `UWacomInteractionTargetComponent`。它不播放 hover、drag preview、伤害、确认或破坏表现。

`UWacomBattleEnemyPartPresentationComponent` 负责部位表现：接收 hover probe、TargetSelect pending card 预测、first-person drag preview、`TargetConfirmed`、`DamageDealt`、`EnemyPartHpEmptied` cue，以及预测 Widget / 常驻状态 Badge 的 runtime facts。Cue 被收敛为 App-private 互斥 Playback，按 `Destroyed > Damage > TargetConfirmed` 决定覆盖并输出统一进度；只有 Playback 真正接受确认或伤害 Cue 后，App-private impact controller 才在 `ImpactAnchor` 下启动并复用一个 Niagara Component。拖卡具体悬停部位则使用独立 Preview Playback 与第二个复用 Niagara Component：有效目标显示覆盖 `HitBounds` 的完整四角像素框和中心菱形，无效目标显示暗紫红断裂框；它不播放声音、不提交命令，也不复用旧 Drag Preview 缩放。PartActor 同时把 `HitBounds` 作为只读表现尺寸源提交给 Presentation：运行时按当前摄像机 Right/Up 投影计算命中直径或预演宽高，不会按整个敌人 Host 的尺寸放大。它只更新表现 / debug，不修改 `BattleSession`。

世界像素命中特效的制作入口位于 Host/Part：Host 的 `DefaultImpactStyle` 为全部部位提供默认值，Part 的 `ImpactStyleOverride` 可覆盖，`bEnableImpactFeedback` 可单独关闭。Style 的 Niagara System 和 MI 有效时，确认表现为角标向锚点收束，伤害表现为中心方印、菱形环和短程碎片；资源无效时 Cue 生命周期和目标路由仍继续，不能因为表现缺失阻塞命令。`Destroyed` 当前只保持 Cue 优先级，不播放旧缩放或新粒子。摄像机平面轴、Reduced Motion 与装饰闪光强度通过 Niagara User 参数逐 Cue 快照，锚点、特效和声音都不进入 target handle 或规则身份。

拖卡目标预演使用独立制作入口：Host 的 `DefaultTargetPreviewStyle` 为全部部位提供默认值，Part 的 `TargetPreviewStyleOverride` 可覆盖，`bEnableTargetPreviewFeedback` 可单独关闭。Style/System/MI 缺失时仍保留原有 `ValidWorldTarget / Invalid` 状态和预测 Badge；鼠标切换部位时旧 Preview 约 `0.10s` 淡出，新 Preview 约 `0.18s` 收束进入。成功释放后 Preview 与权威 TargetConfirmed 使用不同组件，允许短暂视觉交接，但预演本身绝不代表已经命中或造成伤害。

`NS_WacomBattleEnemyPartImpact_Pixel` 的四个 Emitter Stack 由 WacomEditor-only `WacomBuildBattleEnemyPartImpactNiagara` 生成器维护，内容制作人员不需要手工填写 Burst、粒子 Spawn/Update 或 Dynamic Material Parameter。可安全手调的入口只有：PartActor 的 ImpactAnchor 偏移、Host/Part 的 Impact/TargetPreview Style 引用、Style 的强度/节奏/覆盖/摄像机偏移/声音，以及对应 MI 的颜色和像素外观。重新生成 Niagara 不会修改命中盒、target handle、Battle 身份或规则数据。

BattleHUD scene enemy coordinator 扫描当前 Host registry 时会把每个 PartActor 的 Bridge 与 Presentation 成对缓存：Bridge 用于判断 handle 是否属于当前战斗目标，Presentation 用于注册 BattlePresentation target、播放 cue、显示 targetable affordance、hover probe 和 drag preview。PartActor / Host 的综合 debug summary 可以同时展示 Bridge 绑定事实和 Presentation 表现事实，但这只是 PIE 排查入口，不改变职责边界。

Host registry 建立时执行一次 runtime scene binding：Host 扫描 live PartActor，注入 `EnemySlotId`、Host visual context、默认 Impact/TargetPreview Style 和稳定 Badge stagger；这一入口不重建 Host visual、PartActor `VisualLayers` 或 Authoring Status。PartActor `BeginPlay / EndPlay` 和显式 runtime attach/detach 通过 Host topology revision 标记真实拓扑变化。Coordinator 在普通 Snapshot 中只比较 revision 与弱引用有效性；revision 未变化时不扫描 ChildActor、不重建 registry，也不注销/重注册 Presentation target。

`ABattleTriggerActor.BuildBattleSceneEnemyHosts()` 是 runtime preparation，不是 Authoring refresh：它通过无副作用的内部解析按 Encounter 顺序选择有效且未销毁的 Host，重复 Host 只采用 Encounter 顺序中的第一项，然后仅把对应 `EnemySlotId` 注入 Host。它不触发 `RefreshBattleEnemyPartAuthoringState()`；PartActor 的 runtime identity 由后续 HUD registry 初始化统一注入。Trigger Debug View 使用同一纯解析结果且不写 Host。Coordinator 会同时观察 Host 的 `EnemySlotId` 和 topology revision；同一 Host 指针改变槽位身份时会重建一次 registry，身份和拓扑都未变化时继续复用现有注册。

普通 Snapshot 同步只允许更新 Bridge Snapshot binding、InteractionTarget runtime id、Presentation runtime facts、targetable 和 EnemyPanel/hover/prediction/cue 状态。视觉生成仍只属于构造、Details/显式 Authoring 刷新和 runtime 初始化；Host Flipbook 播放进度与 PartActor VisualLayer 组件不能因为 Snapshot 刷新被重置。`EnemyPartActed Count > 0` 和最新 Snapshot 已确认的整体破坏可以通过 presentation queue 按稳定 `EnemySlotId` 请求 Host 语义动画；播放只原地切换现有 Flipbook 组件，不改变 target handle、HitBounds、Part identity 或规则事件顺序。

BattleEnd Snapshot 到达时立即清理 Bridge binding、Presentation target、hover、drag、panel 和 targetable，确保已死亡敌人不再可交互；coordinator 只临时保留 Host 弱引用和最终 `bAllPartsDestroyed` facts，让队列中最后一个 Destroyed Clip 可以完成。`BattleEndSignal`、source/session clear、HUD shutdown 或 Host 销毁会取消旧 barrier 并清空 retiring Host；重新进入战斗后由当前 Host registry 建立新绑定并恢复 Idle。Host/Part 真正销毁或 topology revision 变化仍按稳定部位 identity 清理或重建 registry，但普通重建不重置正在播放的 Clip。

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

命中结果转 `FWacomInteractionTargetHandle` 的组件扫描已收口到 App-private `WacomInteractionTargetHitResolver`，供 Battle scene target 和 Run world target probe 复用。Run world hover / click / probe 的 handle 校验和 clickable resolver 已收口到 App-private `FWacomRunWorldInteractionRouter`；Run world card drop / Run menu drop 仍由 App-private Run card drop coordinator 识别 target adapter，再进入明确的 RunSession 事务或 owning menu submit policy。

## §8 Debug / Development Entry Points

- Debug summary / debug view / log 入口只用于 PIE 排查，不作为正式规则入口。
- Actor `ConfigureDebug...Sample` 按 `Wacom|...|Prototype` 分类保留，只用于开发验证样例配置。
- 旧 Battle event log、旧 2D hand 和旧敌方 2D fallback 已删除；正式手牌走 first-person card layer，正式敌方目标走 SceneEnemyHost / PartActor / WorldTargetBridge。
- `TryRouteBattleSceneTargetClick` 等 protected test seam 保留给自动化测试；测试侧应通过 `WacomTests/Private` access wrapper 使用，不扩散生产 public API。
