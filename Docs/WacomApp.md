---
type: orchestration-spec
scope: wacom-app
status: active
updated: 2026-05-28
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

---

## §3 PlayerController 与 PlayerCharacter

`AWacomPlayerController` 是探索输入、RunSession 持有者和 UI 请求入口。

核心蓝图资产归档到 `/Game/Wacom/Core`：GameMode 放 `/Game/Wacom/Core/GameModes`，PlayerCharacter 放 `/Game/Wacom/Core/Player`，PlayerController 放 `/Game/Wacom/Core/Controllers`。Run Tunnel authoring 蓝图放 `/Game/Wacom/Run/Tunnel/Blueprints`。`/Game/Wacom/_GAME` 是历史临时目录，不再作为新增资产入口；具体目录规范见 `Docs/Content_Organization.md`。

主要职责：

- BeginPlay 创建并持有 `URunSession`。
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

使用 use-key 而不是 overlap 自动触发，是为了支持撤离后玩家仍在 Sphere 内时再次按 E 重入战斗。

### ShopTriggerActor

`AWacomShopTriggerActor` 是场景商店入口：

- 实现 `IWacomWorldInteractable`。
- `PersistentId` 是 Run 商店库存和已购买状态的 key。
- `ShopDefinition` 提供静态商品列表；未配置时兼容旧 `Offers` 数组。
- 按 E 后调用 `AWacomPlayerController::RequestOpenShop(PersistentId, BuildResolvedOffers())`；`BuildResolvedOffers()` 优先使用 `ShopDefinition`，旧 `Offers` 数组只是兼容兜底。
- 不切换 GameFlowState；商店只是 `GameMenu` 层界面。

`UShopDefinition.ShopId` 是内容 ID，不替代场景 Actor 的 `PersistentId`。

### RunEventTriggerActor

`AWacomRunEventTriggerActor` 是场景探索事件入口：

- 实现 `IWacomWorldInteractable`。
- `PersistentId` 是 RunEvent 当前节点和完成状态 key。
- `EventDefinition` 引用 `UWacomRunEventDefinition` 事件图资产。
- 按 E 后调用 `AWacomPlayerController::RequestOpenRunEvent(PersistentId, EventDefinition)`。
- 已完成事件不会重复打开，但可以显示弱提示，并在按 E 时通过 AppToast 提示已完成。

关卡放置 Debug RunEvent 的步骤见 `WacomData.md` 中的 Debug 事件资产说明和 `WacomUI.md` 的 RunEvent UI 章节。

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

商店和 RunEvent 切换必须遵守生命周期顺序：Router 先关闭已有 `GameMenu` 顶层，让旧 Screen 的 `NativeOnDeactivated` 完成 `EndShopVisit()` / `EndRunEvent()`；随后创建 async push 请求。新访问的 `BeginShopVisit()` / `BeginRunEvent()` 必须延后到 `BeforePush`，如果 Push、Cast 或 Refresh 失败，Router 必须立即调用对应 `End*` 回滚刚 Begin 的 active 访问。

UI 行为细节见 `WacomUI.md`：

- Run MVVM 和 ExplorationHUD。
- AppToast、交互 Toast、Battle EventToast。
- BackpackScreen、ShopScreen、RunEventScreen。
- BattleHUD、战斗日志、手牌 hover 详情和目标选择 ViewData。
- Card / Shop / BattleEvent PresentationBuilder。

WBP 制作合约见：

- `UI_Backpack_WBP_Binding.md`
- `UI_Battle_WBP_Binding.md`

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

战斗结果包和 Run 结算规则见 `WacomRun.md`；战斗内规则见 `WacomBattle.md`。

---

## §8 菜单与测试约定

菜单：

