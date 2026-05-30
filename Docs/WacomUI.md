---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-05-31
tags:
  - wacom/ui
  - wacom/commonui
  - wacom/mvvm
  - wacom/wbp
---

# WacomUI 文档

> [!info] 本文职责
> 本文是 Wacom UI 表现层的当前事实入口。[[WacomApp]] 只记录 App / GameMode / 输入 / 世界交互总控；具体 UI 数据流、Widget 职责、Toast、Screen 和 WBP 承接口径放在这里。

> [!warning] 阅读边界
> 本文记录 UI 数据流和表现合同，不定义战斗、Run、卡牌或资产规则。

## §1 UI 总原则

UI 不直接修改战斗或 Run 状态。UI 读取 Snapshot、ViewData 或 ViewModel，然后把玩家意图提交给上层控制者：

| 领域                      | 数据来源                                                             | 命令出口                                                                                                                                                             |
| ----------------------- | ---------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ExplorationHUD          | `UWacomRunViewModelProvider -> UWacomRunViewModel`               | 无；只读显示探索状态和交互提示                                                                                                                                                  |
| Run Screen / 菜单类 Widget | Run Snapshot / ViewModel / Presentation ViewData                 | `URunSession` 写 API，通常经 PlayerController / Screen 调用                                                                                                             |
| 背包                      | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图，私有 `FWacomBackpackCommandFlow` 编排 `MoveInstance / DeleteCardForGoldByInstance / SetSpecialZoneCardBattleEnabled`、Toast 和 Confirm |
| 商店                      | `URunSession::BuildCurrentShopSnapshot()`                        | `UWacomShopScreen` 接收 UI 意图，私有 `FWacomShopScreenFlow` 编排 `PurchaseShopOffer / EndShopVisit` 和 Toast                                                              |
| 探索事件                    | `URunSession::BuildCurrentRunEventSnapshot()`                    | `UWacomRunEventScreen` 接收 UI 意图，私有 `FWacomRunEventScreenFlow` 编排 `ChooseRunEventOptionWithResult / EndRunEvent` 和 Toast                                          |
| 战斗                      | `FBattleSnapshot`、`FBattleEvent`、BattleSession ViewData          | `UBattleHUD` 接收玩家意图，私有 BattleHUD flow helper 编排命令提交、目标选择、事件日志和击倒弹窗                                                                                               |

Widget 可以有 C++ fallback 布局，但 C++ 的职责是协议、生命周期和兜底显示；正式视觉由 WBP 承接。
复杂 Widget 的流程逻辑应收口到 `WacomApp/Private` 的 command flow / coordinator helper。Screen 负责 View、CommonUI 生命周期、绑定和重建；helper 负责命令编排、确认弹窗、Toast 和关闭访问等副作用。

### Public Widget API 与测试口

复杂 Widget 的 public 头只保留 WBP 绑定、运行时生命周期和真实玩家意图入口。不要为了自动化测试在 `WacomApp/Public` 暴露 callable 的 `ForTest`、`ForAutomationTest`、`OverrideForTest`，也不要新增 Blueprint 可见测试 `UFUNCTION`。测试不应把未来 WBP、CommonUI 生命周期或 MVVM 重构锁死在当前 C++ fallback 结构上。

需要测试复杂 Widget 行为时按以下优先级处理：

1. 在 `WacomTests` 中写 tests-only probe subclass，通过真实激活、刷新和玩家意图入口观察结果。
2. 如果运行时代码本身需要可扩展点，提炼 protected production seam；它必须对生产行为有清晰语义。
3. 只有前两者不适合时，才使用 automation-only private friend / test-access；这类入口不能是 `UFUNCTION` 或 Blueprint API，并应尽量放在 `WacomApp/Private` 或测试模块内。

---

## §2 CommonUI 层级

`UWacomGameUIManagerSubsystem` 持有 `UWacomPrimaryGameLayout`。PrimaryLayout 跟随当前 PlayerController 创建，切关卡时 TearDown 并重建。

默认布局资产路径：

- `/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C`
- 父类：`UWacomPrimaryGameLayout`

### 顶层 UI 类解析

Wacom UI Settings 使用 `UWacomUIDeveloperSettings` 作为项目级软类注册表。顶层 UI 类解析优先级为：

```text
Wacom UI Settings
-> C++ fallback
```

`AWacomPlayerController` 不再提供顶层 ScreenClass 覆盖入口。需要替换顶层界面时，在 `Edit > Project Settings > Wacom UI Settings` 注册对应类。

细节：

| 入口 | 优先级 |
|---|---|
| PrimaryLayout | `Wacom UI Settings.PrimaryLayoutClass` -> `/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C`；PrimaryLayout 不走 C++ fallback，解析失败则不创建根布局 |
| BackpackScreen | `UI.Widget.BackpackScreen` settings 注册 -> `UWacomBackpackScreen` C++ fallback |
| PauseMenuScreen | `UI.Widget.PauseMenuScreen` settings 注册 -> `UWacomPauseMenuScreen` C++ fallback |
| ShopScreen | `UI.Widget.ShopScreen` settings 注册 -> `UWacomShopScreen` C++ fallback |
| RunEventScreen | `UI.Widget.RunEventScreen` settings 注册 -> `UWacomRunEventScreen` C++ fallback |
| AppToast | `Wacom UI Settings.AppToastWidgetClass` -> `UWacomAppToastWidget` C++ fallback |

V2-B 状态：

- `BackpackScreen` / `PauseMenuScreen`：Router 通过 `UWacomGameUIManagerSubsystem::PushRegisteredWidgetToLayerAsync()` 打开；settings 软类未加载时走异步加载，缺失或失败时回到 C++ fallback。GameMenu 已有异步 pending 时，重复打开请求会被忽略。
- `ShopScreen` / `RunEventScreen`：同样通过 async push 打开；`BeginShopVisit()` / `BeginRunEvent()` 只在异步类解析完成、stale guard 通过且即将 Push 前执行。Push、Cast 或 Refresh 失败时立即 `EndShopVisit()` / `EndRunEvent()` rollback。
- `PrimaryLayout`：仍同步创建；settings 失败后只尝试固定 `WBP_PrimaryGameLayout` 路径 fallback。
- `AppToast`：仍同步解析并直接 `AddToViewport`，不进入 CommonUI async push。

### Wacom UI Settings 配置校验

Wacom UI Settings 是顶层 UI WBP 的唯一项目级覆盖入口，不要求本轮配置所有顶层 WBP。未配置或配置加载失败时，仍按上表回退：PrimaryLayout 只允许固定路径 fallback / null；其余顶层界面回到对应 C++ fallback。其中 Shop / RunEvent / PauseMenu 等 `WidgetClasses` 缺失属于合法 fallback，不是错误。

编辑器 Data Validation 应检查以下错误：

- `PrimaryLayoutClass` 非空时，必须继承 `UWacomPrimaryGameLayout`。
- `AppToastWidgetClass` 非空时，必须继承 `UWacomAppToastWidget`。
- `WidgetClasses` 的 tag 必须属于 `UI.Widget.*` 命名空间。
- `WidgetClasses` 的 class 必须继承 `UWacomActivatableWidget`。
- `WidgetClasses` 中重复 tag 是错误。
- `WidgetClasses` 中空 class 是错误；如果希望使用 fallback，应删除该条目而不是保留空 class。

`WBP_PrimaryGameLayout` 必须绑定 4 个 `UCommonActivatableWidgetStack`：

