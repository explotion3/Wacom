---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-05-22
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
| 背包 | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 调 `MoveInstance / DeleteCardForGold / SetSpecialZoneCardBattleEnabled` |
| 商店 | `URunSession::BuildCurrentShopSnapshot()` | `UWacomShopScreen` 调 `PurchaseShopOffer`，关闭时调 `EndShopVisit` |
| 探索事件 | `URunSession::BuildCurrentRunEventSnapshot()` | `UWacomRunEventScreen` 调 `ChooseRunEventOptionWithResult` |
| 战斗 | `FBattleSnapshot`、`FBattleEvent`、BattleSession ViewData | `UBattleHUD` 统一调 `Session->SubmitCommand` |

Widget 可以有 C++ fallback 布局，但 C++ 的职责是协议、生命周期和兜底显示；正式视觉由 WBP 承接。

---

## §2 CommonUI 层级

`UWacomGameUIManagerSubsystem` 持有 `UWacomPrimaryGameLayout`。PrimaryLayout 跟随当前 PlayerController 创建，切关卡时 TearDown 并重建。

默认布局资产路径：

- `/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C`
- 父类：`UWacomPrimaryGameLayout`

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

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入；战斗 HUD 和探索 HUD 由各自基类控制输入口径。

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

默认 Toast WBP 路径：

- `/Game/Wacom/UI/Foundation/WBP_AppToastWidget.WBP_AppToastWidget_C`
- 父类：`UWacomAppToastWidget`
- 可选绑定：`Container : VerticalBox`

当前生命周期：

- 探索局开始时 PlayerController 会预热；首次 Toast 也会懒加载兜底。
- 无消息时 Widget `Collapsed`；入队后 `HitTestInvisible`；消息播完后只隐藏，不销毁。
- Widget `SetIsFocusable(false)`，`GetDesiredInputConfig()` 返回空，不改变探索或菜单输入。
- `FWacomAppToastView` 保留 `MessageText / Tone / IconKey / LifetimeOverride`；C++ fallback 第一版只显示文字。

当前接入：

- 商店购买成功：`获得卡牌：{CardName}`。
- 商店购买失败：按 Offer ViewData 的 `DisabledReason` 显示“金币不足 / 该商品已购买 / 商品不可购买”。
- 背包移动成功/失败：成功显示移动到目标区，失败显示通量满、备战满、特殊区满等原因。
- 背包删牌成功/失败：成功显示销毁卡牌和获得金币，失败显示固有卡、最后容量来源卡、未持有卡等原因。
- RunEvent 选项结果/不可用原因：由 `UWacomRunEventPresentationBuilder` 转成中文 Toast。

### 战斗事件 Toast 与日志

`UWacomBattleEventPresentationBuilder` 把 `FBattleEvent` 转成 `FBattleEventPresentationView`。ViewData 包含 `MessageText / bShouldDisplay / VisualTone / IconKey`，但当前 Toast 和日志主要消费文字。

- `UEventToast` 负责战斗内 Toast 队列、显示和倒计时，不负责文案判断。
- `UBattleEventLogPanel` 是 `BattleHUD` 内部半屏日志抽屉，不通过 CommonUI Layer push/pop。
- `BattleHUD` 保存本场 `BattleEventLogHistory`，切换 Session 时清空。
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

`UWacomBackpackScreen` 位于 `GameMenu` 层。它负责 UI 编排、拖拽、详情面板生命周期和命令提交；规则真相仍在 `URunSession`。
C++ fallback 布局由私有 `FBackpackFallbackLayoutBuilder` 搭建，运行时 DropTarget / WrapBox / 详情层由私有 `FBackpackRuntimeZoneBuilder` 创建；WBP 绑定字段和命令入口仍保留在 `UWacomBackpackScreen`。

当前结构：

