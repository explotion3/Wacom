---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-05-28
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

| 领域 | 数据来源 | 命令出口 |
|---|---|---|
| ExplorationHUD | `UWacomRunViewModelProvider -> UWacomRunViewModel` | 无；只读显示探索状态和交互提示 |
| Run Screen / 菜单类 Widget | Run Snapshot / ViewModel / Presentation ViewData | `URunSession` 写 API，通常经 PlayerController / Screen 调用 |
| 背包 | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图，私有 `FWacomBackpackCommandFlow` 编排 `MoveInstance / DeleteCardForGoldByInstance / SetSpecialZoneCardBattleEnabled`、Toast 和 Confirm |
| 商店 | `URunSession::BuildCurrentShopSnapshot()` | `UWacomShopScreen` 接收 UI 意图，私有 `FWacomShopScreenFlow` 编排 `PurchaseShopOffer / EndShopVisit` 和 Toast |
| 探索事件 | `URunSession::BuildCurrentRunEventSnapshot()` | `UWacomRunEventScreen` 接收 UI 意图，私有 `FWacomRunEventScreenFlow` 编排 `ChooseRunEventOptionWithResult / EndRunEvent` 和 Toast |
| 战斗 | `FBattleSnapshot`、`FBattleEvent`、BattleSession ViewData | `UBattleHUD` 接收玩家意图，私有 BattleHUD flow helper 编排命令提交、目标选择、事件日志和击倒弹窗 |

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
- RunEvent 选项结果/不可用原因：由 `UWacomRunEventPresentationBuilder` 转成中文 Toast。

### 战斗事件表现队列、Toast 与日志

`UWacomBattleEventPresentationBuilder` 把 `FBattleEvent` 转成 `FBattleEventPresentationView`。ViewData 包含 `MessageText / bShouldDisplay / VisualTone / IconKey`，但当前 Toast 和日志主要消费文字。