| 控件名 | 对应 Layer | 用途 |
|---|---|---|
| `GameLayerStack` | `UI.Layer.Game` | 探索 HUD、BattleHUD |
| `GameMenuLayerStack` | `UI.Layer.GameMenu` | 主菜单、暂停菜单、背包、商店、探索事件 |
| `ModalLayerStack` | `UI.Layer.Modal` | 确认弹窗等阻塞式弹窗 |
| `OverlayLayerStack` | `UI.Layer.Overlay` | CommonUI 内 overlay 入口；当前 AppToast 不走该 Stack |

| Layer | 用途 |
|---|---|
| `Game` | `UWacomExplorationHUD`、`UBattleHUD` |
| `GameMenu` | 主菜单、暂停菜单、背包、商店、探索事件 |
| `Modal` | `UWacomConfirmDialog` |
| `Overlay` | CommonUI 内的 Overlay 层；当前 AppToast 不走该 Stack |

Game 层同一时间只应有一个主要 HUD 处于 active 状态：

```text
探索 BeginPlay -> Push ExplorationHUD 到 Game 层
EnterBattle -> Push BattleHUD 到 Game 层，ExplorationHUD 进入非 active 状态
ExitBattle -> Pop BattleHUD，ExplorationHUD 重新 active 并补刷新
```

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入；战斗 HUD 和探索 HUD 仍声明自身期望的 UI input config，但底层 gameplay profile 由 `UWacomInputContextCoordinatorSubsystem` 统一应用。探索期固定使用 Run Tunnel 输入模型：Coordinator 切到 `All + NoCapture`、显示鼠标并保持探索 IMC；普通隐藏鼠标 FPS FreeLook 不再是正式玩家路径。`UWacomMenuWidgetBase` 负责 Menu 模式下的返回键口径：ESC 和 Gamepad FaceButton Right 触发 Back 请求，默认广播 `OnBackRequestedNative` 后 `DeactivateWidget()`；子类只在语义不同（例如 ConfirmDialog 把 Back 当 Cancel）时覆盖。

---

<a id="wacomui-toast"></a>
## §3 Toast 与反馈

三类 Toast 分工不同，不互相合并：

| 类型 | 出口 | 用途 |
|---|---|---|
| 交互提示 | `UWacomExplorationHUD::SetInteractToastVisible` | 范围内提示，例如“按 E 战斗/商店/查看事件” |
| 战斗事件 | `UEventToast + UWacomBattleEventPresentationBuilder` | 战斗内即时事件提示 |
| AppToast | `UWacomAppToastSubsystem` | 战斗外获得卡牌、金币变化、背包移动/删牌、商店、RunEvent 反馈 |

### AppToast

`UWacomAppToastSubsystem` 是战斗外通用反馈出口。它持有唯一 `UWacomAppToastWidget`，直接 `AddToViewport(ZOrder=10000)`，不进入 CommonUI Stack。

Toast WBP 注册口径：

- 父类：`UWacomAppToastWidget`
- 在 `Edit > Project Settings > Wacom UI Settings` 的 `AppToastWidgetClass` 注册正式 WBP。
- 未注册、软类加载失败或类型不匹配时，回退 `UWacomAppToastWidget` C++ fallback；不再按旧固定 WBP 路径尝试加载。
- 可选绑定：`Container : VerticalBox`

当前生命周期：

- 探索局开始时 PlayerController 会预热；首次 Toast 也会懒加载兜底。
- 无消息时 Widget `Collapsed`；入队后 `HitTestInvisible`；消息播完后只隐藏，不销毁。
- Widget `SetIsFocusable(false)`，`GetDesiredInputConfig()` 返回空，不改变探索或菜单输入。
- `UWacomAppToastSubsystem` 跨关卡跟随 GameInstance 存活，但缓存 Widget 只在属于当前 World 和当前本地 PlayerController 时复用；若发现旧 World、旧 PlayerController 或无效 Owner，会先 `RemoveFromParent()` 并清空引用，再用当前本地 PlayerController 重建。自动化测试注入的无真实 World/PC transient widget 属于测试 override，可继续复用。
- Subsystem `Deinitialize()` 会移除并清空缓存 Widget，避免退出 GameInstance 后残留 viewport widget。
- `FWacomAppToastView` 保留 `MessageText / Tone / IconKey / LifetimeOverride`；C++ fallback 第一版只显示文字。

当前接入：

- 商店购买成功：`获得卡牌：{CardName}`。
- 商店购买失败：按 Offer ViewData 的 `DisabledReason` 显示“金币不足 / 该商品已购买 / 商品不可购买”。
- 背包移动成功/失败：成功显示移动到目标区，失败显示通量满、备战满、特殊区满等原因。
- 背包删牌成功/失败：成功显示销毁卡牌和获得金币，失败显示固有卡、最后容量来源卡、未持有卡等原因。
- RunEvent 选项结果/不可用原因：由 `UWacomRunEventPresentationBuilder` 转成中文 Toast；卡牌支付成功时会使用结果里的支付卡 Definition 追加 `交出卡牌：{CardName}`。V0-AS 后成功结果还会在效果 Toast 之后追加 outcome Toast：事件完成或关闭显示 `事件已结束`，节点切换且事件仍打开时显示 `进入：{节点标题}`。

### 战斗事件表现队列、Toast 与日志

`UWacomBattleEventPresentationBuilder` 把 `FBattleEvent` 转成 `FBattleEventPresentationView`。ViewData 包含 `MessageText / bShouldDisplay / VisualTone / IconKey`，但当前 Toast 和日志主要消费文字。

