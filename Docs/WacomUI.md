---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-05-24
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

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入；战斗 HUD 和探索 HUD 由各自基类控制输入口径。`UWacomMenuWidgetBase` 负责 Menu 模式下的返回键口径：ESC 和 Gamepad FaceButton Right 触发 Back 请求，默认广播 `OnBackRequestedNative` 后 `DeactivateWidget()`；子类只在语义不同（例如 ConfirmDialog 把 Back 当 Cancel）时覆盖。

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
- TargetCue 是 `WacomApp/Private` 内的轻量表现信号。当前 `DamageDealt / EnemyPartHpEmptied` 会经 `BattleHUD` 分发到 `EnemyInfoBar / EnemyPartWidget` 做命中或破坏闪烁；队列不依赖具体 Widget 或未来场景 Actor。
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
- 当前 2D `EnemyInfoBar / EnemyPartWidget` 消费该 ViewData，并接收 `BattleHUD` 分发的 TargetCue 做轻量命中/破坏反馈；未来 HD-2D / PaperZD 部位 Actor 也应接入同一 HUD 分发点。

手牌与详情：

- `UHandPanel` 把 `FHandQueueSnapshot` 转成 `FHandCardVisualEntry[]`，默认统一水平手牌带。
- `UCardWidget` 是战斗手牌交互外壳；`UWacomCardView` 只负责卡面显示。
- Hover 反馈移动 `HoverVisualRoot` 视觉层，不移动根命中区域，不改变布局占位。
- Hover 详情由 `UCardWidget` 上报、`UHandPanel` 转发、`BattleHUD` 管理；详情默认显示在卡牌左侧，空间不足时放右侧。
- 进入目标选择、提交命令、刷新 Snapshot、切换 Session 或战斗结束时隐藏详情。

### 3D hand prototype

`CardActor + WidgetComponent` 的 3D 手牌目前只是原型入口，默认关闭。开启开关只用于验证 HD-2D 场景中的空间手牌可行性，不替换现有 2D `UHandPanel`，也不改变 BattleHUD 的命令出口。

原型边界：

- `BattleHUD` 仍是战斗 UI 命令出口；3D 手牌只把点击、悬停等玩家意图回传给 `BattleHUD`，不直接调 `BattleSession->SubmitCommand()`。
- 3D Presenter / CardActor 只读消费 `FBattleSnapshot` / `FHandCardSnapshot`，用 `InstanceId` 维护视觉对象身份；不能回写战斗状态。
- `WidgetComponent` 内继续承载 `UCardWidget` / 卡面展示协议，避免 2D 与 3D 卡牌展示各自解释卡牌数据。
- 3D 手牌不替代当前 `UHandPanel` 和 2D hover 详情；开启原型时两者可以并存，便于对照和回退。
- 本原型不做 RenderTarget 池、不做 3D hover detail、不做 3D enemy targeting；这些属于后续正式表现项。

战斗 WBP 制作细节见 `UI_Battle_WBP_Binding.md`。扇形手牌、拖拽出牌、3D 目标选择等属于后续表现项。

---

## §9 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口：

- 从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData`、`FWacomCardViewEffectBadge`。
- 负责中文词条、费用、价值、身材/容量、效果徽章、被动 fallback 文本等展示推导。
- 只服务 UI 表现，不参与战斗或 Run 规则结算。

当前复用方：

- 背包卡牌和拖拽预览。
- 背包与战斗手牌详情面板。
- 战斗手牌卡面，额外用 `FHandCardSnapshot.RuntimeCost` 覆盖费用显示。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌展示数据统一由 `UWacomCardPresentationBuilder` 生成；不要在 CardView 上恢复 Definition 级 legacy static API。

`UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`。当前详情数据来自 DisplayName、Description 和 Passives；任务、变化等字段等待卡牌数据结构正式扩展。

---

## §10 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约：

| 文档                           | 职责                                                                                  |
| ---------------------------- | ----------------------------------------------------------------------------------- |
| `UI_Backpack_WBP_Binding.md` | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| `UI_Battle_WBP_Binding.md`   | 战斗手牌、HandPanel、BattleHUD、BattleEventLog、Enemy fallback UI 的父类、路径、绑定槽位和 PIE 检查       |

规则真相不写在 Binding 文档里。若 WBP 制作时需要知道为什么这么做，应回到 `WacomUI.md`、`WacomRun.md` 或 `WacomBattle.md`。
