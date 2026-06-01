---
type: orchestration-spec
scope: wacom-app
status: active
updated: 2026-06-01
tags:
  - wacom/app
  - wacom/gameflow
  - wacom/input
  - wacom/world-interaction
---

# WacomApp 模块文档

> [!info] 本文职责
> 本文是 WacomApp 的 App orchestration 文档。它记录 GameMode、PlayerController、PlayerCharacter、世界交互、输入、UI 总入口和战斗进出流程。

> [!note] UI 边界
> 具体 UI 行为、Screen、Toast、ViewData 和 WBP 承接见 [[WacomUI]]。

## §1 模块职责

`WacomApp` 是表现层和游戏主模块（`PRIMARY_GAME_MODULE`）。

负责：

- GameMode / PlayerController / PlayerCharacter。
- Game flow 状态切换：探索与战斗。
- 输入上下文协调、Enhanced Input 的 IMC 切换与输入入口。
- 世界交互接口和场景 Trigger Actor。
- UI 根布局创建、Push / Pop 入口与战斗 UI 路由。

不负责：

- 单场战斗规则真相，战斗命令必须提交给 `UBattleSession`。
- Run / 背包 / 商店 / 探索事件规则真相，战外命令必须提交给 `URunSession`。
- App / UI 对玩家已拥有卡提交 `InstanceId`，不以 Definition 指代某张具体卡；DataAsset / RunEvent 表达“某种卡”的资产语义时例外。
- 静态数据资产定义。
- WBP 视觉细节和卡牌展示规则；这些见 `WacomUI.md` 与两个 UI Binding 文档。

---

## §2 关卡与 GameMode

| 关卡 | GameMode | 职责 |
|---|---|---|
| `L_MainMenu` | `AWacomMenuGameMode` | 主菜单，不 Spawn 探索 Pawn，提供 New Game / Continue / Quit 入口 |
| `L_Exploration` | `AWacomGameMode` | 探索主流程，持有 GameFlowState，进入/退出战斗，初始化探索 HUD |

当前 GameFlowState：

```cpp
enum class EGameFlowState : uint8
{
    Exploration,
    Battle,
};
```

探索状态允许移动、世界交互、打开菜单/背包/商店/事件界面。战斗状态禁用探索移动，切换到战斗输入，并 Push `UBattleHUD` 到 Game 层。

当前 `AWacomGameMode::bSaveSystemEnabled == false`，自动存档路径会静默 no-op。SaveGame 当前边界见 `WacomRun.md`。

主菜单当前继续使用 `L_MainMenu + AWacomMenuGameMode + UWacomMainMenuScreen`，不新建 V2 地图。菜单切关统一使用 UE package path：

- Exploration：`/Game/Wacom/Maps/L_Exploration`
- MainMenu：`/Game/Wacom/Maps/L_MainMenu`

不要把 travel 目标写成 `/Game/Wacom/Maps/L_Exploration.L_Exploration` 这类 ObjectPath。UE 5.7 PIE 下 ObjectPath travel 曾触发 `FPackagePath::TryFromMountedName was passed an ObjectPath` 和 `!NewPIEWorld->bIsWorldInitialized` ensure，表现为 `L_MainMenu -> 新游戏 -> L_Exploration` 卡顿、Actor 初始化不完整和输入不可用；日志同时确认 `bSaveSystemEnabled=false` 且 RunSession 是新 Run，因此该问题不归因于存档恢复。

---

## §3 PlayerController 与 PlayerCharacter

`AWacomPlayerController` 是探索输入、RunSession 持有者和 UI 请求入口。

核心蓝图资产归档到 `/Game/Wacom/Core`：GameMode 放 `/Game/Wacom/Core/GameModes`，PlayerCharacter 放 `/Game/Wacom/Core/Player`，PlayerController 放 `/Game/Wacom/Core/Controllers`。Run Tunnel authoring 蓝图放 `/Game/Wacom/Run/Tunnel/Blueprints`。`/Game/Wacom/_GAME` 是历史临时目录，不再作为新增资产入口；具体目录规范见 `Docs/Content_Organization.md`。

主要职责：

- BeginPlay 创建并持有 `URunSession`。
- 持有 `UWacomRunFirstPersonCardSourceComponent`，在 Exploration 下把 RunSession 的备战卡组写入 PlayerCharacter 的 first-person card anchor。该 bridge 默认只做展示和 debug，不启用战斗手牌交互；进入战斗或 Controller EndPlay 时清理，退出战斗回到 Exploration 后重新刷新。V0-AL 后 Controller 也负责 GameMenu 对该 source 的默认压制和 menu lease 转发。V0-AN 后 Controller 提供 `SetRunFirstPersonCardLayerMenuLeaseFromRunCards()` wrapper，菜单可以用 Blueprint-friendly request 从玩家真实持有卡中筛候选卡，而不是直接构造 first-person entry。V0-AO 后 Controller 还负责 Run menu card drop intent：只有 active menu lease 的 owning menu 把 Zone/card 解析为 `SubmitZoneTarget` 时，释放到 Zone 才可提交。默认 prototype 菜单使用 `ControllerDestroyOwnedCard` policy，由 Controller 调 `URunSession::DestroyCardByInstance()`；RunEventScreen 使用 `MenuHandled` policy，提交时调用 `ChooseRunEventOptionWithPaidCardResult()` 完成事件事务并回填结果。
- 提供 `IMC_Exploration` 与 `IMC_Battle` 的 Push / Pop helper；正式战斗进出时由 GameMode 调用这些 helper。
- 处理 BeginPlay 初始探索 IMC 和 PIE / 切关卡后的兜底恢复。
- 处理探索交互、暂停菜单、背包、商店、RunEvent 打开请求；PlayerController 只发起请求并持有必要上下文，背包 / 商店 / RunEvent 的 GameMenu 打开细节和外部返回清理由私有 `FWacomExplorationScreenRouter` 承接。
- 转发战斗快捷键到当前 BattleHUD / BattleSession。

`AWacomPlayerCharacter` 是第一人称探索 Pawn：

- 使用 `UCameraComponent` 和 `UCharacterMovementComponent`。
- 探索 IMC 下响应 WASD 和鼠标视角。
- 持有 `UWacomCursorLookDriverComponent`，把鼠标位置转成可复用的 yaw / pitch 镜头偏移；Run Tunnel 和 Battle camera 只负责把偏移应用到各自 base rotation。
- 战斗时调用 `SetExplorationInputEnabled(false)` suspend Run Tunnel，并启用 `UWacomBattleCameraLookComponent` 做轻量鼠标镜头跟随；不 UnPossess，也不移动 Pawn。
- 持有 `UWacomFirstPersonCardAnchorComponent`，用于计算 HUD 渲染卡牌的第一人称投影锚点；它读取 Run Tunnel / Battle camera 的 base transform，并默认使用 `BodyLocked` 投影模式，让整副手牌中心绑定在稳定的角色 / tunnel / battle 基准上，不让 cursor look 重新计算 anchor；最终中心点仍通过当前真实相机投影，保留第一人称空间感。`CardLayoutMode` 默认 `Authored2D`：只投影手牌中心，每张卡牌用美术可控 2D solver 计算位置、下坠、旋转和层级；`LegacyProjectedFan2D` 保留旧的每卡 3D 槽位投影用于对照。`LegacyWorldProjected` 保留旧的 LookInfluence 影响 anchor + 当前相机投影路径，只用于调试对照或后续视差实验。组件通过 `FirstPersonCardViewClass` 指定第一人称卡面 Widget，正式验证推荐使用 `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`；空值只作为 `UWacomCardView` 测试 fallback。组件提供 HUD debug projection、静态卡牌层、由 `BattleHUD` 写入的 runtime battle hand entry 数据源，以及 hover/click 交互入口；entry 保留卡牌实例身份和轻量视觉状态，点击意图仍回到 `BattleHUD`。`bEnableAnchorScreenSmoothing` 只平滑 `Authored2D` 的整副手牌中心；`bEnableCardSlotMotion` 则平滑单张 slot 的位置、角度、缩放和透明度，并按 `CardInstanceId` 或静态 index 复用 widget，让重排、新卡进入和移除淡出更柔和。设计见 `Docs/First_Person_Card_Layer_Design.md`。