- `FWacomBattleEventPresentationQueue` 负责把 `FBattleEvent` 按顺序调度为 TargetCue、Toast、短暂停顿和击倒弹窗 step；队列播放期间 `BattleHUD` 进入 `Resolving`，阻止继续提交战斗命令。
- TargetCue 是 `WacomApp/Private` 内的轻量表现信号。当前 `DamageDealt / EnemyPartHpEmptied` 会经 `BattleHUD` 私有目标注册表按敌方部位实例 ID 分发；2D `EnemyInfoBar` 刷新时把当前 `EnemyPartWidget` 注册进去。场景对象可通过 `UWacomBattlePresentationTargetComponent` 手动注册 `PartInstanceId`，也可在 BattleHUD 开启 `bEnableSceneEnemyTargetBindingPrototype` 后填写稳定 `UEnemyPartDefinition::PartId`，由 snapshot 刷新和 UIState 刷新后自动绑定运行时实例 ID。组件收到命中 / 破坏 cue 时会对指定或自动发现的 `UPrimitiveComponent` 播放 V0 scale pulse，并在 timer 结束后恢复；进入 `TargetSelect` 且该部位可选时，同一 primitive 会播放持续的 V0 scale 呼吸提示，退出选择或不可选时恢复。V0-C 下战斗场景点击主路径是 `AWacomPlayerController::InputKey()` 在左键 Release 且 HUD 处于 `TargetSelect` 时执行 `GetHitResultUnderCursor(ECC_Visibility)`，从命中 Actor / Component 查找 `UWacomBattlePresentationTargetComponent`，再调用 `RequestSceneTargetClick()` 转发到 `BattleHUD->OnEnemyPartClickedByUser()`；如果 Release 先落到 BattleHUD 根层，`BattleHUD::NativeOnMouseButtonUp()` 会调用同一个 PlayerController router 作为兜底。V0-B 的 `ClickTargetComponent` / `Primitive.OnClicked` 绑定保留为兼容辅助路径。真实 hover、鼠标命中提示、材质描边和正式敌人 Actor 留后续。队列不依赖具体 Widget 或未来场景 Actor。
- 目标注册表 V0 是单 handler 覆盖策略：同一 `PartInstanceId` 后注册者替换旧注册者；未来如果需要 2D Widget 与 3D Actor 同时表现，再升级为 priority 或 multicast。
- 开启场景绑定原型时，`BattleHUD` 会在 `EnemyInfoBar` 因目标选择状态刷新并重新注册 2D 部位后，再同步一次场景目标。因此 V0 语义是：同一部位上，场景 target 保持为当前表现 / 点击 provider。
- PIE 验证时，`VisualTargetComponent` 和 `ClickTargetComponent` 可以保持 `None`；组件会自动查找 Owner 上第一个 `UPrimitiveComponent`。如果在 Details 面板误创建了 `StaticMeshComponent_0` 这类嵌在组件下的对象，应清空引用或删除临时组件，避免 fallback 命中错误对象。推荐原型结构是 `Actor -> StaticMeshComponent(Cube) + UWacomBattlePresentationTargetComponent`。
- 当前 V0-C 的 PIE 场景点击由 `AWacomPlayerController` 显式 cursor trace router 处理，跟随 `BattleHUD::bEnableSceneEnemyTargetBindingPrototype` 启用；路由只在左键 Release 且 HUD 已进入 `TargetSelect` 时消费。BattleHUD 根层 `MouseButtonUp` 兜底也复用同一路由，覆盖单击有效阶段落在 MouseUp 的 PIE 情况。`Primitive.OnClicked` 仍可用于旧验证，但不再是推荐主路径；选择需要敌方部位目标的卡牌进入 `TargetSelect` 后，单击匹配的场景 primitive 应直接完成目标选择，不再需要关卡蓝图临时 trace。
- 场景目标排障优先看组件的 `GetBattlePresentationTargetDebugSummary()` 或在 Details/蓝图中调用 `LogBattlePresentationTargetDebugSummary()`：确认 `PartId`、运行时 `PartInstanceId`、`Registered=true`、实际解析到的 visual/click primitive、`BlocksVisibility=true`、`TargetAffordance=true`、`LastAutoBind=MatchedPartId`、`LastClick=Forwarded`。组件上的 `bLogDebugStateChanges` 和 PlayerController 上的 `bLogBattleSceneTargetClickRouting` 默认关闭；临时打开后只输出注册、自动绑定、点击和 cue 状态，不做屏幕常驻调试。
- `UEventToast` 负责单条战斗 Toast 的显示和倒计时，不负责文案判断或事件排序。
- `UBattleEventLogPanel` 是 `BattleHUD` 内部半屏日志抽屉，不通过 CommonUI Layer push/pop。
- `BattleHUD` 保存本场 `BattleEventLogHistory`，切换 Session 时清空日志和表现队列。
- `KnockdownChoiceRequested` 不再当帧直接 Push modal；它由表现队列延后到前序事件播放后再打开。
- `HandZoneChanged` 等 `bShouldDisplay=false` 的事件不会进入 Toast 或日志。

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
- 可用选项由 `FWacomRunEventScreenFlow` 调 `ChooseRunEventOptionWithResult(ChoiceId)`，Run 层执行条件、效果、跳转和完成标记。
- `FWacomRunEventScreenFlow` 使用 `UWacomRunEventPresentationBuilder` 把 `FRunEventChoiceResult` 转成 AppToast；不可用选项也由该 flow 发阻塞原因 Toast。
- `RemoveCard` 结果显示“交出卡牌：{CardName}”；`MarkEventCompleted` 默认不弹 Toast。
- 关闭事件界面时由 `FWacomRunEventScreenFlow` 调 `EndRunEvent()`；关闭型选项先由 Run 层清 active event，再 Deactivate。
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
- 当前 2D `EnemyInfoBar / EnemyPartWidget` 消费该 ViewData；`EnemyInfoBar` 同时把部位 Widget 注册到 `BattleHUD` 私有目标注册表，用于接收 TargetCue 并播放轻量命中/破坏反馈。场景侧原型可用 `UWacomBattlePresentationTargetComponent` 注册同一 `PartInstanceId`；开启 BattleHUD 的场景目标绑定原型后，组件可填写稳定 `PartId` 并在 snapshot 刷新、目标选择 UIState 刷新后自动绑定当前运行时实例 ID。组件的 V0 视觉反馈包含命中/破坏短暂 scale pulse，以及 TargetSelect 可选目标的持续 scale 呼吸提示；不改材质或规则状态。V0-C 中左键 Release 由 PlayerController 的 Visibility cursor trace router 转发为 HUD 目标选择意图，BattleHUD 根层 `MouseButtonUp` 兜底复用同一路由，V0-B 的 `Primitive.OnClicked` 兼容路径仍保留。V0 下场景 target 会替换 2D target，而不是并行同播。

手牌与详情：

- `UHandPanel` 把 `FHandQueueSnapshot` 转成 `FHandCardVisualEntry[]`，默认统一水平手牌带。
- `UCardWidget` 是战斗手牌交互外壳；`UWacomCardView` 只负责卡面显示。
- Hover 反馈移动 `HoverVisualRoot` 视觉层，不移动根命中区域，不改变布局占位。
- Hover 详情由 `UCardWidget` 上报、`UHandPanel` 转发、`BattleHUD` 管理；详情默认显示在卡牌左侧，空间不足时放右侧。
- 进入目标选择、提交命令、刷新 Snapshot、切换 Session 或战斗结束时隐藏详情。

