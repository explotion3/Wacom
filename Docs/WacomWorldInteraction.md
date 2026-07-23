---
type: interaction-contract
scope: wacom-world-interaction
status: active
updated: 2026-07-18
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
- Floor 1 Production 灰盒使用真实 Definition 驱动的 6 个 Battle、1 个 Shop、4 个 RunEvent 和 4 个 RewardPickup Host；四个 Navigation 节点没有 content Host。所有实例使用 `Floor.Main.01.<NodeId>` 作为 `PersistentId`，并由 `UWacomRunMapNodeBindingComponent` 映射到对应 typed payload。
- `BP_WacomRunFloorEntranceMarker_Graybox` 不是正式可交互 Actor：它只有无碰撞可见 primitive、Node binding 和实例 `PersistentId`，不实现 `IWacomWorldInteractable`、click/hover bridge、Floor travel 或 Level Blueprint 逻辑。未来 FloorEntrance 交互与 world handoff 归 App flow，marker 不拥有规则。

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

成功提交由 `SubmitRunWorldCardInteraction()` 返回显式 `FRunTreasureSettlementResult`：在同一 working-state 事务内可选消耗源卡、发放 Gold / Card、写完成 ID、消费 1 Action Point 并 Resolve 当前 Treasure 节点，最后最多广播一次 Run state changed。校验失败或重复提交为 0 成本且全部状态不变；receiver 必须显式检查 result，并在成功后把 `ExplorationResolution` 交给 PlayerController 的唯一节点活动表现入口，不得只依赖粗粒度广播猜测成功。

金币 Pickup、固定卡 Pickup、Definition 驱动 RewardPickup 和 KeyChest card-drop receiver 共用同一 WacomApp Treasure 表现同步 adapter。旧的无正式 Floor Session 会返回空版本结果并保持兼容 no-op；正式 Floor 的成功结算若不能按序应用到已绑定的表现 Coordinator，则记录错误并走 PlayerController 的现有场景绑定恢复路径，不能静默留下规则/表现版本漂移。

失败反馈属于 App 表现层：preview 阶段只更新轻量有效 / 无效反馈；release 命中过目标且提交失败时才发 AppToast。文案优先来自 receiver failure contract，没有 receiver 可询问时由 PlayerController 提供通用配置异常 fallback。

KeyChest 是当前第一条 Run world card interaction 验证入口。普通 E 键或左键点击只显示“需要钥匙 / 宝箱已打开”提示，不直接结算奖励；拖卡成功后才提交 RunSession 事务。

## §5 Battle Scene Target

BattleTrigger 仍是 Encounter 与关卡 Host 映射的唯一入口：`EncounterDefinition.EnemySlots` 生成规则初始化，`SceneEnemyHostSlots` 按同一 `EnemySlotId` 顺序绑定 `AWacomBattleEnemyActor`。正式 Trigger 必须覆盖每个有效槽位，不能重复 Host，不能遗漏或添加 Encounter 外槽位；Details 同步按钮只补齐 slot 列表，不猜 Host。

敌人场景目标只有一个制作结构：Host Actor 下直接放 typed Part Component，每个 Part 下直接放 typed Sprite/Flipbook Layer 与 ImpactAnchor。`PartSlotId` 对应 Definition 槽位，`PartId` 由定义派生；Component Transform 和规则 identity 属于纯 `USceneComponent` Part，它不再是 Primitive，也没有 authored BoxExtent。`InteractionVisualLayerId` 精确指定唯一正式交互视觉层，该层 authored Idle Sprite 或 Idle Flipbook 第一帧的 Paper2D 3D ShrinkWrapped BodySetup 是稳定命中真相。禁止 ChildActor、部位 Actor、生成 VisualLayers、Host 整体图和按名称/数组位置推断归属。

`FWacomBattleSceneEnemyHostAuthoring` 在 Editor 中显式同步缺失 Part，并创建默认 `Visual_Main`、匹配的 `InteractionVisualLayerId` 与 ImpactAnchor。它保留所有已有 Transform、Layer、资源和 Anchor，surplus 只报告不删除，不再提供手工命中尺寸制作入口。Validator/Report 永远只读，并拒绝 interaction ID 缺失/歧义、稳定首帧无 3D ShrinkWrapped BodySetup、非 `12cm` thickness 或 authored visual 带碰撞；Blueprint viewport、Compile/Save/Reload 后看到的组件就是运行时使用的组件。