战斗时不 UnPossess 的原因是保持 PlayerController 的 InputComponent 活跃，让 `IMC_Battle` 按键继续通过 Controller 路由到战斗 UI。

---

## §4 世界交互

探索期 `E` 交互使用通用接口 `IWacomWorldInteractable`。

| 类型 | 职责 |
|---|---|
| `IWacomWorldInteractable` | 提供提示文本、交互位置、可用性和执行交互 |
| `AWacomPlayerController` | 维护候选对象，按距离选择最近且可交互对象，按 E 调 `TryInteract` |
| `UWacomExplorationHUD` | 只显示交互提示文本，不解析交互类型 |

交互对象进入范围时注册到 PlayerController，离开范围或销毁时反注册。多个候选重叠时，PlayerController 使用 `GetInteractLocation()` 选最近对象；`CanInteract=false` 的对象不会显示提示，也不会响应 E。

### BattleTriggerActor

`ABattleTriggerActor` 是场景敌人战斗入口：

- 实现 `IWacomWorldInteractable`，默认提示“按 E 战斗”。
- SphereCollision 只做距离判定，不自动触发战斗。
- 持有 `UEnemyDefinition*` 和关卡级唯一 `PersistentId`。
- BeginPlay 时若 `PersistentId` 已在 RunState 的 destroyed trigger 记录中，则自毁。
- 玩家按 E 后经 PlayerController 请求 GameMode 进入战斗。
- V0-BH 后，Actor 默认自带远距离鼠标点击目标：`ClickBounds` 只阻挡 `Visibility` trace、不产生 overlap；`UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 自动把目标标记为 `Interaction.Target.Run.Object`，并把 `PersistentId` 同步为 stable id。初始化和 stable id 绑定由 shared click target helper 统一配置。左键释放命中时仍调用 `IWacomWorldInteractable::TryInteract()`，最终复用 `RequestEnterBattle()`，所以进入战斗流程与 E 键一致。
- V0-BH 后，鼠标 hover 到 BattleTrigger 的 `ClickBounds` 时，ExplorationHUD 交互提示位显示 `HoverPromptText`，默认 `点击战斗`；如果当前 Run 已记录该 `PersistentId` destroyed 但 Actor 仍能被 debug/probe 到，则显示 `DestroyedHoverPromptText`，默认 `战斗已结束`。打开 `GameMenu` 或菜单卡牌 drag/drop 正在处理时，不显示 hover 且点击不穿透。
- V0-BH 后，Trigger 提供 `GetBattleTriggerDebugSummary()` / `LogBattleTriggerDebugSummary()`，可在 PIE 查看 `PersistentId / EnemyDef / CanInteract / Destroyed / ClickTarget / StableId / HoverPrompt / Last`；其中 click target facts 来自通用 clickable debug view，排查能 E 但点不到或提示不对的问题。
- V0-BT 后，BattleTrigger 摆放实例接入 Validate Map/Level：缺 `PersistentId` 或 `EnemyDef` 是 error，同 World 内重复 `PersistentId` 是 warning；`BP_BattleTriggerActor` 默认资产 / CDO 允许保持空配置。

使用 use-key 而不是 overlap 自动触发，是为了支持撤离后玩家仍在 Sphere 内时再次按 E 重入战斗。

### ShopTriggerActor

`AWacomShopTriggerActor` 是场景商店入口：

- 实现 `IWacomWorldInteractable`。
- `PersistentId` 是 Run 商店库存和已购买状态的 key。
- `ShopDefinition` 提供静态商品列表；未配置时兼容旧 `Offers` 数组。
- 按 E 后调用 `AWacomPlayerController::RequestOpenShop(PersistentId, BuildResolvedOffers())`；`BuildResolvedOffers()` 优先使用 `ShopDefinition`，旧 `Offers` 数组只是兼容兜底。
- 不切换 GameFlowState；商店只是 `GameMenu` 层界面。
- V0-BG 后，Actor 默认自带远距离鼠标点击目标：`ClickBounds` 只阻挡 `Visibility` trace、不产生 overlap；`UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 自动标记为 `Interaction.Target.Run.Object`，并把 `PersistentId` 同步为 stable id。初始化和 stable id 绑定由 shared click target helper 统一配置。左键释放命中时仍调用 `IWacomWorldInteractable::TryInteract()`，最终复用 `RequestOpenShop()`，所以库存、购买和 Router 行为与 E 键一致。
- V0-BF 后，鼠标 hover 到 Shop 的 `ClickBounds` 时，ExplorationHUD 交互提示位显示 `HoverPromptText`，默认 `点击交易`；打开 Backpack / Pause / Shop / RunEvent 等 `GameMenu` 或菜单卡牌 drag/drop 正在处理时，不显示 hover 且点击不穿透。
- V0-BF 后，Trigger 提供 `GetShopTriggerDebugSummary()` / `LogShopTriggerDebugSummary()`，可在 PIE 查看 `PersistentId / ShopDefinition / Offers / CanInteract / ClickTarget / StableId / HoverPrompt / Last`；V0-BG 后其中 click target facts 来自通用 clickable debug view，排查能 E 但点不到或提示不对的问题。
- V0-BT 后，ShopTrigger 摆放实例接入 Validate Map/Level：缺 `PersistentId`、解析后没有可用商品、商品缺 `CardDefinition` 或负价格是 error；重复 `PersistentId` 是 warning。`ShopDefinition` 不是硬性必填，未配置时有效的手工 `Offers` 仍通过；`BP_WacomShopTriggerActor` 默认资产 / CDO 不会因空配置 invalid。

`UShopDefinition.ShopId` 是内容 ID，不替代场景 Actor 的 `PersistentId`。

### RunEventTriggerActor

`AWacomRunEventTriggerActor` 是场景探索事件入口：