- `FWacomBattleEventPresentationQueue` 负责把 `FBattleEvent` 按顺序调度为 TargetCue、Toast、短暂停顿和击倒弹窗 step；队列播放期间 `BattleHUD` 进入 `Resolving`，阻止继续提交战斗命令。
- TargetCue 是 `WacomApp` 内的轻量表现信号。当前 `DamageDealt / EnemyPartHpEmptied` 会经 `BattleHUD` 私有目标注册表按敌方部位实例 ID 分发；2D `EnemyInfoBar` 刷新时把当前 `EnemyPartWidget` 注册进去。V0-W 后 TargetCue 明确区分 `BattleEvent` 与 `TargetConfirmed`：前者仍由表现队列发送，用于真实伤害 / 破坏反馈；后者只在目标卡成功提交后立即发送，用于表示“目标点击已被接受”。`TargetConfirmed` 不代表伤害已经发生，也不替代后续 `DamageDealt / EnemyPartHpEmptied` cue。V0-X 后场景对象使用 `UWacomInteractionTargetComponent + UWacomBattleEnemyPartWorldTargetBridgeComponent`：前者提供统一 world target handle，后者用稳定 `UEnemyPartDefinition::PartId` 在 snapshot / UIState 刷新后绑定当前运行时 `PartInstanceId`，并接收目标确认、伤害、破坏 cue。Bridge 收到目标确认 cue 时播放较轻 scale pulse；收到命中 / 破坏 cue 时播放 damage / destroyed scale pulse，并在 timer 结束后恢复。进入 `TargetSelect` 且该部位可选时，同一 primitive 会播放持续的轻量提示，退出选择或不可选时恢复。战斗场景点击主路径是 `AWacomPlayerController::InputKey()` 在左键 Release 且 HUD 处于 `TargetSelect` 时执行 `GetHitResultUnderCursor(ECC_Visibility)`，从命中 Actor 的组件中查找 `IWacomInteractionTargetProvider`，只有 `TargetKind=World` 且 `TargetTag=Interaction.Target.Battle.EnemyPart` 的 handle 会转发到 `BattleHUD->OnEnemyPartClickedByUser()`；如果 Release 先落到 BattleHUD 根层，`BattleHUD::NativeOnMouseButtonUp()` 会调用同一个 PlayerController router 作为兜底。真实 hover、鼠标命中提示、材质描边和正式敌人 Actor 留后续。队列不依赖具体 Widget 或未来场景 Actor。
- 目标注册表 V0 是单 handler 覆盖策略：同一 `PartInstanceId` 后注册者替换旧注册者；未来如果需要 2D Widget 与 3D Actor 同时表现，再升级为 priority 或 multicast。
- 开启场景绑定原型时，`BattleHUD` 会在 `EnemyInfoBar` 因目标选择状态刷新并重新注册 2D 部位后，再同步一次场景目标。因此 V0 语义是：同一部位上，场景 target 保持为当前表现 / 点击 provider。
- PIE 验证时，`UWacomBattleEnemyPartWorldTargetBridgeComponent.VisualTargetComponent` 可以保持 `None`；Bridge 会自动查找 Owner 上第一个 `UPrimitiveComponent` 作为反馈目标。推荐原型结构是 `Actor -> StaticMeshComponent(Cube) + WacomInteractionTargetComponent + WacomBattleEnemyPartWorldTargetBridgeComponent`。
- 当前 PIE 场景点击由 `AWacomPlayerController` 显式 cursor trace router 处理；路由只在左键 Release 且 HUD 已进入 `TargetSelect` 时消费。BattleHUD 根层 `MouseButtonUp` 兜底也复用同一路由，覆盖单击有效阶段落在 MouseUp 的 PIE 情况。目标 Actor 通过 `UWacomInteractionTargetComponent` 暴露统一 handle，Battle enemy part bridge 负责把稳定 `PartId` 绑定成运行时 `PartInstanceId`；选择需要敌方部位目标的卡牌进入 `TargetSelect` 后，单击匹配且阻挡 Visibility 的场景 primitive 应直接完成目标选择，不再需要关卡蓝图临时 trace 或 `Primitive.OnClicked`。
- 场景目标排障优先看 Bridge 的 `GetBattleWorldTargetDebugSummary()` 或 Details 面板里的 `LogBattleWorldTargetDebugSummary()`：确认 `PartId`、运行时 `PartInstanceId`、`Bound=true`、`Registered=true`、`Targetable=true`、`LastBind=MatchedPartId`。同时检查同 Actor 上 `WacomInteractionTargetComponent` 的 `TargetId` 与 `InteractionTargetTag=Interaction.Target.Battle.EnemyPart`。PlayerController 上的 `bLogBattleSceneTargetClickRouting` 默认关闭，临时打开后只输出 trace 和 handle 路由状态。
- Run / 探索场景目标 V0-AJ 使用 `UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent`。Bridge 填写 `RunTargetStableId`，自动生成或复用运行时 `TargetId`，并把通用 target 标记为 `Interaction.Target.Run.Object`。PlayerController 的 Run probe 只在 Exploration 下接受该 tag；`bEnableRunWorldTargetProbePreview` 开启时会低频驱动命中 Actor 的轻量 scale preview。该 preview 是表现和诊断，不是点击提交、不是 Run 规则 resolver，也不替代 `IWacomWorldInteractable + E`。
- Run target PIE 验证最小结构：`Actor -> Primitive(阻挡 Visibility) + WacomInteractionTargetComponent + WacomRunWorldInteractionTargetBridgeComponent`。鼠标指向时 primitive 应有轻量缩放；Details 面板调用 `GetRunWorldTargetDebugSummary()` 或 `LogRunWorldTargetDebugSummary()` 应看到 `TargetTag=Interaction.Target.Run.Object`、有效 runtime id、stable id 和 preview 状态。
- `UEventToast` 负责单条战斗 Toast 的显示和倒计时，不负责文案判断或事件排序。
- `UBattleEventLogPanel` 是 `BattleHUD` 内部半屏日志抽屉，不通过 CommonUI Layer push/pop。
- `BattleHUD` 保存本场 `BattleEventLogHistory`，切换 Session 时清空日志和表现队列。
- `KnockdownChoiceRequested` 不再当帧直接 Push modal；它由表现队列延后到前序事件播放后再打开。
- `HandZoneChanged` 等 `bShouldDisplay=false` 的事件不会进入 Toast 或日志。

### First-person Card target bridge

V0-Y 后 first-person hand 的卡槽也接入统一 `FWacomInteractionTargetHandle` 合同。V0-AD 后 Card target 不再只做 probe：`TargetMode=HandCard` 的源卡拖到另一张 first-person 手牌上释放时，BattleHUD drop resolver 会解析为 `PlayCardCardTarget`，提交 `PlayCard` 的 `TargetCardInstanceId`，用于 `Target.SelectedHandCard` 效果。非 `HandCard` 源卡拖到卡牌上仍保持 probe / deny，不提交战斗规则。

- `UWacomFirstPersonCardLayerSlotWidget` 对 active、可见、非 exiting 且有有效 `CardInstanceId` 的 slot 构建 `TargetKind=Card` 的 handle。
- `ScreenPosition` 使用当前 `VisualSlotView.ScreenPosition`，所以 slot motion、hover、pending 和重排动画中暴露的是玩家实际看到的位置。
- Card target 不要求目标卡 `bIsPlayable=true`。不可打卡仍可成为指定手牌效果的目标；是否合法由 Battle / Run resolver 判断。
- `UWacomFirstPersonCardLayerWidget` 和 `UWacomFirstPersonCardAnchorComponent` 转发 hover、unhover 和 hovered visual update 的 card target 事件；V0-Z 的 drag view 会把当前 card target 带给 BattleHUD。Battle 中 `UBattleSession::ValidateTargetWithCard()` 当前支持 `TargetKind=Card + TargetMode=HandCard`，并拒绝 self、无效卡、不在手牌的卡，以及被 `UCardDefinition::HandCardTargetFilter` 禁止的普通手牌 / 左右手锚点 / keyword 条件。`CanTargetWithCard()` 只保留为 bool 包装。
- outgoing、投影失败、关闭 first-person interaction、runtime source 清理和 layer clear 都会清理 hovered card target，避免后续拖拽系统拿到幽灵目标。

### Run first-person card source

V0-AK 后探索期有一个独立的 first-person card source：`UWacomRunFirstPersonCardSourceComponent`。它由 `AWacomPlayerController` 持有，并绑定当前 `URunSession::OnRunStateChangedNative`。处于 Exploration 且 source active 时，它读取 `URunSession::BuildBackpackStorageSnapshot()`，把 `BattleDeckPhysicalCards` 和可选 `BattleDeckProjectedCards` 转为 `FWacomFirstPersonCardLayerEntry` 后写入 `UWacomFirstPersonCardAnchorComponent` 的 runtime source `RunFirstPersonBattleDeck`。