### First-person card layer direction

Wacom 的正式卡牌表现方向不是把 `UCardWidget` 长期塞进 `WidgetComponent / RenderTarget`。主手牌应保持 HUD / UMG 渲染，以保证文本清晰、材质动画、hover 详情、拖拽和响应式布局稳定；但布局不再是传统固定 HUD，而是由 first-person card anchor 计算虚拟手牌平面，再投影为 UMG render transform。这样卡牌会像跟随玩家身体 / tunnel 前进的第一人称手牌，而不是死贴镜头的屏幕按钮。

当前已实现 `UWacomFirstPersonCardAnchorComponent`、默认关闭的 HUD debug projection、默认关闭的静态卡牌层、正式的战斗手牌呈现模式、first-person battle hand hover/click、first-person hover detail provider、first-person hover 详情跟随 / ZOrder 修正、Render Quality V0-A、默认 `BodyLocked` 的身体锁定锚点投影、默认 `Authored2D` 的美术可控手牌布局、默认 `SoftClampToViewport` 的离屏锚点柔性限制、`Authored2D` hand center screen smoothing、默认开启的 card slot motion polish、slot motion robustness / perf diagnostics、event-aware card transitions、TargetSelect pending focus polish，以及 playable / hover / press feedback polish。静态层使用 `UWacomCardView` 或 `FirstPersonCardViewClass` 指定的专用卡面 WBP 渲染配置的测试卡牌或 placeholder 卡牌；战斗 hand adapter 由 `BattleHUD::BattleHandPresentationMode` 控制，非 `LegacyHandPanel` 时会把 `FBattleSnapshot.Hand.Cards` 按顺序转成 `FWacomFirstPersonCardLayerEntry`，覆盖同一个 first-person card layer 显示。Entry 保留 `InstanceId / Zone / HandAnchor / Playable / PendingTargeting`，用于轻量视觉状态和后续交互身份。