- 实现 `IWacomWorldInteractable`。
- `PersistentId` 是 RunEvent 当前节点和完成状态 key。
- `EventDefinition` 引用 `UWacomRunEventDefinition` 事件图资产。
- 按 E 后调用 `AWacomPlayerController::RequestOpenRunEvent(PersistentId, EventDefinition)`。
- V0-BG 后，Actor 默认自带远距离鼠标点击目标：`ClickBounds` 只阻挡 `Visibility` trace、不产生 overlap；`UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 自动把目标标记为 `Interaction.Target.Run.Object`，并把 `PersistentId` 同步为 stable id。初始化和 stable id 绑定由 shared click target helper 统一配置。左键释放命中该目标时，PlayerController 仍调用现有 `IWacomWorldInteractable::TryInteract()`，最终复用 `RequestOpenRunEvent()`，所以缺配置、已完成、Router async push、Toast 等行为与 E 键一致。
- V0-BE 后，鼠标 hover 到 RunEvent 的 `ClickBounds` 时，PlayerController 复用 Run world probe loop，在 ExplorationHUD 交互提示位显示 `HoverPromptText`；已完成事件显示 `CompletedHoverPromptText`。鼠标 hover 提示优先于范围内 E 键候选提示，离开 hover 后恢复 E 键提示。打开 Backpack / Pause / Shop / RunEvent 等 `GameMenu`，或菜单 first-person card drag/drop 正在处理时，hover prompt 会被清理，不穿透提示场景事件。
- 鼠标点击不要求玩家在 `TriggerSphere` 范围内；`TriggerSphere` 仍只服务 E 键近距离候选注册。
- 已完成事件不会重复打开，但可以显示弱提示，并在按 E 时通过 AppToast 提示已完成。
- V0-BC 后，Details 面板提供 `ConfigureDebugSnakeGiftSample()` 和 `ConfigureDebugFlagRewardSample()`，用于把当前 Trigger 配成标准 Debug RunEvent 样例。按钮只写当前 Actor 的 `PersistentId / EventDefinition / prompt`，不打开事件、不修改 RunState、不生成资产。
- V0-BC 后，Trigger 提供 `GetRunEventTriggerDebugSummary()` / `LogRunEventTriggerDebugSummary()`，可在 PIE 中查看 `PersistentId / EventId / StartNode / CurrentNode / Active / Completed / Duplicate / Last`；V0-BG 后其中 click target facts 来自通用 clickable debug view，排查样例是否绑定正确、是否正在访问或已完成。
- V0-BT 后，RunEventTrigger 摆放实例接入 Validate Map/Level：缺 `PersistentId` 或 `EventDefinition` 是 error，同 World 内重复 `PersistentId` 是 warning；`BP_WacomRunEventTriggerActor` 默认资产 / CDO 允许保持空配置。

关卡放置 Debug RunEvent 的步骤见 `WacomData.md` 中的 Debug 事件资产说明和 `WacomUI.md` 的 RunEvent UI 章节。

### RunPickupActor

V0-BN 后，Run world Pickup 的世界交互壳统一在 `AWacomRunPickupActorBase`：

- Base 实现 `IWacomWorldInteractable + UWacomRunWorldClickableInteractable`，并统一管理 `PersistentId / TriggerRadius / prompt / bDestroyWhenCollected`、`TriggerSphere / ClickBounds / PickupVisual / ClickInteractionTarget / ClickTargetBridge`、E 键候选注册、鼠标 click/hover 目标绑定、scale probe debug、已拾取生命周期和跨金币 / 卡牌 Pickup 的重复 `PersistentId` 诊断。
- 具体可放置类名不变：`AWacomRunPickupActor` 仍表示金币 Pickup，`AWacomRunCardPickupActor` 仍表示固定单卡 Pickup。V0-BP 后新增 `AWacomRunRewardPickupActor` 作为数据驱动通用入口，正式内容可逐步迁移到 `PickupDefinition`。子类只负责奖励配置、RunSession 结算和成功 toast。
- V0-BO 后，金币 / 卡牌的样例配置按钮都调用 Base authoring helper，统一刷新 `TriggerRadius`、`TriggerSphere`、`ClickBounds` collision、stable id、默认 prompt 和 collected lifecycle，避免按钮只改字段但组件状态滞后。
- Base 提供 `GetRunPickupBaseDebugSummary()` / `LogRunPickupBaseDebugSummary()`，用于排查共享交互壳；V0-BO 后摘要稳定包含 `TriggerRadius / BoundsExtent / Visual / ClickTarget / ClickStableId / Duplicate / Collected / ConfigReason` 等制作字段。子类的专用 debug summary 仍保留奖励字段。
- 推荐关卡摆放入口是 `Content/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor + UWacomRunPickupDefinition`。`Content/Wacom/Maps/SceneActor/BP_WacomRunPickupActor` 和 `Content/Wacom/Maps/SceneActor/BP_WacomRunCardPickupActor` 继续保留为专用 fallback / 快速 debug 入口。Blueprint 只放默认外观和可见 primitive，不写 EventGraph 逻辑、不直接调用 RunSession；C++ 父类仍是逻辑 truth 和无 BP 时的 fallback。
- `UWacomRunPickupDefinition.PickupId` 是静态内容 / debug ID，不参与已拾取状态；所有 Pickup Actor 的防重复 key 仍是场景 `PersistentId`。
- V0-BS 后，`AWacomRunPickupActorBase::IsDataValid()` 会校验关卡摆放实例：缺 `PersistentId` 或奖励配置无效是 error，同 World 内重复 `PersistentId` 是 warning。BP 默认资产 / CDO 允许保持空配置，避免推荐摆放 BP 被误判为 invalid；正式提交地图前建议跑 Validate Map/Level。

`AWacomRunPickupActor` 是 Run world 金币拾取物 V1：

- `PersistentId` 是当前 Run 内防重复拾取的 key；`GoldAmount` 是拾取成功获得的金币数，必须大于 0。
- 默认拥有 `TriggerSphere / ClickBounds / PickupVisual / ClickInteractionTarget / ClickTargetBridge`。`TriggerSphere` 只服务 E 键近距离候选注册；`ClickBounds` 只阻挡 `Visibility` trace、不产生 overlap；`PickupVisual` 是 C++ 占位可见球体，正式美术可在 Blueprint 或子类替换。
- 按 E 或远距离左键命中时都调用 `URunSession::CollectGoldPickup(PersistentId, GoldAmount)`，由 Run 层同一事务增加金币并标记已拾取；重复提交不会再次加金币。
- 成功拾取后默认 Destroy Actor；如果 `bDestroyWhenCollected=false`，则隐藏并禁用碰撞，便于调试。成功时复用 AppToast 显示金币变化。
- 鼠标 hover 默认显示 `点击拾取`，已拾取但仍可被 debug/probe 到时显示 `CollectedHoverPromptText`，默认 `已拾取`。GameMenu active 或菜单卡牌 drag/drop 时，hover/click 不穿透。
- V0-BL 后，Details 面板提供 `ConfigureDebugGoldPickupSample()`，用于把当前 Actor 配成标准金币拾取调试样例。按钮只写当前 Actor 的 `PersistentId / GoldAmount / TriggerRadius / prompt / lifecycle`，不修改 RunState、不生成资产；V0-BO 后会同时刷新 bounds、stable id 和组件 collision。
- 提供 `GetRunPickupDebugSummary()` / `LogRunPickupDebugSummary()`，可在 PIE 查看 `PersistentId / Gold / HasRun / CanInteract / Collected / TriggerRadius / BoundsExtent / Visual / ConfigValid / ConfigReason / Duplicate / HasVisual / ClickTarget / StableId / HoverPrompt / Last`；其中 click target facts 来自通用 clickable debug view。缺 `PersistentId`、非正 `GoldAmount`、同关卡重复 `PersistentId` 都会在 summary 或 BeginPlay warning 中暴露；重复 ID 只是制作 warning，表示这些 Pickup 会共享同一份已拾取状态。

### RunCardPickupActor

`AWacomRunCardPickupActor` 是 Run world 固定卡牌拾取物 V1：

- PlayerController 不为它增加专用分支，仍走 Run world clickable resolver。
- `PersistentId` 是当前 Run 内防重复拾取的 key；`CardDefinition` 是拾取成功获得的一张固定卡牌。V1 不支持掉落表、多卡、目标区域选择或拾取动画。
- 默认拥有 `TriggerSphere / ClickBounds / PickupVisual / ClickInteractionTarget / ClickTargetBridge`，组件语义与金币 Pickup 对齐：E 键走 overlap 候选，远距离左键走 `Visibility` trace，hover 使用共享 scale probe。
- 按 E 或远距离左键命中时都调用 `URunSession::CollectCardPickup(PersistentId, CardDefinition)`；Run 层复用获得卡牌入 Run 的语义，并与金币 Pickup 共用 `CollectedPickupIds` 防重复。
- 成功后调用 `UWacomAppToastSubsystem::ShowCardGained(CardDefinition)`，然后按 `bDestroyWhenCollected` 决定 Destroy，或隐藏并禁用碰撞用于调试。
- 默认文案是 `按 E 拾取卡牌`、`点击拾取卡牌`、`已拾取`。Details 面板提供 `ConfigureDebugCardPickupSample()`，按 Actor 名生成 `Pickup.Debug.Card.{ActorName}`，尝试加载 `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang`，并恢复默认 prompt / lifecycle；资产缺失时保留当前 `CardDefinition` 并 warning。V0-BO 后按钮会同时刷新 bounds、stable id 和组件 collision。
- 提供 `GetRunCardPickupDebugSummary()` / `LogRunCardPickupDebugSummary()`，可在 PIE 查看 `PersistentId / Card / CardId / HasRun / CanInteract / Collected / TriggerRadius / BoundsExtent / Visual / ConfigValid / ConfigReason / Duplicate / HasVisual / ClickTarget / ClickStableId / HoverPrompt / Last`。缺 `PersistentId`、缺 `CardDefinition`、同关卡重复 `PersistentId` 都会在 summary 或 BeginPlay warning 中暴露；重复 ID 只是制作 warning，表示这些 Pickup 会共享同一份已拾取状态。

### RunRewardPickupActor

`AWacomRunRewardPickupActor` 是 Run world 数据驱动拾取物 V1：

- 继承 `AWacomRunPickupActorBase`，所以 E 键、远距离左键、hover prompt、scale probe、click stable id、重复 `PersistentId` 诊断和已拾取 lifecycle 都复用同一套 Pickup 壳。
- `PickupDefinition` 引用 `UWacomRunPickupDefinition`。Definition 只描述固定单一奖励：`RewardType=Gold` 时使用 `GoldAmount`，`RewardType=Card` 时使用 `CardDefinition`；`None`、缺 Definition 或配置错误都会让 `CanInteract()` 拒绝。
- 按 E 或远距离左键命中时，本 Actor 只根据 Definition 类型分发到现有 `URunSession::CollectGoldPickup(PersistentId, GoldAmount)` 或 `CollectCardPickup(PersistentId, CardDefinition)`。它不新增 RunSession 入口，也不改变 `CollectedPickupIds` 共享防重复语义。
- 成功后复用现有金币变化 Toast 或卡牌获得 Toast，再按 Base 的 `bDestroyWhenCollected` 决定 Destroy 或隐藏禁用。
- 默认 prompt 会随奖励类型兜底：Card 使用 `按 E 拾取卡牌 / 点击拾取卡牌`，Gold 或未知配置使用 `按 E 拾取 / 点击拾取`，已拾取统一显示 `已拾取`。如果在 Details 里手动填了 prompt，则仍以 Actor 字段为准。
- 提供 `GetRunRewardPickupDebugSummary()` / `LogRunRewardPickupDebugSummary()`，可在 PIE 查看 `PersistentId / Definition / PickupId / RewardType / Gold / CardId / HasRun / CanInteract / Collected / ConfigReason / Duplicate / Visual / ClickTarget / ClickStableId / HoverPrompt / Last`。它会同时带上 Base authoring facts，便于排查 Definition 错误、场景 ID 重复和能 E 但点不到的问题。
- V0-BQ 后 Details 面板提供 `ConfigureDebugGoldDefinitionPickupSample()` 和 `ConfigureDebugPoisonFangDefinitionPickupSample()`。按钮分别加载 `/Game/Wacom/Data/Pickups/DA_Pickup_DebugGold3` 或 `/Game/Wacom/Data/Pickups/DA_Pickup_DebugPoisonFang`，按 Actor 名生成 `Pickup.Debug.Definition.{ActorName}` 或 `Pickup.Debug.Definition.Card.{ActorName}`，并恢复默认 prompt、`TriggerRadius=160`、`bDestroyWhenCollected=true` 与 click stable id。按钮只改当前 Actor 配置，不修改 RunState。
- V0-BR 后新增 `Content/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor` 作为正式推荐摆放入口。该 BP 父类是 `AWacomRunRewardPickupActor`，默认不烘焙 `PersistentId` 或 `PickupDefinition`，避免多个实例共享错误 ID 或误用 Debug Definition；每个关卡实例应手动配置唯一 `PersistentId` 和一个 `UWacomRunPickupDefinition`。BP 仍不写 EventGraph 业务逻辑，Details 样例按钮可在 BP 实例上继续使用。
- V0-BS 后，摆放实例的 Validate Map/Level 会把缺 `PickupDefinition`、Definition 内部缺 `PickupId / RewardType`、金币非正或卡牌为空报告为 error；复制后重复 `PersistentId` 只报告 warning，因为共享已拾取状态可能是有意制作。

### RunKeyChestActor

V0-BV 后，旧 `AWacomDebugChestActor` 问题原型已从 Source 移除，不保留 deprecated stub，也不提供 CoreRedirect；本地残留的 `BP_WacomDebugChestActor` 视为废弃资产，不再打开或摆放。新的验证入口统一使用 `AWacomRunKeyChestActor`。

`AWacomRunKeyChestActor` 是 Run world card drop 原型宝箱 V2：

- 默认拥有 `TriggerSphere / ClickBounds / ChestVisual / ClickInteractionTarget / ClickTargetBridge / CardDropReceiver`。`ClickTargetBridge` 仍把目标标记为 `Interaction.Target.Run.Object`，所以拖卡 probe、hover preview 和普通 Run world click resolver 共用同一命中合同。
- 它实现 `IWacomWorldInteractable + UWacomRunWorldClickableInteractable`，但普通 E 键或左键不会开箱，只显示 `需要钥匙` 或 `宝箱已打开` toast；真正结算只能来自第一人称卡牌拖拽 release 到宝箱目标。
- V0-CG 后推荐填写 `CardInteractionDefinition`（`UWacomRunWorldCardInteractionDefinition`）作为制作配置源。Definition 会优先驱动内部 `UWacomRunWorldCardDropReceiverComponent` 的允许卡牌 Definition/CardId、Required/Blocked keywords、是否消耗卡、reward payload 和 receiver preview/success/completed/rejected/config/source/generic 文案；`InteractionDefinition.InteractionId` 只用于内容识别、debug 和 validation，不替代场景 `PersistentId`。`WacomRegenerateContent` 会生成推荐调试资产 `/Game/Wacom/Data/Interactions/DA_RunWorldCardInteraction_DebugKeyGold3`。
- KeyChest 配置优先级固定为 `CardInteractionDefinition` > 手填 receiver fallback。旧 KeyChest 专用 Definition 链路已删除，不再是 KeyChest 摆放或内容入口。未填写通用 Definition 时，`UWacomRunWorldCardDropReceiverComponent` 仍负责把当前拖拽卡实例、宝箱 `PersistentId`、允许卡牌 Definition/CardId、Required/Blocked keywords、是否消耗卡和 `Rewards` 组装成 `FRunWorldCardInteractionRequest`，再调用 `URunSession::ValidateRunWorldCardInteraction()` / `SubmitRunWorldCardInteraction()`。
- V0-BW/V0-BZ 后，宝箱的内部 `TriggerSphere / ClickBounds / ChestVisual / ClickInteractionTarget / ClickTargetBridge / CardDropReceiver` 都是实现细节，不作为可展开 Details 编辑入口使用，并继续隐藏容易触发 UE 5.7 Details 栈溢出的 Collision / BodyInstance 深层分类。关卡制作只改 Actor facade 字段：`TriggerRadius`、`ClickBoundsExtent`、`VisualMesh`、`VisualScale`、`VisualRelativeLocation`、`CompletedVisualMesh`、`CompletedVisualScale`、`CompletedVisualRelativeLocation`、文案、`PersistentId` 和 receiver 配置。完成态外观只同步内部 `ChestVisual`，不改变 `ClickBounds / TriggerSphere` 命中范围。
- V0-BZ 后，KeyChest 会在 BeginPlay 尝试绑定当前 `URunSession::OnRunStateChangedNative`，RunState 改变后按 `CompletedRunWorldInteractionIds` 刷新关闭 / 已打开外观；如果 BeginPlay 时还拿不到 RunSession，hover、E 键、普通点击和 debug summary 入口会补绑定并刷新。宝箱已打开后仍保留 hover、左键和 E 键命中，只显示 `CompletedPromptText`，拖卡重复释放仍由 RunSession 拒绝，不再奖励金币或消耗卡。
- V0-CB 后，Run world card drop 的 release 失败文案由目标上的 `UWacomRunWorldCardDropReceiverComponent` 提供：`AlreadyCompleted` 使用 receiver completed prompt，错卡 / 缺关键词 / 被黑名单阻挡 / 缺卡定义使用 receiver rejected prompt，配置异常使用 receiver config warning prompt + reason，源卡不可用使用 receiver source-card-unavailable prompt，其他失败使用 receiver generic failure prompt + reason。PlayerController 只负责命中、提交、展示 Toast，以及 `MissingCardDropReceiver / InvalidSubmitContext` 等没有 receiver 可询问时的通用配置异常 fallback。拖拽 preview 阶段仍只更新卡牌和目标的轻量有效 / 无效反馈，不弹 Toast；松到空处也不弹失败 Toast。V0-CG 后成功路径按 reward payload 展示反馈：Gold 调 `ShowGoldChanged()`，Card 调 `ShowCardGained()`，不额外显示成功文案。
- Details 按钮 `ConfigureDebugKeyChestSample()` 会把当前 Actor 配成标准样例，并清空 `CardInteractionDefinition` 以保持无 Definition 的快速 PIE fallback：`PersistentId=Chest.Debug.{ActorName}`，`TriggerRadius=180`，`ClickBoundsExtent=(85,65,55)`，默认 cube visual，完成态默认复用关闭态 mesh 但使用更扁的 scale 和轻微下移，接受 `/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey` 或 `CardId=DebugKey`，成功奖励 3 金币并消耗钥匙。按钮只改当前 Actor 配置、刷新 bounds / visual / receiver / click stable id，不修改 RunState、不生成资产。
- Details 按钮 `ConfigureDebugKeyChestInteractionDefinitionSample()` 会把当前 Actor 配成同一套 safe facade 样例，并绑定 `/Game/Wacom/Data/Interactions/DA_RunWorldCardInteraction_DebugKeyGold3` 到 `CardInteractionDefinition`、刷新 receiver 和 click stable id。正式 PIE 验证优先用这个按钮或手动填写同一通用 Definition。
- V0-CG 后，KeyChest 摆放实例接入 Validate Map/Level：缺 `PersistentId` 是 error；有 `CardInteractionDefinition` 时校验 Definition 内部 `InteractionId / 正向筛选 / Rewards`；无 Definition 时继续校验 receiver fallback。缺 `CardDropReceiver`、缺 reward、Gold reward 非正、Card reward 缺定义、无正向卡牌筛选都会成为 error；同 World 内重复 `PersistentId` 是 warning。有效筛选要求 `AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 至少一个非空；`BlockedKeywords` 只能作为附加限制。
- `GetRunKeyChestDebugSummary()` / `LogRunKeyChestDebugSummary()` 会输出 `Definition / InteractionId / DefinitionReason / DefinitionSource / ConfigValid / ConfigReason / Duplicate / TriggerRadius / ClickBoundsExtent / VisualName / VisualMesh / VisualScale / CompletedVisualMesh / CompletedVisualScale / CompletedVisualLocation / VisualState / ClickTarget / ClickStableId / ReceiverAllowedDefs / ReceiverAllowedIds / RequiredKeywords / BlockedKeywords / PositiveFilter / RewardCount / GoldTotal / CardRewardCount / Consume / Completed / Last / ReceiverDebug`，用于 PIE 排查通用 Definition、钥匙筛选、奖励 payload、safe facade 是否同步、stable id、重复 ID、完成状态和当前关闭 / 已打开视觉状态。Receiver debug summary 还会输出 `Definition / InteractionId / DefinitionReason / ConfigSource`、reward 统计和 rejected/config/source-unavailable/generic failure prompt facts。PlayerController 的 `LastRunWorldCardDropDebugSummary` 会记录 release 时的 `Phase / Reason / Validation / Submitted / ToastSource / FailureToast`，用于排查为什么 release 没有结算、失败文案来自 receiver 还是 Controller fallback，以及最终展示了哪条失败 Toast。