- 该 source 只做展示和诊断。Run 卡牌 entry 使用真实 `FCardInstance.InstanceId` 和 `UWacomCardPresentationBuilder` 生成的卡面 ViewData，但不会提交 Run 命令。
- 为了视觉可读性，entry 保持 `bIsPlayable=true` 且 `CardViewData.bDisabled=false`；同时 source 写入后会确保 `SetBattleHandInteractionPrototypeEnabled(false)`，所以不会启用战斗手牌 click / hold / drag 交互。
- V0-AL 后，探索期 `GameMenu` 默认会压制 Run BattleDeck source：Backpack、Pause、Shop、RunEvent 打开时清理当前 `RunFirstPersonBattleDeck` 显示，关闭后恢复并刷新最新 Run BattleDeck。这个压制由 `UWacomMenuWidgetBase` 激活 / 失活通知 PlayerController 维护，所以 Widget 自己 `DeactivateWidget()` 也能恢复，不依赖单一路由入口。
- 需要卡牌参与菜单交互的界面可以通过 PlayerController / source component 显式申请 menu lease。V0-AN 后推荐菜单使用 `UWacomMenuWidgetBase::SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards()`，提交 `FWacomRunMenuCardLeaseRequest`，由系统从玩家当前真实持有卡实例构建 `FWacomFirstPersonCardLayerEntry`；不要在蓝图中手拼 entry。Request 可按 Definition、CardId、显式 InstanceId、RequiredKeywords、BlockedKeywords 和 include zone 开关筛选，空筛选默认拒绝，除非显式开启 `bAllowAllOwnedCardsWhenNoFilter`。候选来源是 `FRunState.Backpack -> BattleDeck -> BurdenZone -> SpecialZones.Cards`，不读取 `BattleDeckProjectedCards`，避免 SpecialZone 入战投影重复显示。Active menu lease 优先于 GameMenu suppress，用自己的 runtime source id 显示候选卡；同一时间只允许一个 lease，清理 lease 后回到“仍在菜单则压制、菜单已关闭则恢复 BattleDeck”的规则。菜单基类拥有的 lease 会在 `NativeOnDeactivated` 自动清理。
- 开发验证可在 PIE Exploration 下执行控制台命令 `Wacom.OpenRunMenuCardLeaseTestMenu` 打开 C++ 验证菜单 `UWacomRunMenuCardLeaseTestMenu`。该菜单默认用 `AllowedCardIds=PoisonFang` 申请 owned menu lease，并内置 `ZoneId=RunEvent.Pay.Fang` 的 `UWacomRunMenuDropTargetWidget`；释放到该区域会通过 V0-AO drop intent prototype 支付这张具体持有卡实例。
- V0-AM 后，active menu lease source 会临时启用 first-person card hold / drag 手势，但只由 PlayerController 做菜单 drop probe，不走 BattleHUD 出牌路径。菜单中的 `UWacomRunMenuDropTargetWidget` 暴露 `FWacomInteractionTargetHandle(TargetKind=Zone)`；拖拽更新时 Controller 用 drag view 的 DPI-aware widget position 命中最上层可见 / enabled drop target，驱动目标区域 probe preview，并把 `ZoneProbe` feedback 写回 first-person layer。V0-AO 后 preview 和 release 统一走 `ResolveRunMenuCardDropIntent()`：默认菜单仍是 probe-only；owning menu 通过通用 drop intent hook 返回 `SubmitZoneTarget` 时，release 才可能提交。`ControllerDestroyOwnedCard` policy 由 PlayerController 调 `URunSession::DestroyCardByInstance(SourceCardInstanceId)` 永久移除那张玩家真实持有卡，不给金币；`MenuHandled` policy 由菜单自己提交领域事务并回填提交结果。
- V0-AQ 后，`UWacomMenuWidgetBase` 提供 `ResolveRunMenuFirstPersonCardDropIntent()` 和 `SubmitRunMenuFirstPersonCardDropIntent()` 蓝图 hook。默认实现只返回 `ProbeZoneTarget`；菜单可以返回 `SubmitZoneTarget + ControllerDestroyOwnedCard` 让 Controller 执行 prototype destroy，也可以返回 `SubmitZoneTarget + MenuHandled` 自己调用领域 API 并把 `bSubmitted / RejectReason / RunValidationReason` 写回结果。V0-AP 的 `UWacomRunEventScreen` 使用 `MenuHandled`，调用 `ChooseRunEventOptionWithPaidCardResult()` 在 RunEvent 事务中移除精确 instance、执行 Effects 并推进节点。V0-AR 后支付选项行会显示候选数量或缺失原因，合法拖入时 Zone 使用 `SubmitReady`，成功提交后使用 `Submitted`，并通过 AppToast 告知交出的具体卡。V0-AS 后同一次成功结果还会继续显示节点 / 事件 outcome Toast，例如 `事件已结束` 或 `进入：{节点标题}`。提交后 provider-backed lease 会按原 request 从最新 RunState 重建候选；没有剩余候选时清空 lease，并在菜单仍打开时保持 GameMenu suppress，避免回到 anchor card fallback。
- 进入战斗、Controller EndPlay、source disabled 或 runtime clear 时会清理 `RunFirstPersonBattleDeck` 和 active menu lease，并取消可能残留的 first-person drag gesture。退出战斗回 Exploration 后重新刷新当前 Run BattleDeck。
- Debug 可在组件上调用 `GetRunFirstPersonCardSourceDebugSummary()` 或 `LogRunFirstPersonCardSourceDebugSummary()`，关注 `Enabled / Active / SuppressedByGameMenu / HasLease / LeaseId / LeaseSource / HasRun / HasAnchor / Physical / Projected / Entries / Last`，以及 provider 的 `LeaseId / SourceId / Result / Candidates / Considered / Debug`。

当前 Run first-person source 不做 Run card-to-world / card-to-card resolver，不替代 `IWacomWorldInteractable + E`，也不迁移背包既有 `UWacomZoneDropTarget` UMG DragDrop。V0-AP 只把 RunEvent 选项卡牌支付接入该桥：RunEventScreen 自动为支付选项创建 Zone target、申请候选卡 lease，并在 release 时提交 paid-choice 事务。Shop / Backpack 等菜单的 first-person card selection 仍是后续方向。

---

## §4 Run UI 与 MVVM

Run 域 Widget 使用 ViewModel + Provider 订阅模型，避免 Widget 直接订阅业务层。

```text
RunSession 写状态
  -> OnRunStateChangedNative
  -> UWacomRunViewModelProvider 读取 RunState
  -> UWacomRunViewModel FieldNotify 字段更新
  -> OnRunViewModelRefreshedNative
  -> Widget 读取 ViewModel 并刷新
```

当前状态：

- `UWacomRunViewModelProvider` 是 GameInstance Subsystem。
- `UWacomRunViewModel` 是纯数据，不持有 Session 指针。
- C++ Widget 订阅 Provider 的粗粒度多播并手动 SetText。
- FieldNotify 字段已就位，但 WBP ViewBinding 尚未消费。
- Widget 在 `NativeOnActivated` 补订阅和补刷新，处理 CommonUI Reactivate。

`UWacomExplorationHUD` 位于 Game 层，显示时段、节点、天数、手指、经验、压力分项和交互 Toast。战斗中 BattleHUD 压到 Game 层顶端；战斗结束后 ExplorationHUD Reactivate 并补刷新。

---

## §5 背包 UI

`UWacomBackpackScreen` 位于 `GameMenu` 层。它负责 View 所有权、CommonUI 生命周期、WBP 绑定、拖拽入口、详情面板生命周期和全量重建；规则真相仍在 `URunSession`。
C++ fallback 布局由私有 `FBackpackFallbackLayoutBuilder` 搭建，运行时 DropTarget / WrapBox / 详情层由私有 `FBackpackRuntimeZoneBuilder` 创建；移动 / 删除 / 入战开关的命令编排收口到私有 `FWacomBackpackCommandFlow`。WBP 绑定字段和玩家意图入口仍保留在 `UWacomBackpackScreen`。

背包 UI 对玩家已拥有卡只提交 `InstanceId`。卡牌 Definition 可用于展示、卡名 fallback 和资产语义说明，但不能作为 UI 删除或移动某张已拥有卡的身份。

当前结构：

| 区域 | 当前职责 |
|---|---|
| 删牌区 | `UWacomDeleteZoneDropTarget` 只做预览和转发删牌意图；预览、确认前校验和确认后提交都使用卡牌 `InstanceId`；Definition 只用于卡名 fallback |
| 备战区 | `UWacomZoneDropTarget` 显示 `BattleDeck` 和已入战 SpecialZone 投影卡，只做预览和转发移动意图 |
| 通量内容区 | 显示物理位于 `Backpack` 的通量内容，A 类容器也作为内容卡显示 |
| 特殊存放区 | 每张 B 主卡对应一个 `UWacomSpecialZoneWidget` |
| 负重区 | 仅 `BurdenCount > 0` 时显示卡牌；卡牌可拖出，但区域背景不接收 drop |