Host 的 `UWacomBattleEnemySceneRuntimeComponent` 按 typed hierarchy 建立 `FPartRuntimeState`。Topology 只在组件注册、注销、销毁或显式通知时变化；Snapshot 不扫描层级。HUD coordinator 直接登记 `UWacomBattleEnemyPartComponent` 与完整 `EncounterId + EnemySlotId + PartSlotId + PartInstanceId`，不再存在 Bridge/Presentation 双 registry。`SetPartRegisteredWithHUD()` 同步刷新碰撞，因此未进入当前 registry 的 Part 不会阻挡专用查询。

Battle 敌人交互独占 `WacomBattleInteraction` trace channel（`ECC_GameTraceChannel1`），不复用 Run 世界交互的 `Visibility` 命中。正式 Sprite/Flipbook interaction collision 和运行时 fallback 都保持 `WorldDynamic`，只 Block 专用通道、Ignore `Visibility` 与其他通道且不产生 overlap。Click、hover 与拖卡 screen-position probe 共用同一二阶段查询：`Visibility` Line Trace 只提供场景遮挡深度；专用通道 Line Trace 取得严格轮廓；失败后再用专用通道执行半径 `8cm` Sphere Sweep。候选必须属于当前 HUD registry 且不能位于遮挡物之后；多候选按屏幕距离、射线深度、稳定 Part identity 决胜。

配置异常时 SceneRuntime 才为符合运行资格的 Part 懒创建并复用一个 transient fallback Box。它不是 authored Visual Layer，不进入 Blueprint SCS、Authoring Report 或资产保存；handle、SourceObject 和稳定 identity 始终转发给 Part。Bounds 优先取可唯一解析 interaction layer 的稳定 Idle 视觉 bounds，其次取 Part 下所有有效直接 typed visual 的稳定 authored bounds 并集，最终为原点 `55×45×55cm` half-extent；每轴最小 `6cm`，不额外加宽。8cm Sweep 仍是唯一输入容错。解绑、Destroyed、BattleEnd、Retire 与 registry 注销立即关闭 fallback，topology 移除和 EndPlay 销毁。

HUD 对每个已接收 Snapshot 只构建一次 Scene Enemy sync frame，并将精确 Part facts 与 targetability 推给对应 typed Part；同一批次不从 Session 再建 Snapshot，也不在每个 Part 内重复扫描敌人数组。相同 facts 与交互状态是 no-op。Hover 射线只按配置间隔执行，稳定目标按 `WorldTargetId + Snapshot.Version + UIState + PendingCardId` 复用 preview；SceneRuntime 不创建世界预测 Widget，规则投影统一交给 Host Enemy Panel，Part 只保留 Niagara target feedback。

Scene Target 的完美释放 / 抵抗提示只消费 `FBattleCardActionPreview`：按稳定部位身份把 Battle 给出的玩家峰值、敌方峰值、比较结果与 `bWillSkipActionDueToStun` 送入 Host Enemy Panel，不得从卡牌 Effect、Intent 资产或卡面文本重算伤害。Enemy Entry 使用金色 Surface、`P > E` / `P ≤ E`、projected 眩晕和变暗加斜线的 Intent 表达；不在 Part 上方显示长文字。失败比较即使没有 Snapshot diff 也需要显示；非目标部位、非攻击意图和没有正伤害 invocation 的部位没有抵抗比较。

指定 Sprite/Flipbook Layer 实现 `IWacomInteractionTargetProvider`，但只转发父 Part 构造的 `Interaction.Target.Battle.EnemyPart` handle；`SourceObject`、Encounter/Enemy/Part identity 与 registry ownership 仍属于 Part。Actor 级 fallback 不为战斗部位修正错误 identity，装饰层保持 `NoCollision` 且不能借 Actor fallback 变成 Part。first-person drag、普通 click、hover probe、Action Preview 与 TargetConfirmed 都消费同一稳定 handle。

Battle scene click、hover 与拖卡释放共用同一个二阶段屏幕查询：先用 `Visibility` Line Trace 取得真实场景遮挡深度，再用 `WacomBattleInteraction` Line Trace 接受严格 Sprite 轮廓；未命中 Part 时沿同一射线和专用通道执行默认半径 `8cm` Sphere Sweep。宽松候选必须属于当前 BattleHUD registry，不能位于遮挡物之后；重叠候选按屏幕 ImpactPoint 距离、射线深度和稳定 Part identity 依次排序。宽松区只改善输入容错，不改变描边、VFX、规则合法性或伤害范围。