---

## §5 UI 总入口

`UWacomGameInstance` 使用 `UWacomGameUIManagerSubsystem` 管理 UI 根布局。

`UWacomGameUIManagerSubsystem` 负责：

- 创建或重建 `UWacomPrimaryGameLayout`。
- 按 Layer Push / Pop Activatable Widget。
- 切关卡时 TearDown 旧 Layout，跟随当前 PlayerController 重建。

PrimaryLayout 的层级用途、输入路由和 HUD active 行为由 `WacomUI.md` 维护；本文只记录 App 侧创建、重建和 Push / Pop 入口。

顶层 UI 类解析由 App 侧入口执行，表现合同见 `WacomUI.md`：

- 顶层 Screen / Toast 不再通过 `AWacomPlayerController` 暴露 ScreenClass 覆盖入口。
- Backpack / PauseMenu / Shop / RunEvent 通过 `UWacomUIDeveloperSettings.WidgetClasses` 的 `UI.Widget.*` tag 注册。
- AppToast 通过 `UWacomUIDeveloperSettings.AppToastWidgetClass` 注册。
- Settings 未命中、软类为空或加载失败时，Backpack / PauseMenu / Shop / RunEvent / Toast 回到对应 C++ fallback。
- PrimaryLayout 优先使用 `UWacomUIDeveloperSettings.PrimaryLayoutClass`；未配置或加载失败时只尝试固定 `WBP_PrimaryGameLayout` 路径 fallback，仍失败则拒绝创建根布局。
- Backpack / PauseMenu / Shop / RunEvent 都已接入 `PushRegisteredWidgetToLayerAsync()`：settings 软类未加载时异步加载，GameMenu pending 期间重复打开请求会被忽略。
- Shop / RunEvent 的 `RequestOpen*` 返回 true 只表示打开请求已接收；真正的 `BeginShopVisit()` / `BeginRunEvent()` 发生在 async push 的 `BeforePush` 阶段。