| 区域 | 当前职责 |
|---|---|
| 删牌区 | `UWacomDeleteZoneDropTarget` 只做预览和转发删牌意图；金币预览读取 `URunSession::GetDeleteGoldRewardForCard()` |
| 备战区 | `UWacomZoneDropTarget` 显示 `BattleDeck` 和已入战 SpecialZone 投影卡，只做预览和转发移动意图 |
| 通量内容区 | 显示物理位于 `Backpack` 的通量内容，A 类容器也作为内容卡显示 |
| 特殊存放区 | 每张 B 主卡对应一个 `UWacomSpecialZoneWidget` |
| 负重区 | 仅 `BurdenCount > 0` 时显示 |

数据和刷新：

- 列表数据来自 `RunSession->BuildBackpackStorageSnapshot()`。
- 顶部金币、容量等标量来自 Run ViewModel。
- 操作成功后不做局部 patch，当前通过 `RebuildAll()` 全量重建。
- `RebuildAll()` 已拆为顶部、备战区、通量区、特殊区和负重区刷新函数。

交互：

- DropTarget 只读 RunSession 做 hover 预览校验；drop 时把 `UWacomCardDragOperation` 转发给 `UWacomBackpackScreen`。
- 普通 drop 由 `UWacomBackpackScreen::HandleZoneDropRequested()` 先调用 `ValidateMoveInstance()`，再调用 `MoveInstance()`，并统一发 AppToast。
- Delete drop 由 `UWacomBackpackScreen::HandleDeleteDropRequested()` 先调用 `ValidateDeleteCardForGold()`；失败直接 AppToast，成功才弹 ConfirmDialog，确认后提交 `DeleteCardForGold()`。
- SpecialZone 内容卡右键入战请求由 `UWacomSpecialZoneWidget` 转发给 `UWacomBackpackScreen`。
- 卡牌 hover 详情由 `UWacomBackpackScreen` 管理，数据来自 `UWacomBackpackScreenPresenter` 和 `UWacomCardPresentationBuilder`。

背包 WBP 制作细节见 `UI_Backpack_WBP_Binding.md`。未来正式 WBP、拖拽 polish、增量刷新等计划见 `Roadmap.md` 和 `TechDebt.md`。

---

## §6 商店 UI

`UWacomShopScreen` 位于 `GameMenu` 层。商店打开不切换 `EGameFlowState`。

当前流程：

- 打开商店前由 PlayerController 调 `RunSession->BeginShopVisit(PersistentId, Offers)`。
- Screen 刷新时读取 `BuildCurrentShopSnapshot()`。
- `UWacomShopPresentationBuilder` 把 `FRunShopOffer + 当前金币` 转成 `FWacomShopOfferPresentationView`。
- Offer 行只渲染 ViewData 并广播购买请求，不直接解析 `CardDefinition` 或判断金币。
- 点击购买调用 `PurchaseShopOffer(OfferId)`，成功后刷新商品列表和金币。
- `NativeOnDeactivated` 中只调用一次 `EndShopVisit()`；本次访问买过任意商品时 Run 层关闭时消耗 1 节点。
- 如果 Push 商店 UI 失败，PlayerController 会调用 `EndShopVisit()` 清理 active shop。

正式商店视觉和 hover 详情属于后续表现项；商品规则、库存和节点消耗见 `WacomRun.md`。

---

## §7 RunEvent UI

`UWacomRunEventScreen` 位于 `GameMenu` 层，用于展示轻量事件图当前 Node。

当前流程：

- 打开前由 PlayerController 调 `RunSession->BeginRunEvent(PersistentId, EventDefinition)`。
- Screen 刷新时读取 `BuildCurrentRunEventSnapshot()`。
- ChoiceButton 显示 Label 和中文禁用原因；不可用选项也允许点击以弹出原因 Toast。
- 可用选项调用 `ChooseRunEventOptionWithResult(ChoiceId)`，Run 层执行条件、效果、跳转和完成标记。
- `UWacomRunEventPresentationBuilder` 把 `FRunEventChoiceResult` 转成 AppToast。
- `RemoveCard` 结果显示“交出卡牌：{CardName}”；`MarkEventCompleted` 默认不弹 Toast。
- 关闭事件界面时调用 `EndRunEvent()`；关闭型选项先由 Run 层清 active event，再 Deactivate。