Action、Destroyed 与反馈全部原地操作 authored 组件：Part Animation Style 精确选择 `LayerId`；Destroyed 切换同一 Sprite/Flipbook Component，但命中继续使用 authored Idle/第一帧的稳定 BodySetup。SceneRuntime 另从该 authored Idle Sprite / Idle Flipbook 第一帧的 `RenderBounds` 与 interaction visual 世界 Transform 构造 oriented Presentation Bounds：Target Preview 的中心和宽高来自稳定视觉矩形，不读当前动画帧或碰撞 Bounds；它的 transient Niagara 只挂在 Part 下管理生命周期。Damage、TargetConfirmed、Destroyed Niagara 与声音仍落在 typed ImpactAnchor，缺 Anchor 回退 Part 原点，但 Niagara 直径从同一 Presentation Bounds 投影计算，因此调整 Anchor 不会移动选中框，换行动/受击/Destroyed 画面也不会改变框尺寸。配置异常时 Presentation Bounds 独立按 interaction visual authored bounds、直接 typed visual 并集、Part 原点安全 bounds 求值，不要求 transient fallback 已创建。拖出敌人目标卡时，first-person drag source 会参与统一目标选择事实，SceneRuntime 将所有合法部位合成为 `Available` 中心标记；具体 Valid/Invalid Hover 在该部位自己的复用 Niagara Component 内覆盖它，离开悬浮后恢复 Available。同一 Host 下不同部位的动态 Niagara Component 必须使用不同 UObject 身份，禁止用固定名称让多个部位共享组件。合法 TargetSelect 与存活部位悬停使用 interaction visual 下方的 transient padded-quad ring-only 描边；原 Paper2D Component、材质和 scale 不变。代理画布增加最多 `2 source pixels` 的透明留白，C++ 从 Sprite baked XY/UV 解出 Atlas 局部坐标，材质把画布 UV counter-remap 回原源矩形并对每个采样应用 inside mask，禁止采到图集相邻 Sprite。`DShader/Material/World/M_WacomBattleEnemyPartInteractionOutline.dsm` 生成 `/Game/DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline`：可选态只显示一像素暖金内环，悬停态显示亮金内环与较暗第二像素外环，中心主体 Alpha 恒为零，并用轻微后向深度偏移避免 Paper2D 共面竞争。禁止用 C++ 手工 MaterialExpression 图、原 Sprite 整体缩放、全屏 CustomDepth 后处理或整块 silhouette 作为正式描边。非法与终态不显示。目标可用性不再缩放 Host 或 Visual Layer。普通 Snapshot、相同 Host 设置和未变化 topology 不重置播放进度、命中源或组件指针。BattleEnd/source clear/EndPlay 清理动态组件、描边、timer、delegate 与 barrier，但不销毁 authored Visual。

EnemyPanel 是 Host 的 screen-space 被动视图，不是 world target。Idle 时 `InspectHitTarget` 可以上报完整 Part identity；拖卡、TargetSelect、Resolving 与 BattleEnd 时恢复点击穿透，世界 interaction visual 路由继续有效。详情 coordinator 与面板只消费现有 ViewData，不修改 BattleSession。

胜利后的场景退役由 BattleTrigger 承担。GameMode 在成功提交非撤离 Victory 后先禁用 Trigger，等待返回探索镜头与 ExitBattle barrier，再让 Host runtime 清理 Part 表现、隐藏 Host、关闭 Part collision 并销毁 Trigger。Withdraw、Defeat、Undetermined 和结算失败保留 Host；重新进入战斗会从 authored Layer 状态重新初始化。

正式内容示例：TrainingWarrior 是单 Body Part；Snake 是 Head/Body/Tail；SlimeTrio 是 Left/Core/Right。它们使用同一 runtime、target、UI 与 authoring 路径，不再按单部位/多部位切换架构。
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
- 旧 Battle event log、旧 2D hand 和旧敌方 2D fallback 已删除；正式手牌走 first-person card layer，正式敌方目标走 SceneEnemyHost / typed Part Component。
- `TryRouteBattleSceneTargetClick` 等 protected test seam 保留给自动化测试；测试侧应通过 `WacomTests/Private` access wrapper 使用，不扩散生产 public API。