编辑器配置与验证清单：

- 在 `Edit > Project Settings > Wacom UI Settings` 填写需要覆盖的 UI 类；本轮不强制配置所有顶层 WBP。
- 保存配置后可通过编辑器 Data Validation 检查 Wacom UI Settings，确认 PrimaryLayout、AppToast 和 `WidgetClasses` 的继承关系、tag 命名、重复 tag、空 class 等错误。
- PIE 时按 Wacom UI Settings -> fallback 的优先级解析；Shop / RunEvent / PauseMenu 等未配置 settings 项时，应继续走合法 C++ fallback。

探索期背包、暂停菜单、商店、RunEvent 都是 `GameMenu` 层界面。公开请求入口仍在 `AWacomPlayerController`，内部由私有 `FWacomExplorationScreenRouter` 统一处理探索状态检查、PrimaryLayout 确保、关闭已有 GameMenu 顶层、GameMenu 异步 Push pending、防重复打开，以及商店 / RunEvent 这类外部流程返回时的 RunSession 清理。

V0-AL 后，`GameMenu` 激活时默认压制探索期 first-person BattleDeck 展示，避免直接加到 viewport 的卡层遮挡菜单；失活后恢复。Router 在异步切换菜单期间会加一层短暂 transition suppress，避免旧菜单关闭、新菜单尚未 Push 完成时卡牌闪出。V0-AN 后，RunEvent “交出毒牙”这类菜单卡牌交互不应绕开该规则，也不应在蓝图中手填 `FWacomFirstPersonCardLayerEntry`，而应调用 `UWacomMenuWidgetBase::SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards()`。菜单只提交候选筛选 request，例如 `AllowedCardDefinitions = [DA_Card_PoisonFang]`；PlayerController / source component 从 `URunSession::GetRunState()` 的真实持有区构建 lease entries，并在菜单 deactivate 时自动清理 owned lease。V0-AQ 后菜单覆写 `ResolveRunMenuFirstPersonCardDropIntent()` / `SubmitRunMenuFirstPersonCardDropIntent()` 声明某个 Zone/card 是 probe、Controller 默认销毁，还是菜单自处理提交。PlayerController 统一 resolver 负责 preview、validation 和 release 分发；RunEventScreen 使用 `MenuHandled` policy，release 成功时由 Screen 调 `ChooseRunEventOptionWithPaidCardResult()`，把精确移卡、Effects 和节点推进留在 RunSession 事务内，并把失败写回 drop result。