事件规则、条件、效果和 PersistentId 口径见 `WacomRun.md` / `WacomData.md`。

---

<a id="wacomui-battle-ui"></a>
## §8 战斗 UI

战斗 UI 使用 Snapshot + Controller 推送模型，不走 Run MVVM。

规则合法性和状态变更见 [WacomBattle.md](./WacomBattle.md)，战后结算见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement)，WBP 制作合约见 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

理由：

- `BattleHUD` 战斗开始 Push、结束 Pop，生命周期短。
- 子 Widget 都在同一棵树里，由 `BattleHUD` 统一递归刷新。
- `FBattleSnapshot` 是值类型快照，适合一次性读当前战斗状态。
- 动态手牌和敌方部位列表由 HUD / Panel 直接重建更简单。
- C++ fallback BattleHUD 布局由私有 `FBattleHUDFallbackLayoutBuilder` 搭建；WBP 绑定字段、命令入口和刷新流程仍保留在 `UBattleHUD`。

命令出口：

- 子 Widget 只发委托，不直接调 `Session->SubmitCommand()`。
- `UBattleHUD` 是战斗 UI 命令出口：PlayCard、Wait、EndTurn、KnockdownChoice 经它提交。TargetSelect 是 HUD 管理的 UI 状态，选定目标后再转成 PlayCard command。
- Dialog 点击后回到 `BattleHUD->OnKnockdownChoiceSelected()`，Dialog 不直接改战斗状态。

目标选择：

- 点击需要敌方部位目标的手牌时，`BattleHUD` 进入 `TargetSelect` 并记录 `PendingTargetingCardId`。
- 再次点击同一张牌会取消目标选择。
- `BattleHUD::BuildTargetSelectionView()` 输出 UI-only 只读 ViewData，当前只区分是否选目标和部位是否破坏；最终合法性仍由 BattleSession / PlayCardResolver 校验。
- 当前 2D `EnemyInfoBar / EnemyPartWidget` 消费该 ViewData；未来 HD-2D / PaperZD 部位 Actor 也应消费同一份视图。

手牌与详情：

- `UHandPanel` 把 `FHandQueueSnapshot` 转成 `FHandCardVisualEntry[]`，默认统一水平手牌带。
- `UCardWidget` 是战斗手牌交互外壳；`UWacomCardView` 只负责卡面显示。
- Hover 反馈移动 `HoverVisualRoot` 视觉层，不移动根命中区域，不改变布局占位。
- Hover 详情由 `UCardWidget` 上报、`UHandPanel` 转发、`BattleHUD` 管理；详情默认显示在卡牌左侧，空间不足时放右侧。
- 进入目标选择、提交命令、刷新 Snapshot、切换 Session 或战斗结束时隐藏详情。

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

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。旧静态入口 `BuildFromCardDefinition / BuildDetailFromCardDefinition` 仅作为兼容转发，新代码应直接用 PresentationBuilder；移除旧入口的清理项见 [TechDebt: UI 层技术债](./TechDebt.md#techdebt-ui)。

`UWacomCardDetailPanel` 只显示 `FWacomCardDetailViewData`。当前详情数据来自 DisplayName、Description 和 Passives；任务、变化等字段等待卡牌数据结构正式扩展。

---

## §10 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约：

| 文档                           | 职责                                                                                  |
| ---------------------------- | ----------------------------------------------------------------------------------- |
| `UI_Backpack_WBP_Binding.md` | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| `UI_Battle_WBP_Binding.md`   | 战斗手牌、HandPanel、BattleHUD、BattleEventLog、Enemy fallback UI 的父类、路径、绑定槽位和 PIE 检查       |

规则真相不写在 Binding 文档里。若 WBP 制作时需要知道为什么这么做，应回到 `WacomUI.md`、`WacomRun.md` 或 `WacomBattle.md`。