- `MainMenuScreen`：New Game / Continue / Quit。
- `PauseMenuScreen`：Resume / Save / Quit to Menu。
- `UWacomConfirmDialog`：推入 Modal 层，用于删除卡牌、退出确认等。
- 菜单按钮不直接 OpenLevel；切关卡委托给 GameMode 或 PlayerController。

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
- `UWacomBattleEnemyPartWorldTargetBridgeComponent`：Battle 专用桥接组件。它读取稳定 `PartId`，在 HUD 刷新时解析当前 `PartInstanceId`，写回同 Actor 上的 `UWacomInteractionTargetComponent`，并注册接收 `TargetConfirmed / DamageDealt / EnemyPartHpEmptied` 表现 cue。
- `UWacomRunWorldInteractionTargetBridgeComponent`：Run / 探索专用桥接组件。它把手工填写的 `RunTargetStableId` 和自动/已有运行时 `TargetId` 写回同 Actor 上的 `UWacomInteractionTargetComponent`，并标记 `Interaction.Target.Run.Object`。它只提供鼠标 probe preview 和 debug，不提交 Run 规则，也不替代 `IWacomWorldInteractable + E`。
- `AWacomPlayerController::TryRouteBattleSceneTargetClick()` 中通过 cursor trace 命中 Component 后，扫描 `IWacomInteractionTargetProvider` 接口构建统一 handle；只有 `TargetKind=World` 且 `TargetTag=Interaction.Target.Battle.EnemyPart` 的 handle 会被转发为 Battle enemy part 点击。
- `AWacomPlayerController::TryProbeRunSceneInteractionTarget()` 和 `TryProbeRunSceneInteractionTargetAtWidgetPosition()` 在 Exploration 下用同一 Provider 路径构建 handle，但只接受 `TargetTag=Interaction.Target.Run.Object`。`bEnableRunWorldTargetProbePreview` 开启时，Controller 会低频 probe 鼠标下方 Run target 并驱动 bridge 的 scale preview；失去命中、切换目标、退出 Controller 时会清理旧 preview。
- `UWacomFirstPersonCardLayerSlotWidget` 为当前 active、可见、非 exiting 且拥有有效 `CardInstanceId` 的 first-person slot 构建 Card target handle。它使用当前 visual slot 的 `ScreenPosition`，不要求卡牌可打；后续拖拽 resolver 再判断当前拖拽卡能否作用到该卡槽。
- First-person drag feedback 使用同一个 `FWacomInteractionTargetHandle`。World 目标反馈只作用于场景 bridge 的 transient preview，不经过 `EnemyInfoBar` 或 BattleEvent presentation queue；Card 目标反馈区分合法 hand-card target 和 probe-only target。
- Battle first-person drag/drop 由 `BattleHUD::ResolveFirstPersonCardDropIntent()` 统一解析 preview 和 release 语义。当前提交既有 `PlayCard` 命令：无目标卡 armed 提交空目标，合法 world enemy part 提交目标部位，合法 `TargetMode=HandCard` 源卡提交 `TargetCardInstanceId`。UI 不区分加费、减费、弃置或消耗的具体规则；`Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected` 对左右手锚点的拒绝来自 BattleSession / PlayCardResolver 合法性。不支持的 Card target 仍为 probe-only，Zone / Run target 后续接入。V0-AG 后，resolver 预览使用 `UBattleSession::ValidateTargetWithCard()` 获取可解释拒绝原因；拖拽 `TargetMode=HandCard` 源卡时，HUD 会为整副 first-person hand 生成合法 / 非法 Card target affordance，玩家只看到轻量颜色和缩放，具体 reason 只进入 debug summary / 自动化测试。

### 描述层

`FWacomInteractionTargetHandle`（`WacomCore/Public/Types/WacomInteractionTargetTypes.h`）是纯数据 struct：

| 字段 | 用途 |
|---|---|
| `TargetKind` | None / World / Card / Zone |
| `WorldTargetId` | World 目标的 FGuid |
| `CardInstanceId` | Card 目标的 FGuid；first-person hand 当前由卡槽 hover / visual update 提供 |
| `ZoneId` | Zone 目标的 FName（命中来源待接入）|
| `TargetTag` | 目标语义标签，例如 `Interaction.Target.Battle.EnemyPart` |
| `StableTargetId` | 稳定 authored/data ID，例如敌人 `PartId` |
| `SourceObject` | 命中来源 Component 弱引用 |
| `WorldLocation` / `ScreenPosition` | 命中位置 |

### 规则层

Battle 已接入 `UBattleSession::CanTargetWithCard(CardInstanceId, FWacomInteractionTargetHandle)` 和 `ValidateTargetWithCard(...)`，当前用于 TargetSelect 可选部位视图、first-person drag/drop resolver 的 world target 合法性判断、`TargetMode=HandCard` 的 Card target 合法性判断，以及拖拽预览。`CanTargetWithCard()` 只返回 bool，内部转调 validation；`ValidateTargetWithCard()` 会区分 self target、源卡不是 HandCard、目标不在手牌、selected discard/exhaust 不支持锚点、非法 world target 等原因。`Target.SelectedHandCard` 的加费 / 减费允许普通手牌和左右手锚点；指定弃置 / 消耗只允许普通手牌。Run resolver 后续接入。

### 当前范围

- [x] World 目标：通过 `UWacomInteractionTargetComponent` 命中；Battle enemy part 由 `UWacomBattleEnemyPartWorldTargetBridgeComponent` 绑定运行时 ID
- [x] Run World 目标：通过 `UWacomRunWorldInteractionTargetBridgeComponent` 标记 `Interaction.Target.Run.Object` 并提供 probe preview；规则层 resolver 后续接入
- [x] Card 目标：first-person hand slot hover / visual update 已通过 `UWacomFirstPersonCardLayerWidget` 与 `UWacomFirstPersonCardAnchorComponent` 暴露；旧 `UCardWidget / UHandPanel` 不作为本轮维护入口
- [ ] Zone 目标：通过 UMG drop area 命中（后续接入）
- [x] Battle 规则层 Resolver：`UBattleSession::CanTargetWithCard`
- [ ] Run 规则层 Resolver（后续接入）

### 不变项

- 原有 `IWacomWorldInteractable` 探索期 E 键交互不变。
- Run target probe preview 只表达“鼠标当前命中该物体”，不表达可交互、可提交或某张卡能作用。
- 原有 BattleHUD target registration / TargetCue 表现不变。
- `TryRouteBattleSceneTargetClick` 的 `protected virtual` 测试 seam 不变。