商店和 RunEvent 切换必须遵守生命周期顺序：Router 先关闭已有 `GameMenu` 顶层，让旧 Screen 的 `NativeOnDeactivated` 完成 `EndShopVisit()` / `EndRunEvent()`；随后创建 async push 请求。新访问的 `BeginShopVisit()` / `BeginRunEvent()` 必须延后到 `BeforePush`，如果 Push、Cast 或 Refresh 失败，Router 必须立即调用对应 `End*` 回滚刚 Begin 的 active 访问。

UI 行为细节见 `WacomUI.md`：

- Run MVVM 和 ExplorationHUD。
- AppToast、交互 Toast、Battle Combat Log。
- BackpackScreen、ShopScreen、RunEventScreen。
- BattleHUD、战斗日志、手牌 hover 详情和目标选择 ViewData。
- Card / Shop / BattleEvent PresentationBuilder。

WBP 制作合约见：

- `UI_Backpack_WBP_Binding.md`
- `UI_Battle_WBP_Binding.md`
- `UI_RunEvent_WBP_Binding.md`

---

## §6 输入协调

`UWacomInputContextCoordinatorSubsystem` 是本地玩家输入上下文的唯一协调者。GameMode、PlayerController 和运行时组件只声明当前意图，Subsystem 统一应用 CommonUI input config、鼠标显隐 / capture、Enhanced Input MappingContext，以及 PlayerController click / mouse-over event lease。

当前上下文：

| Context | CommonUI input config | IMC |
|---|---|---|
| `MainMenu` | `Menu + NoCapture`，鼠标可见 | 不启用探索 / 战斗 IMC |
| `Exploration` | `All + NoCapture`，鼠标可见 | `IMC_Exploration` |
| `Battle` | `All + NoCapture`，鼠标可见 | `IMC_Battle` |

PlayerController 上的 `PushMappingContext / PopMappingContext` helper 仍保留为兼容 / 调试入口；正式流程由 Coordinator 管理。PlayerController BeginPlay 和 GameMode BeginPlay 会初始化 Coordinator，防止 PIE 复用 Controller 时输入状态停留在上一关。

Run Tunnel 是探索期默认移动模型，不再有正式的普通 FPS FreeLook 探索 profile。进入战斗时 `UWacomRunTunnelMovementComponent` 只 `Suspend`，保留当前 Segment / Distance；`UWacomBattleCameraLookComponent` 接管 ControlRotation 并在战斗 base rotation 上叠加共享 cursor look offset。战斗结束后先停用 Battle camera look，再让 Coordinator 回到 `Exploration`，Run Tunnel `Resume` 后继续沿原 tunnel path 移动。

BattleHUD、3D 手牌和场景目标点击需要的 `bEnableClickEvents / bEnableMouseOverEvents` 也通过 Coordinator 的 owner lease 管理。多个系统同时申请时不会互相提前恢复；最后一个 owner 释放后恢复 PlayerController 原始状态。

| IMC | 内容 |
|---|---|
| `IMC_Exploration` | WASD 移动、鼠标视角、交互、ESC 菜单、B 背包 |
| `IMC_Battle` | 1-7 打牌、等待、结束回合、重启、刷新 HUD；不绑定背包 |

当前 IA 口径：

| IA | 用途 |
|---|---|
| `IA_Move` | WASD 移动 |
| `IA_Look` | 鼠标视角 |
| `IA_Interact` | 探索期按 E 与最近世界交互对象互动 |
| `IA_OpenMenu` | ESC 打开或关闭菜单 |
| `IA_OpenBackpack` | 探索期打开背包 |
| `IA_PlayCard1~7` | 战斗手牌快捷键 |
| `IA_Wait` | 战斗等待 |
| `IA_EndTurn` | 战斗结束回合 |

切关卡时 PlayerController 的 InputComponent 会被重建。GameMode 在 BeginPlay 后根据当前 GameFlowState 让 Coordinator 重新应用对应 profile。

`AWacomPlayerController::RefreshInteractToast()` 只在 Exploration 状态显示交互提示。战斗中即使候选对象仍在列表，也不会显示交互 Toast。

V0-BK 后，探索期左键释放的场景点击路由顺序是：Battle target click -> RunTunnel branch click -> Run world interactable click -> `Super::InputKey()`。Run world interactable click 只接受 `TargetKind=World + Interaction.Target.Run.Object`，并通过共享 resolver 要求命中 Actor 同时实现 `IWacomWorldInteractable` 与 `UWacomRunWorldClickableInteractable`；当前正式 opt-in 对象是 `AWacomRunEventTriggerActor`、`AWacomShopTriggerActor`、`ABattleTriggerActor`、`AWacomRunPickupActor`、`AWacomRunCardPickupActor`、`AWacomRunRewardPickupActor`，以及 V0-BV 原型 `AWacomRunKeyChestActor`。打开 Backpack / Pause / Shop / RunEvent 等 `GameMenu` 时不会穿透点击场景目标。E 键入口不变，仍使用最近 overlap 候选。同一 Run world probe loop 还维护 hover prompt：hover 到支持目标时显示点击提示，移开后回到最近 E 键候选提示；hover prompt 和 hover debug 都走 clickable 接口，`GetRunWorldInteractableHoverDebugSummary()` 可排查当前 hover actor、stable id、prompt、completed、ClickBounds / bridge / visual 配置和拒绝原因，例如 `MissingWorldInteractableContract` 或 `MissingClickableContract`。

兼容 / 调试入口仍保留：`Wacom.Interact` 调用当前最近交互对象，`Wacom.OpenBackpack` 打开背包。正式玩家交互口径仍是 IA 输入。

---

## §7 战斗进出流程

### EnterBattle

```text
玩家进入 ABattleTriggerActor 范围
-> Trigger 注册为候选 IWacomWorldInteractable
-> ExplorationHUD 显示“按 E 战斗”
-> 玩家按 E
-> PlayerController PickClosestInteractable
-> Trigger.TryInteract()
-> PlayerController.RequestEnterBattle(EnemyDef, TriggerActor)
-> GameMode.EnterBattle()
```