数据和刷新：

- 列表数据来自 `RunSession->BuildBackpackStorageSnapshot()`。
- 顶部金币、容量等标量来自 Run ViewModel。
- 操作成功后不做局部 patch，当前通过 `RebuildAll()` 全量重建。
- `RebuildAll()` 已拆为顶部、备战区、通量区、特殊区和负重区刷新函数。

交互：

- DropTarget 只读 RunSession 做 hover 预览校验；drop 时把 `UWacomCardDragOperation` 转发给 `UWacomBackpackScreen`，再由 `FWacomBackpackCommandFlow` 处理实际命令。
- 普通 drop 由 `FWacomBackpackCommandFlow` 先调用 `ValidateMoveInstance()`，再调用 `MoveInstance()`，并统一发 AppToast。
- Delete drop 由 `FWacomBackpackCommandFlow` 先调用 `ValidateDeleteCardForGoldByInstance()`；失败直接 AppToast，成功才弹 ConfirmDialog，确认后提交 `DeleteCardForGoldByInstance()`。
- SpecialZone 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`，再由 `FWacomBackpackCommandFlow` 调 `SetSpecialZoneCardBattleEnabled()`。
- 卡牌 hover 详情由 `UWacomBackpackScreen` 管理，数据来自 `UWacomBackpackScreenPresenter` 和 `UWacomCardPresentationBuilder`。

背包 WBP 制作细节见 `UI_Backpack_WBP_Binding.md`。未来正式 WBP、拖拽 polish、增量刷新等计划见 `Roadmap.md` 和 `TechDebt.md`。

---

## §6 商店 UI

`UWacomShopScreen` 位于 `GameMenu` 层。商店打开不切换 `EGameFlowState`。Screen 负责 View 所有权、CommonUI 生命周期、商品列表重建和按钮绑定；购买、关闭访问和 Toast 副作用收口到私有 `FWacomShopScreenFlow`。

当前流程：

- 商店公开请求入口在 PlayerController；内部由 Router 先关闭已有 `GameMenu` 顶层，再调用 `RunSession->BeginShopVisit(PersistentId, Offers)`。
- Screen 刷新时读取 `BuildCurrentShopSnapshot()`。
- `UWacomShopPresentationBuilder` 把 `FRunShopOffer + 当前金币` 转成 `FWacomShopOfferPresentationView`。
- Offer 行只渲染 ViewData 并广播购买请求，不直接解析 `CardDefinition` 或判断金币。
- 点击购买由 `FWacomShopScreenFlow` 调 `PurchaseShopOffer(OfferId)`，成功 / 失败后统一发 AppToast，Screen 再刷新商品列表和金币。
- `NativeOnDeactivated` 的关闭访问路径由 `FWacomShopScreenFlow` 调 `EndShopVisit()`，每次 Screen 激活周期只结束一次；本次访问买过任意商品时 Run 层关闭时消耗 1 节点。
- 如果 Push 商店 UI 失败，Router 会调用 `RunSession->EndShopVisit()` 回滚刚 Begin 的 active shop。

正式商店视觉和 hover 详情属于后续表现项；商品规则、库存和节点消耗见 `WacomRun.md`。

---

## §7 RunEvent UI

`UWacomRunEventScreen` 位于 `GameMenu` 层，用于展示轻量事件图当前 Node。Screen 负责 View 所有权、CommonUI 生命周期、选项列表重建和按钮绑定；选项执行、事件关闭和 Toast 副作用收口到私有 `FWacomRunEventScreenFlow`。

当前流程：

- RunEvent 公开请求入口在 PlayerController；内部由 Router 先关闭已有 `GameMenu` 顶层，再调用 `RunSession->BeginRunEvent(PersistentId, EventDefinition)`。
- Screen 刷新时读取 `BuildCurrentRunEventSnapshot()`。
- ChoiceButton 显示 Label 和中文禁用原因；不可用选项也允许点击以弹出原因 Toast。
- 普通可用选项由 `FWacomRunEventScreenFlow` 调 `ChooseRunEventOptionWithResult(ChoiceId)`，Run 层执行条件、效果、跳转和完成标记。需要卡牌支付的选项普通点击会被 Run 层以 `RequiresCardPayment` 拒绝。
- 支付选项刷新时会创建 `UWacomRunMenuDropTargetWidget` 包住对应 choice row，ZoneId 来自 snapshot。Screen 聚合当前节点所有支付选项的候选 `InstanceId`，通过 owned menu lease 只显示这些可支付卡。`UWacomRunEventChoiceButton` 对支付选项显示 `拖入卡牌支付：{N} 张可用`；没有候选时显示 `缺少可支付卡牌：{Reason}`，Reason 优先来自 `PaymentDisabledReason`，再回退到 `DisabledReason`。拖卡 release 到匹配 Zone 且 `ValidateRunEventOptionCardPayment()` 通过时，Screen 接管 menu submit 并调用 `ChooseRunEventOptionWithPaidCardResult()`；该 API 在 RunSession 事务内移除精确实例、执行 Effects 并推进或关闭事件。
- Run menu Zone feedback 语义保持通用：`Probe` 表示命中但不会提交，`SubmitReady` 表示松手会由当前菜单提交，`Invalid` 表示当前卡 / Zone / 校验不合法，`Submitted` 表示提交成功。RunEvent 支付成功后 Toast 会显示交出的卡名；点击支付选项但不拖卡时仍显示 `需要拖入卡牌支付`。
- `FWacomRunEventScreenFlow` 使用 `UWacomRunEventPresentationBuilder` 把 `FRunEventChoiceResult` 转成 AppToast；不可用选项也由该 flow 发阻塞原因 Toast。成功结果的 Toast 顺序是具体效果、支付卡牌、最后 outcome：事件关闭 / 完成显示 `事件已结束`，节点切换且事件仍打开显示 `进入：{节点标题}`，节点标题为空时回退 `ResolvedNodeId`。
- `RemoveCard` 结果显示“交出卡牌：{CardName}”；`MarkEventCompleted` 默认不弹 Toast。
- 关闭事件界面时由 `FWacomRunEventScreenFlow` 调 `EndRunEvent()`；关闭型选项先由 Run 层清 active event，再 Deactivate。V0-AS 不新增常驻结果面板、不延迟关闭；关闭型事件仍即时关闭，outcome 只通过 Toast 告知。
- 如果 Push 事件 UI 失败，Router 会调用 `RunSession->EndRunEvent()` 回滚刚 Begin 的 active event。

事件规则、条件、效果和 PersistentId 口径见 `WacomRun.md` / `WacomData.md`。

---

<a id="wacomui-battle-ui"></a>
## §8 战斗 UI

战斗 UI 使用 Snapshot + Controller 推送模型，不走 Run MVVM。`UBattleHUD` 保留 WBP 绑定、生命周期、Snapshot 刷新和玩家意图入口；命令提交、目标选择、事件消费 / 日志 / 击倒弹窗编排收口到 `WacomApp/Private/UI/Battle` 的 BattleHUD flow helper。

规则合法性和状态变更见 [WacomBattle.md](./WacomBattle.md)，战后结算见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement)，WBP 制作合约见 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

理由：

- `BattleHUD` 战斗开始 Push、结束 Pop，生命周期短。
- 子 Widget 都在同一棵树里，由 `BattleHUD` 统一递归刷新。
- `FBattleSnapshot` 是值类型快照，适合一次性读当前战斗状态。
- 动态手牌和敌方部位列表由 HUD / Panel 直接重建更简单。
- C++ fallback BattleHUD 布局由私有 `FBattleHUDFallbackLayoutBuilder` 搭建；WBP 绑定字段、命令入口和刷新流程仍保留在 `UBattleHUD`。
- `FWacomBattleHUDCommandFlow / TargetingFlow / EventFlow` 只在 `WacomApp/Private` 内部使用，不新增 Blueprint 或跨模块 public API。

命令出口：

- 子 Widget 只发委托，不直接调 `Session->SubmitCommand()`。
- `UBattleHUD` 是战斗 UI 命令出口：PlayCard、Wait、EndTurn、KnockdownChoice 仍经它的 public 玩家意图入口进入；具体 `Session->SubmitCommand()` 和命令后收尾由 private command flow 执行。
- TargetSelect 是 HUD 持有的 UI 状态，目标选择 flow 负责状态转换和 `FBattleTargetSelectionView` 构建。
- Dialog 点击后回到 `BattleHUD->OnKnockdownChoiceSelected()`，Dialog 不直接改战斗状态。

目标选择：

- 点击需要敌方部位目标的手牌时，`BattleHUD` 进入 `TargetSelect` 并记录 `PendingTargetingCardId`。
- 再次点击同一张牌会取消目标选择。
- `BattleHUD::BuildTargetSelectionView()` 输出 UI-only 只读 ViewData，当前只区分是否选目标和部位是否破坏；最终合法性仍由 BattleSession / PlayCardResolver 校验。
- 当前 2D `EnemyInfoBar / EnemyPartWidget` 消费该 ViewData；`EnemyInfoBar` 同时把部位 Widget 注册到 `BattleHUD` 私有目标注册表，用于接收 TargetCue 并播放轻量命中/破坏反馈。场景侧原型使用 `UWacomInteractionTargetComponent + UWacomBattleEnemyPartWorldTargetBridgeComponent` 注册同一 `PartInstanceId`；Bridge 填写稳定 `PartId` 并在 snapshot 刷新、目标选择 UIState 刷新后自动绑定当前运行时实例 ID。Bridge 的 V0 视觉反馈包含命中/破坏短暂 scale pulse，以及 TargetSelect 可选目标的持续轻量提示；不改材质或规则状态。左键 Release 由 PlayerController 的 Visibility cursor trace router 转发为 HUD 目标选择意图，BattleHUD 根层 `MouseButtonUp` 兜底复用同一路由。V0 下场景 target 会替换 2D target，而不是并行同播。

手牌与详情：

- `UHandPanel` 把 `FHandQueueSnapshot` 转成 `FHandCardVisualEntry[]`，默认统一水平手牌带。
- `UCardWidget` 是战斗手牌交互外壳；`UWacomCardView` 只负责卡面显示。
- Hover 反馈移动 `HoverVisualRoot` 视觉层，不移动根命中区域，不改变布局占位。
- Hover 详情由 `UCardWidget` 上报、`UHandPanel` 转发、`BattleHUD` 管理；详情默认显示在卡牌左侧，空间不足时放右侧。
- `BattleHUD.bEnableCardDetailReadabilityPolish` 默认开启后，旧手牌详情和 first-person viewport 详情共用一套读牌 motion：初次 hover 短延迟后淡入，普通 unhover 淡出，已显示时切到另一张卡立即换数据和目标锚点，位置平滑跟随并用 side hysteresis 减少贴边左右跳。关闭该开关可恢复立即显示 / 隐藏。
- 进入目标选择、提交命令、刷新 Snapshot、切换 Session 或战斗结束时强制隐藏详情，不等待淡出。

### First-person card layer direction

Wacom 的正式卡牌表现方向不是把 `UCardWidget` 长期塞进 `WidgetComponent / RenderTarget`。主手牌应保持 HUD / UMG 渲染，以保证文本清晰、材质动画、hover 详情、拖拽和响应式布局稳定；但布局不再是传统固定 HUD，而是由 first-person card anchor 计算虚拟手牌平面，再投影为 UMG render transform。这样卡牌会像跟随玩家身体 / tunnel 前进的第一人称手牌，而不是死贴镜头的屏幕按钮。

当前已实现 `UWacomFirstPersonCardAnchorComponent`、默认关闭的 HUD debug projection、默认关闭的静态卡牌层、正式的战斗手牌呈现模式、first-person battle hand hover/click、first-person hover detail provider、first-person hover 详情跟随 / ZOrder 修正、Render Quality V0-A、默认 `BodyLocked` 的身体锁定锚点投影、默认 `Authored2D` 的美术可控手牌布局、默认 `SoftClampToViewport` 的离屏锚点柔性限制、`Authored2D` hand center screen smoothing、默认开启的 card slot motion polish、slot motion robustness / perf diagnostics、event-aware card transitions、transition origin / movement readability、TargetSelect pending focus polish、playable / hover / press feedback polish、layout preset / tuning profile、hover detail / inspect readability polish、hold inspect + drag / aim commit、drag camera look continuity、drop intent resolver contract，以及 Run exploration card source bootstrap / menu lease / menu Zone drop target / RunEvent card payment choice。静态层使用 `UWacomCardView` 或 `FirstPersonCardViewClass` 指定的专用卡面 WBP 渲染配置的测试卡牌或 placeholder 卡牌；战斗 hand adapter 由 `BattleHUD::BattleHandPresentationMode` 控制，非 `LegacyHandPanel` 时会把 `FBattleSnapshot.Hand.Cards` 按顺序转成 `FWacomFirstPersonCardLayerEntry`，覆盖同一个 first-person card layer 显示。探索期 adapter 由 `UWacomRunFirstPersonCardSourceComponent` 控制，把 Run BattleDeck 的物理卡和可选投影卡写入 source `RunFirstPersonBattleDeck`，只做可读展示，不启用 Battle hand 交互；GameMenu 可用 menu lease 显式接管该 layer 显示候选持有卡，release 到菜单 Zone 后由 owning menu 决定是 probe、prototype destroy，还是 RunEvent 事务提交。Entry 保留 `InstanceId / Zone / HandAnchor / Playable / PendingTargeting / TargetMode`，用于轻量视觉状态、手势分类和后续交互身份。

第一人称战斗手牌层不创建 `UCardWidget`，不做拖拽或直接出牌命令，不删除 `UHandPanel`，也不改变战斗命令出口。`BattleHUD::BattleHandPresentationMode` 是唯一配置入口：`LegacyHandPanel` 只使用旧手牌；默认 `FirstPersonHandWithLegacyFallback` 显示并启用 first-person hand，同时保留旧 `UHandPanel` 可见作为 fallback 和对照；`FirstPersonHandOnly` 在 first-person runtime hand 有效时折叠旧手牌，缺少 anchor、战斗结束、Session 切换或清理 runtime hand 时恢复旧手牌原始可见性。`FirstPersonCardAnchorComponent.ProjectionMode` 默认 `BodyLocked`，战斗中使用 Battle base rotation、Run Tunnel 中使用 spline base 锁定手牌中心锚点，不让 cursor look 重新计算 anchor；投影仍使用当前真实相机，因此镜头偏转时会有第一人称空间变化。`FirstPersonCardAnchorComponent.CardLayoutMode` 默认 `Authored2D`：只投影整副手牌中心点，每张卡牌的位置、下坠、扇形角度和层级由 2D 参数计算；`ViewportClampMode` 默认 `SoftClampToViewport`，投影成功的手牌中心可以离开视口安全区一段距离，超过 `SoftClampOffscreenAllowancePixels` 后再按 `SoftClampBlendRangePixels` 柔性拉回。`bEnableAnchorScreenSmoothing` 默认开启，只在 `Authored2D` 下平滑整副手牌中心，不平滑单张卡牌；它保留 Run Tunnel 前进下降、后退上升的空间趋势，但减少移动时中心点高频上下抖动。`bEnableCardSlotMotion` 默认开启，是第二层单卡视觉过渡：`UWacomFirstPersonCardLayerWidget` 用 `CardInstanceId` 或 `StaticIndex:{Index}` 复用 slot widget，`UWacomFirstPersonCardLayerSlotWidget` 再把视觉位置、角度、缩放和透明度追向 anchor 计算出的目标 slot；新卡按 `CardSlotEnterOffsetPixels / CardSlotEnterOpacity` 淡入，移除或投影失败的可见卡按 `CardSlotExitOffsetPixels / CardSlotExitDuration` 短暂淡出，目标大跳变超过 `CardSlotMotionResetDistancePixels` 时直接贴合。`bEnableEventAwareCardTransitions` 默认开启时，BattleEvent 只作为表现 hint 改变入场 / 离场表现；`bEnableReadableTransitionOrigins` 默认开启时，Drawn / Gained 可从手牌中心锚点进入，Played / Discarded 可从当前视觉卡槽离开，也可配置为 DPI-aware widget-space 的 `ViewportAnchor`，用于模拟牌堆、战斗空间或弃牌区方向。关闭 readable origins 会回到 V0-Q 的 offset-only 行为；snapshot diff 仍是手牌变化真相，事件缺失时回退通用 enter / exit。TargetSelect 中 pending 卡是手牌焦点：它上浮、放大、提高层级，并可按 `PendingTargetingAngleBlend` 向 0 度轻微归正；同一副手牌存在 pending 卡时，其他卡只轻微降透明，不下沉、不缩小。Pending 卡仍可点击取消 TargetSelect，但 hover 不再额外叠加 lift / scale / ZOrder，避免选中态抖动。`bEnableCardInteractionFeedback` 默认开启时，slot widget 用 C++ feedback overlay 表现可打 hover、pressed、confirm 和 deny 反馈；可打、非 pending 卡才获得 hover lift / scale / ZOrder，不可打卡仍可 hover 查看 Idle 详情但不会表现成可打姿态，点击不可打卡只播放 deny tint / 横向 shake，不广播 click intent。V0-P 后 layer 会维护 `FWacomFirstPersonCardLayerMotionDebugView`，统计 active / outgoing / RootCanvas child / ticking slot 和本次刷新创建、复用、移除、异常修复数量；刷新后会修复 active/outgoing 重叠、RootCanvas 未追踪 slot child、重复 key 和 outgoing 超限。同一 `CardInstanceId` 如果在 outgoing 淡出期间重新进入手牌，会回收原 outgoing widget 作为 active，避免形成幽灵 widget。`bLogCardLayerMotionDiagnostics` 默认关闭，只有手动排查时开启，且只在检测到不变量修复时输出简短 summary。`AnchorScreenSmoothingSpeed` 控制整副手牌中心跟手程度，`AnchorScreenSmoothingResetDistancePixels` 控制中心大跳变时直接贴合新位置；`CardSlotMotionSpeed / CardSlotOpacitySpeed` 控制单卡重排、hover、pending、抽牌、获得卡、出牌和弃置刷新时的过渡。`HardClampToViewport` 可复现旧的永远屏内行为，`AllowOffscreen` 可用于验证最接近 3D 空间物体的离屏表现。`FirstPersonHandOnly` 下手牌离屏不会自动恢复旧手牌入口，玩家需要转回视角或切换 presentation mode。`LegacyProjectedFan2D` 保留旧的每卡 3D 槽位投影表现，用于 PIE 对照。`LegacyWorldProjected` 保留旧的 LookInfluence 影响 anchor + 当前相机投影路径，用于调试对照。Hover 会改变 first-person layer 自身的 UMG transform / ZOrder，并在 Idle 状态下通过 `BattleHUD` 显示现有 `UWacomCardDetailPanel`；旧手牌详情使用 BattleHUD 内部 `CardDetailLayer`，first-person 详情使用独立 viewport popup host，默认层级 9999，高于 first-person card layer。V0-V 后两种战斗详情 host 共用 BattleHUD 的读牌 motion state：短 hover delay、淡入淡出、轻量 scale、位置平滑跟随和贴边 side hysteresis；TargetSelect、命令提交、BattleEnd、Session 切换和 runtime source 清理仍强制隐藏。详情数据来自 BattleHUD 最近一次 `FBattleSnapshot.Hand`，定位来自 first-person slot 的视觉屏幕锚点，并在 hovered slot 布局位置变化时实时跟随。左键释放可用卡只把有效 `CardInstanceId` 转发给 `BattleHUD->OnCardClickedByUser()`，后续合法性、TargetSelect 和命令提交仍走 BattleHUD 现有 flow。Runtime battle hand 数据源优先级高于静态预览，且空手牌也显示为空，不回退 placeholder。当前视觉状态只使用 UMG transform、opacity、C++ feedback overlay 和已有 disabled overlay，不可用卡降低 layer 透明度，手牌锚点卡轻微缩放。

V0-Z 后战斗 first-person 左键交互分三类：轻点在 hold delay 内释放，继续走 `OnCardClickedByUser()`；hold 不动进入 `Inspecting`，源卡移动到视口中间附近放大，BattleHUD 显示 first-person detail，释放不提交；拖动超过阈值后，无目标卡向上拖出并释放提交，有目标卡进入 `AimingTargetedCard` 并用 C++ arrow 指向鼠标，释放到合法 world enemy part 时调用现有 `SubmitPlayCard(CardId, TargetPartId)`。V0-AA 后拖拽仍保留 UMG mouse capture，但 drag view 会记录当前拖拽指针的 DPI-aware widget-space 位置和归一化视口位置；BattleHUD 用该位置驱动 `UWacomBattleCameraLookComponent` 的临时 cursor-look override，让拖拽时镜头仍能轻微偏转。拖拽过程中 World target 优先由 `AWacomPlayerController::TryProbeBattleSceneInteractionTargetAtWidgetPosition()` 按拖拽指针反投射构建 handle，不调用 click router；普通点击路由继续使用 under-cursor 路径。V0-AE 后 preview 和 release 都由 `BattleHUD::ResolveFirstPersonCardDropIntent()` 统一解析：无目标卡 armed 为 `PlayCardNoTarget`，合法 world enemy part 为 `PlayCardWorldTarget`，合法 hand-card target 为 `PlayCardCardTarget`，不支持的 Card target 为 `ProbeCardTarget`，Zone、空处、同源卡、非法 world、不可打卡或 UI blocked 为 `Reject`。UI 手势层只提交 `TargetCardInstanceId`，不直接判断加费、减费、弃置或消耗；V0-AH/V0-AI 后普通手牌 / 左右手锚点 / keyword 资格来自 `UCardDefinition::HandCardTargetFilter`，并由 `UBattleSession::ValidateTargetWithCard()` 和 `PlayCardResolver` 共同校验。V0-AF 后 Battle 规则层会发 `CardDiscarded / CardExhausted` 作为“某张手牌离开手牌区”的具体事件；first-person transition cache 会把这两类事件映射为现有 `Discarded` 离场表现，手牌上限路径同时保留 `HandLimitDiscarded` 兼容事件但不会生成重复离场 hint。V0-AB 的箭头颜色、commit-ready、CardProbe / valid card target 和 world preview 都读取该 resolver 结果；场景 world preview 只发给命中的 `UWacomBattleEnemyPartWorldTargetBridgeComponent`，`EnemyInfoBar` 不作为拖拽 UI target。V0-AG/V0-AI 后，`TargetMode=HandCard` 源卡进入 `AimingTargetedCard` 时会对整副 first-person hand 做 resolver validation 扫描：被 filter / resolver 允许的卡目标显示确认色和轻量放大，被拒绝的卡目标显示低透明拒绝 tint；当前指向的卡会覆盖基础 affordance。非 HandCard 源卡不做全手牌扫描，当前命中的卡仍只显示 probe。源卡本身获得 selected-source 姿态，复用 pending 的上浮 / 放大 / 层级 / 归正参数，但 HUD 不进入旧 `TargetSelect`。探索期 Run first-person source 当前不启用这套 click / hold / drag 手势；后续 Run drag/drop resolver 接入前，它只是跟随 anchor 的可读卡牌展示层。

V0-T 后，`UWacomFirstPersonCardLayoutPreset` 是 first-person hand 表现调参的可复用 DataAsset，位于 `WacomApp`，不属于 `WacomData` 规则数据。`FirstPersonCardAnchorComponent.bUseFirstPersonCardLayoutPreset` 默认关闭；开启且 `FirstPersonCardLayoutPreset` 有效时，Anchor 会运行时解析出一份配置供 authored layout、projection/clamp、scale/fan/edge、anchor smoothing、slot motion、transition origin、event transitions、hover/pending/disabled 和 interaction feedback 使用。关闭 preset 或 preset 为空时，继续使用组件实例上的原有参数。Preset 不把值写回组件 UPROPERTY，也不覆盖 `FirstPersonCardViewClass`、静态预览卡牌、debug 开关、viewport ZOrder 或 `BattleHUD::BattleHandPresentationMode`。

Render Quality V0-A 的目标是处理 `WBP_CardView` 作为 HUD/UMG first-person 卡牌被整体旋转时出现的边缘锯齿、细线断裂和动态抖动。问题来自 UMG 整卡屏幕空间旋转采样，不是旧 3D Presenter 的 RenderTarget 或场景光照问题。V0-K 曾尝试让 UMG 卡牌角度和缩放来自虚拟 3D 卡牌平面的 projected basis，但动态 projected scale 在 PIE 中带来像素拉伸和新的卡面质量问题，已回退，不作为当前主线。当前 first-person layer 使用 `BodyLocked + Authored2D + SoftClampToViewport + AnchorScreenSmoothing + SlotMotion`：空间感来自整副手牌中心 anchor 的当前相机投影，卡牌排布来自美术可控 2D solver，卡面 scale 保持稳定，只来自 `StaticCardRenderScale` 和 hover / pending / hand anchor 状态倍率；手牌中心允许部分离开屏幕，不再被强制硬锁在视口内，并且 Run Tunnel 移动中的中心点高频抖动会被轻量平滑。单卡 slot motion 只柔化 snapshot / hover / pending / 重排变化，不参与抗锯齿采样，也不替代 anchor center smoothing；V0-P 的 motion diagnostics 只观察和修复 slot widget 生命周期，不改变视觉目标。最终 `CanvasSlot` 位置经过默认开启的 pixel snapping；slot render angle 仍支持 clamp，但不把“降低旋转角”作为当前主线目标。Debug view 会记录 projection mode、layout mode、viewport clamp mode、是否 body locked layout、是否 current camera projection、look offset 是否参与布局、raw screen position、unclamped/widget/snapped widget position、viewport scale、offscreen distance、anchor widget position、authored layout offset、normalized hand offset、unsmoothed / smoothed anchor widget position、smoothing distance，以及 layer motion 的 active/outgoing/root child/ticking slot 统计，便于 PIE 对比不同窗口尺寸下的布局稳定性和反复获得卡牌时的 widget 生命周期稳定性。项目已经为第一人称层预留 `FirstPersonCardViewClass`，正式验证推荐绑定 `/Game/Wacom/UI/Card/WBP_FirstPersonCardView`，让通用 `WBP_CardView` 继续服务普通 HUD / 背包 / 详情场景；专用 WBP 可以承载 RetainerBox、透明留白和内部缩放等第一人称旋转采样优化。

`WBP_FirstPersonCardView` 的父类仍是 `UWacomCardView`。它是第一人称主手牌的卡面皮肤，不是新的交互控件：不要创建 `UCardWidget`，不要在 WBP 图里提交命令，也不要绕过 `UWacomCardPresentationBuilder`。推荐保持 296 x 420 基础尺寸，在 RetainerBox 内部将实际内容轻微缩放到约 0.99，并为贴图和边框预留透明 Alpha 留白。first-person layer 的 slot 仍负责 hover、pending、disabled opacity、ZOrder 和点击意图；WBP 只负责卡面显示质量。

详细设计见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

### 3D hand prototype

`CardActor + WidgetComponent` 的 3D 手牌目前只是原型入口，默认关闭。开启开关只用于验证 HD-2D 场景中的空间手牌可行性，不替换现有 2D `UHandPanel`，也不改变 BattleHUD 的命令出口。

原型边界：

- `BattleHUD` 仍是战斗 UI 命令出口；3D 手牌只把点击、悬停等玩家意图回传给 `BattleHUD`，不直接调 `BattleSession->SubmitCommand()`。
- 3D Presenter / CardActor 只读消费 `FBattleSnapshot` / `FHandCardSnapshot`，用 `InstanceId` 维护视觉对象身份；不能回写战斗状态。
- `WidgetComponent` 内继续承载 `UCardWidget` / 卡面展示协议，仅作为过渡实验，避免短期原型重复解释卡牌数据。
- 3D 手牌不替代当前 `UHandPanel` 和 2D hover 详情；开启原型时两者可以并存，便于对照和回退。
- 本原型不继续作为正式主手牌路线扩展。RenderTarget 裁剪、世界渲染影响、UI 材质动画刷新和大手牌可读性等问题，应通过 first-person card layer 方向解决，而不是继续给 `AWacomBattle3DHandPresenter` 加补丁。

战斗 WBP 制作细节见 `UI_Battle_WBP_Binding.md`。扇形手牌、拖拽出牌、3D 目标选择等属于后续表现项。

---

## §9 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口：

- 从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData`、`FWacomCardViewEffectBadge`。
- 负责中文词条、费用、价值、身材/容量、效果徽章、被动 fallback 文本等展示推导。
- 只服务 UI 表现，不参与战斗或 Run 规则结算。