第一人称战斗手牌层不创建 `UCardWidget`，不做拖拽或直接出牌命令，不删除 `UHandPanel`，也不改变战斗命令出口。`BattleHUD::BattleHandPresentationMode` 是唯一配置入口：`LegacyHandPanel` 只使用旧手牌；默认 `FirstPersonHandWithLegacyFallback` 显示并启用 first-person hand，同时保留旧 `UHandPanel` 可见作为 fallback 和对照；`FirstPersonHandOnly` 在 first-person runtime hand 有效时折叠旧手牌，缺少 anchor、战斗结束、Session 切换或清理 runtime hand 时恢复旧手牌原始可见性。`FirstPersonCardAnchorComponent.ProjectionMode` 默认 `BodyLocked`，战斗中使用 Battle base rotation、Run Tunnel 中使用 spline base 锁定手牌中心锚点，不让 cursor look 重新计算 anchor；投影仍使用当前真实相机，因此镜头偏转时会有第一人称空间变化。`FirstPersonCardAnchorComponent.CardLayoutMode` 默认 `Authored2D`：只投影整副手牌中心点，每张卡牌的位置、下坠、扇形角度和层级由 2D 参数计算；`ViewportClampMode` 默认 `SoftClampToViewport`，投影成功的手牌中心可以离开视口安全区一段距离，超过 `SoftClampOffscreenAllowancePixels` 后再按 `SoftClampBlendRangePixels` 柔性拉回。`bEnableAnchorScreenSmoothing` 默认开启，只在 `Authored2D` 下平滑整副手牌中心，不平滑单张卡牌；它保留 Run Tunnel 前进下降、后退上升的空间趋势，但减少移动时中心点高频上下抖动。`bEnableCardSlotMotion` 默认开启，是第二层单卡视觉过渡：`UWacomFirstPersonCardLayerWidget` 用 `CardInstanceId` 或 `StaticIndex:{Index}` 复用 slot widget，`UWacomFirstPersonCardLayerSlotWidget` 再把视觉位置、角度、缩放和透明度追向 anchor 计算出的目标 slot；新卡按 `CardSlotEnterOffsetPixels / CardSlotEnterOpacity` 淡入，移除或投影失败的可见卡按 `CardSlotExitOffsetPixels / CardSlotExitDuration` 短暂淡出，目标大跳变超过 `CardSlotMotionResetDistancePixels` 时直接贴合。`bEnableEventAwareCardTransitions` 默认开启时，BattleEvent 只作为表现 hint 改变入场 / 离场方向：抽牌从下方进入，战斗中获得的卡从上方 / 战斗空间方向进入，打出的卡向上离开，手牌上限弃置的卡向下离开；snapshot diff 仍是手牌变化真相，事件缺失时回退通用 enter / exit。TargetSelect 中 pending 卡是手牌焦点：它上浮、放大、提高层级，并可按 `PendingTargetingAngleBlend` 向 0 度轻微归正；同一副手牌存在 pending 卡时，其他卡只轻微降透明，不下沉、不缩小。Pending 卡仍可点击取消 TargetSelect，但 hover 不再额外叠加 lift / scale / ZOrder，避免选中态抖动。`bEnableCardInteractionFeedback` 默认开启时，slot widget 用 C++ feedback overlay 表现可打 hover、pressed、confirm 和 deny 反馈；可打、非 pending 卡才获得 hover lift / scale / ZOrder，不可打卡仍可 hover 查看 Idle 详情但不会表现成可打姿态，点击不可打卡只播放 deny tint / 横向 shake，不广播 click intent。V0-P 后 layer 会维护 `FWacomFirstPersonCardLayerMotionDebugView`，统计 active / outgoing / RootCanvas child / ticking slot 和本次刷新创建、复用、移除、异常修复数量；刷新后会修复 active/outgoing 重叠、RootCanvas 未追踪 slot child、重复 key 和 outgoing 超限。同一 `CardInstanceId` 如果在 outgoing 淡出期间重新进入手牌，会回收原 outgoing widget 作为 active，避免形成幽灵 widget。`bLogCardLayerMotionDiagnostics` 默认关闭，只有手动排查时开启，且只在检测到不变量修复时输出简短 summary。`AnchorScreenSmoothingSpeed` 控制整副手牌中心跟手程度，`AnchorScreenSmoothingResetDistancePixels` 控制中心大跳变时直接贴合新位置；`CardSlotMotionSpeed / CardSlotOpacitySpeed` 控制单卡重排、hover、pending、抽牌、获得卡、出牌和弃置刷新时的过渡。`HardClampToViewport` 可复现旧的永远屏内行为，`AllowOffscreen` 可用于验证最接近 3D 空间物体的离屏表现。`FirstPersonHandOnly` 下手牌离屏不会自动恢复旧手牌入口，玩家需要转回视角或切换 presentation mode。`LegacyProjectedFan2D` 保留旧的每卡 3D 槽位投影表现，用于 PIE 对照。`LegacyWorldProjected` 保留旧的 LookInfluence 影响 anchor + 当前相机投影路径，用于调试对照。Hover 会改变 first-person layer 自身的 UMG transform / ZOrder，并在 Idle 状态下通过 `BattleHUD` 显示现有 `UWacomCardDetailPanel`；旧手牌详情使用 BattleHUD 内部 `CardDetailLayer`，first-person 详情使用独立 viewport popup host，默认层级 9999，高于 first-person card layer。详情数据来自 BattleHUD 最近一次 `FBattleSnapshot.Hand`，定位来自 first-person slot 的视觉屏幕锚点，并在 hovered slot 布局位置变化时实时跟随。左键释放可用卡只把有效 `CardInstanceId` 转发给 `BattleHUD->OnCardClickedByUser()`，后续合法性、TargetSelect 和命令提交仍走 BattleHUD 现有 flow。Runtime battle hand 数据源优先级高于静态预览，且空手牌也显示为空，不回退 placeholder。当前视觉状态只使用 UMG transform、opacity、C++ feedback overlay 和已有 disabled overlay，不可用卡降低 layer 透明度，手牌锚点卡轻微缩放。

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

`UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`。当前详情数据来自 DisplayName、Description 和 Passives；任务、变化等字段等待卡牌数据结构正式扩展。战斗 HUD 内部已把详情显示入口从 `UCardWidget` 几何中解耦；旧手牌详情使用 HUD 内部 `CardDetailLayer`，first-person slot 详情使用独立 viewport popup host，但两者共用同一套详情数据构建和边界 clamp 逻辑。

---

## §10 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约：

| 文档                           | 职责                                                                                  |
| ---------------------------- | ----------------------------------------------------------------------------------- |
| `UI_Backpack_WBP_Binding.md` | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| `UI_Battle_WBP_Binding.md`   | 战斗手牌、HandPanel、BattleHUD、BattleEventLog、Enemy fallback UI 的父类、路径、绑定槽位和 PIE 检查       |

规则真相不写在 Binding 文档里。若 WBP 制作时需要知道为什么这么做，应回到 `WacomUI.md`、`WacomRun.md` 或 `WacomBattle.md`。