GameMode 进入战斗时：

1. 设置 `EGameFlowState::Battle`。
2. Suspend PlayerCharacter 的 Run Tunnel 探索移动，并启用 Battle camera look。
3. Pop `IMC_Exploration`，Push `IMC_Battle`。
4. 由 RunSession 构造 Battle init params。
5. 创建 / 初始化 BattleSession。
6. Push `UBattleHUD` 到 Game 层。
7. 记录触发战斗的 Trigger Actor，用于退出战斗时处理撤离、胜利和场景销毁。

进入战斗前 / 进入战斗过程中会清理探索期 `RunFirstPersonBattleDeck` runtime source，避免 Run 备战展示和 BattleHUD runtime hand 同时占用 first-person card layer。

### ExitBattle

```text
BattleSession 结算完毕
-> BattleHUD / Controller 通知 GameMode
-> GameMode.ExitBattle()
```

GameMode 退出战斗时：

1. 设置 `EGameFlowState::Exploration`。
2. Pop BattleHUD。
3. Pop `IMC_Battle`，Push `IMC_Exploration`。
4. 停用 Battle camera look，并恢复 PlayerCharacter 探索移动。
5. 调 RunSession 结算战斗结果。
6. 真胜利时标记并销毁触发战斗的 `ABattleTriggerActor`。
7. 撤离时不销毁 Trigger，允许玩家再次按 E 重入。
8. 非 Undetermined 战斗结束后消耗 1 节点。

退出战斗回到 Exploration 后，PlayerController 会重新激活并刷新 `UWacomRunFirstPersonCardSourceComponent`，让 first-person card layer 再次显示当前 Run BattleDeck。这个刷新只读 Run snapshot，不提交任何 Run 命令。

战斗结果包和 Run 结算规则见 `WacomRun.md`；战斗内规则见 `WacomBattle.md`。

---

## §8 菜单与测试约定

菜单：

- `MainMenuScreen`：New Game / Continue / Quit。
- `PauseMenuScreen`：Resume / Save / Quit to Menu。
- `UWacomConfirmDialog`：推入 Modal 层，用于删除卡牌、退出确认等。
- 菜单按钮不直接 OpenLevel；切关卡委托给 GameMode 或 PlayerController。主菜单和暂停菜单切关前先 `TearDownPrimaryLayout()`，再在下一帧 `OpenLevel()`，避免在按钮点击 / CommonUI deactivate 链中立即切关。
- UE travel 目标必须使用 package path，不使用带 `.AssetName` 后缀的 ObjectPath。

验证入口：

| 场景 | 用途 |
|---|---|
| `L_Exploration` | 完整探索 -> 世界交互 -> 战斗 / 商店 / 事件流程；PIE 战斗验证走正式 `AWacomGameMode` 生命周期 |
| `L_MainMenu` | 主菜单与启动流程 |

ESC 当前语义：

- 探索时 ESC 打开暂停菜单。
- 菜单中 ESC 关闭当前菜单。
- 战斗时 ESC 使用同一 `IA_OpenMenu` 入口打开或关闭暂停菜单。

---

## §9 交互目标系统

交互目标系统提供统一的"鼠标下方是什么目标"查询能力，为后续拖拽系统打地基。

### 三层结构

| 层 | 组件 | 模块 |
|---|---|---|
| 命中层（Target Provider）| `IWacomInteractionTargetProvider` + 各 Component | `WacomApp` |
| 描述层（Target Handle）| `FWacomInteractionTargetHandle` | `WacomCore` |
| 规则层（Target Resolver）| 域层 Resolver（后续接入）| `WacomBattle` / `WacomRun` |

### 命中层