当前复用方：

- 背包卡牌和拖拽预览。
- 背包、战斗手牌与第一人称战斗手牌 hover 详情面板。
- 战斗手牌卡面，额外用 `FHandCardSnapshot.RuntimeCost` 覆盖费用显示。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌展示数据统一由 `UWacomCardPresentationBuilder` 生成；不要在 CardView 上恢复 Definition 级 legacy static API。

`UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`。当前详情数据来自 DisplayName、Description 和 Passives；任务、变化等字段等待卡牌数据结构正式扩展。战斗 HUD 内部已把详情显示入口从 `UCardWidget` 几何中解耦；旧手牌详情使用 HUD 内部 `CardDetailLayer`，first-person slot 详情使用独立 viewport popup host，但两者共用同一套详情数据构建、边界 clamp 和 V0-V 读牌 motion 逻辑。

---

## §10 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约：

| 文档                           | 职责                                                                                  |
| ---------------------------- | ----------------------------------------------------------------------------------- |
| `UI_Backpack_WBP_Binding.md` | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| `UI_Battle_WBP_Binding.md`   | 战斗手牌、HandPanel、BattleHUD、BattleEventLog、Enemy fallback UI 的父类、路径、绑定槽位和 PIE 检查       |

规则真相不写在 Binding 文档里。若 WBP 制作时需要知道为什么这么做，应回到 `WacomUI.md`、`WacomRun.md` 或 `WacomBattle.md`。