- `IWacomInteractionTargetProvider`：轻量接口，Component 实现后提供 `BuildWorldTargetHandle()`。
- `UWacomInteractionTargetComponent`：通用交互目标组件，任意 Actor 可挂载。字段：`TargetId`（运行时 FGuid）、`InteractionTargetTag`（FGameplayTag）、`StableTargetId`（FName）。
- `AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`：Battle 场景敌人推荐摆放入口。Host 只做分组、debug 和制作校验；每个 PartActor 是一个独立可命中的部位，Actor facade 字段 `PartId / HitBoundsExtent / VisualMesh / VisualScale / VisualRelativeLocation` 会同步到内部命中体、可见体、`UWacomInteractionTargetComponent` 和 bridge。当前规则层仍是单敌人，Host 的 `EnemyDefinition` 只用于校验和 debug。
- `UWacomBattleEnemyPartWorldTargetBridgeComponent`：Battle 专用底层桥接组件。它读取稳定 `PartId`，在 HUD 刷新时解析当前 `PartInstanceId`，写回同 Actor 上的 `UWacomInteractionTargetComponent`，并注册接收 `TargetConfirmed / DamageDealt / EnemyPartHpEmptied` 表现 cue。V0-CN 后推荐通过 `AWacomBattleEnemyPartActor` 间接持有它，手挂组件仍可用于排障和旧原型。
- `UWacomRunWorldInteractionTargetBridgeComponent`：Run / 探索专用桥接组件。它把手工填写的 `RunTargetStableId` 和自动/已有运行时 `TargetId` 写回同 Actor 上的 `UWacomInteractionTargetComponent`，并标记 `Interaction.Target.Run.Object`。它提供鼠标 probe preview、debug，以及 RunEvent / Shop / BattleTrigger / Pickup / KeyChest 这类 Run object 目标身份；规则执行仍必须回到领域出口，普通 click/E 键是 `IWacomWorldInteractable`，拖卡到场景是 `UWacomRunWorldCardDropReceiverComponent -> URunSession`。Run world card drop release 失败反馈同样属于 receiver contract，后续门、机关、祭坛或 NPC 应复用 receiver 文案合同，而不是给 PlayerController 增加类型分支。
- `UWacomRunWorldClickableInteractable`：Run world click / hover 的显式 opt-in 接口。Actor 只有同时实现它和 `IWacomWorldInteractable`，PlayerController 才会把 `Interaction.Target.Run.Object` 命中转成点击交互或 hover prompt。接口提供 hover prompt 和通用 debug view；`FWacomRunWorldClickableInteractableHelper` 统一配置 `ClickBounds` collision、`PersistentId -> StableTargetId / RunTargetStableId` 绑定，并生成 stable id、接口实现、ClickBounds、target component、bridge、visual target 和 reject reason 等通用 debug facts。
- `UWacomRunMenuDropTargetWidget`：Run GameMenu 专用 UMG Zone target bridge。它配置 `ZoneId / StableTargetId`，构建 `FWacomInteractionTargetHandle(TargetKind=Zone)`，并提供 probe / invalid / released-probe / submit-ready / submitted 的轻量 preview。该 Widget 不直接调用 `URunSession`，也不参与背包旧 `UWacomZoneDropTarget` 的 UMG DragDrop 规则提交。
- `AWacomPlayerController::TryRouteBattleSceneTargetClick()` 中通过 cursor trace 命中 Component 后，扫描 `IWacomInteractionTargetProvider` 接口构建统一 handle；只有 `TargetKind=World` 且 `TargetTag=Interaction.Target.Battle.EnemyPart` 的 handle 会被转发为 Battle enemy part 点击。
- `AWacomPlayerController::TryProbeRunSceneInteractionTarget()` 和 `TryProbeRunSceneInteractionTargetAtWidgetPosition()` 在 Exploration 下用同一 Provider 路径构建 handle，但只接受 `TargetTag=Interaction.Target.Run.Object`。V0-BJ 后 hover、click 和 probe preview 共用 `handle -> source actor -> clickable world interactable` resolver：先校验 run object handle，再解析 `SourceObject` owner，最后要求 `IWacomWorldInteractable + UWacomRunWorldClickableInteractable + RunWorldBridge`。`bEnableRunWorldTargetProbePreview` 的视觉信号只对 resolver 通过且当前 hover gate 允许的目标生效；Controller 会低频 probe 鼠标下方 Run target，并驱动 bridge 的 scale 与 CustomDepth / stencil preview，失去命中、切换目标、打开 GameMenu、菜单卡牌 drag/drop、退出 Controller 时都会清理旧 preview。`TryRouteRunWorldInteractableClick()` 复用同一 resolver，只在 Exploration、无 active GameMenu、无菜单卡牌 drag/drop 处理时，把命中的 clickable world interactable 转回现有 `TryInteract()`。hover prompt 同样复用该 loop，通过 `UWacomRunWorldClickableInteractable` 读取文案，写入 ExplorationHUD 的交互提示。V0-BU 的 Run world card drop resolver 也复用 `TryProbeRunSceneInteractionTargetAtWidgetPosition()`，但额外要求目标 Actor 有 `UWacomRunWorldCardDropReceiverComponent`，并且会把精确拖拽卡实例提交给 RunSession 事务。V0-CB 后，只有 release 命中过 Run world target 且提交失败时才发失败 AppToast；具体文案优先来自 receiver failure contract，preview 阶段和松到空处只更新 feedback / debug summary，不刷屏。
- `AWacomPlayerController::TryProbeRunMenuDropTargetAtWidgetPosition()` 在 Exploration + active GameMenu + active menu lease 的 first-person card drag 中使用。它只扫描注册过的 `UWacomRunMenuDropTargetWidget`，按后注册优先作为最上层命中，返回 Zone handle。`ResolveRunMenuCardDropIntent()` 统一解析 preview 和 release：默认是 probe-only；owning menu 返回 `SubmitZoneTarget + ControllerDestroyOwnedCard` 且 `ValidateDestroyCardByInstance()` 通过时，release 才由 Controller 移除精确持有卡实例；`MenuHandled` 由菜单提交并回填结果。
- `UWacomFirstPersonCardLayerSlotWidget` 为当前 active、可见、非 exiting 且拥有有效 `CardInstanceId` 的 first-person slot 构建 Card target handle。它使用当前 visual slot 的 `ScreenPosition`，不要求卡牌可打；后续拖拽 resolver 再判断当前拖拽卡能否作用到该卡槽。
- First-person drag feedback 使用同一个 `FWacomInteractionTargetHandle`。Battle World 目标反馈优先落到 `AWacomBattleEnemyPartActor` 内部 bridge 的 transient preview，不经过 `EnemyInfoBar` 或 BattleEvent presentation queue；Card 目标反馈区分合法 hand-card target 和 probe-only target；Run menu Zone target 使用 `ZoneProbe` 反馈表示“当前菜单区域可被识别”，是否支付由 Run menu drop intent 决定。
- Battle first-person drag/drop 由 `BattleHUD::ResolveFirstPersonCardDropIntent()` 统一解析 preview 和 release 语义。当前提交既有 `PlayCard` 命令：无目标卡 armed 提交空目标，合法 world enemy part 提交目标部位，合法 `TargetMode=HandCard` 源卡提交 `TargetCardInstanceId`。UI 不区分加费、减费、弃置或消耗的具体规则；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 对左右手锚点的拒绝来自 BattleSession / PlayCardResolver 合法性。不支持的 Card target 仍为 probe-only，Zone / Run target 后续接入。V0-AG 后，resolver 预览使用 `UBattleSession::ValidateTargetWithCard()` 获取可解释拒绝原因；拖拽 `TargetMode=HandCard` 源卡时，HUD 会为整副 first-person hand 生成合法 / 非法 Card target affordance，玩家只看到轻量颜色和缩放，具体 reason 只进入 debug summary / 自动化测试。V0-CL 后，战斗事件 presentation queue 忙碌不再作为 Battle first-person drop 的 `UIBlocked` 条件；只要 Battle Snapshot 仍处于 `PlayerAction`、没有 pending Wait / EndTurn 屏障，且源卡 / 目标合法，玩家可在旧表现播放时继续拖卡提交。若玩家已请求 Wait 或 EndTurn，HUD 会等当前卡牌表现栈清空后执行该回合边界命令，并在 pending 期间拒绝继续出牌、目标选择和 first-person drop。

### 描述层

`FWacomInteractionTargetHandle`（`WacomCore/Public/Types/WacomInteractionTargetTypes.h`）是纯数据 struct：

| 字段 | 用途 |
|---|---|
| `TargetKind` | None / World / Card / Zone |
| `WorldTargetId` | World 目标的 FGuid |
| `CardInstanceId` | Card 目标的 FGuid；first-person hand 当前由卡槽 hover / visual update 提供 |
| `ZoneId` | Zone 目标的 FName；V0-AM 起 Run menu drop target 可提供 |
| `TargetTag` | 目标语义标签，例如 `Interaction.Target.Battle.EnemyPart` |
| `StableTargetId` | 稳定 authored/data ID，例如敌人 `PartId` |
| `SourceObject` | 命中来源 Component 弱引用 |
| `WorldLocation` / `ScreenPosition` | 命中位置 |

### 规则层

Battle 已接入 `UBattleSession::CanTargetWithCard(CardInstanceId, FWacomInteractionTargetHandle)` 和 `ValidateTargetWithCard(...)`，当前用于 TargetSelect 可选部位视图、first-person drag/drop resolver 的 world target 合法性判断、`TargetMode=HandCard` 的 Card target 合法性判断，以及拖拽预览。`CanTargetWithCard()` 只返回 bool，内部转调 validation；`ValidateTargetWithCard()` 会区分 self target、源卡不是 HandCard、目标不在手牌、selected discard/exhaust 不支持锚点、非法 world target 等原因。`Target.SelectedHandCard` 的加费 / 减费允许普通手牌和左右手锚点；指定弃置 / 消耗只允许普通手牌。Run resolver 后续接入。

### 当前范围

- [x] World 目标：通过 `UWacomInteractionTargetComponent` 命中；Battle enemy part 推荐由 `AWacomBattleEnemyPartActor` 持有 bridge 并绑定运行时 ID
- [x] Run World 目标：通过 `UWacomRunWorldInteractionTargetBridgeComponent` 标记 `Interaction.Target.Run.Object` 并提供 probe preview；规则层 resolver 后续接入
- [x] Card 目标：first-person hand slot hover / visual update 已通过 `UWacomFirstPersonCardLayerWidget` 与 `UWacomFirstPersonCardAnchorComponent` 暴露；旧 `UCardWidget / UHandPanel` 不作为本轮维护入口
- [x] Zone 目标：Run menu lease 下通过 `UWacomRunMenuDropTargetWidget` 命中并 probe；背包旧 DragDrop 和规则提交仍保持独立
- [x] Battle 规则层 Resolver：`UBattleSession::CanTargetWithCard`
- [ ] Run 规则层 Resolver（后续接入）

### 不变项

- 原有 `IWacomWorldInteractable` 探索期 E 键交互不变。
- Run target probe preview 只表达“鼠标当前命中支持 Run world clickable 合同的世界交互物”，不表达规则可提交或某张卡能作用。当前 visual signal 是轻量 scale + CustomDepth/stencil；没有项目级 outline/post-process 时，不保证出现描边。
- 原有 BattleHUD target registration / TargetCue 表现不变。
- `TryRouteBattleSceneTargetClick` 的 `protected virtual` 测试 seam 不变。
