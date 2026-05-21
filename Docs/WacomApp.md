# WacomApp 模块文档

> 本文是 WacomApp 模块的设计 + 实现文档。

---

## §1 模块职责

WacomApp 是**表现层和游戏主模块**（PRIMARY_GAME_MODULE）。

**负责**：
- GameMode / PlayerController / PlayerCharacter
- UI 架构（CommonUI 框架、Widget 体系、菜单系统）
- Enhanced Input 管理
- 探索场景交互
- 战斗 UI 路由
- 战斗触发 Actor

**不负责**：
- 修改战斗状态真相（只能通过 `UBattleSession` 提交命令、读取快照和事件）
- 单场战斗内规则细节
- 存档逻辑（属于 WacomRun）
- 静态数据定义（属于 WacomData）

---

## §2 关卡与 GameMode

### L_MainMenu + AWacomMenuGameMode

- 主菜单关卡
- 使用 `AWacomMenuGameMode`（不 Spawn Pawn，不做探索）
- 提供 New Game / Continue / Quit 入口

### L_Exploration + AWacomGameMode

- 探索关卡
- 使用 `AWacomGameMode`
- 管理 `EGameFlowState`（Exploration / Battle）
- 提供 `EnterBattle` / `ExitBattle` 接口
- DefaultPawnClass = `AWacomPlayerCharacter`

### L_TestBattle（独立测试）

- 纯战斗测试关卡
- 不走存档逻辑
- 保留作为开发验证用

### EGameFlowState

```cpp
enum class EGameFlowState : uint8
{
    Exploration,   // 玩家自由移动，可触发事件
    Battle,        // 战斗进行中，移动禁用，战斗 UI 激活
};
```

---

## §3 PlayerController

`AWacomPlayerController` 职责：

- 持有 `URunSession*`（BeginPlay 时创建）
- 管理 IMC Push/Pop（IMC_Exploration ↔ IMC_Battle）
- 接收 GameMode 的状态切换通知
- 战斗 IA 绑定（1-7 打牌、W 等待、E 结束回合等）
- ESC 路由（探索时打开暂停菜单，战斗时不响应或打开战斗暂停）

---

## §4 PlayerCharacter

`AWacomPlayerCharacter : ACharacter`

- 第一人称 Pawn
- `UCameraComponent`（第一人称摄像机）
- `UCharacterMovementComponent`（默认）
- 输入：IMC_Exploration 时响应 WASD + 鼠标视角（Move / Look）
- 战斗时：移动禁用（`SetExplorationInputEnabled(false)`），摄像机不动，Pawn 不 UnPossess

战斗时不 UnPossess 的原因：保持 PlayerController 的 InputComponent 活跃，IMC_Battle 的按键仍能通过 Controller 路由到战斗 UI。

---

## §5 世界交互接口

探索期 `E` 交互已从“只认识 BattleTriggerActor”收口为通用接口：

| 类型 | 职责 |
|---|---|
| `IWacomWorldInteractable` | 世界交互对象协议：提示文本、交互位置、可用性、执行交互 |
| `AWacomPlayerController` | 维护 `CandidateInteractables`，按距离选择最近且可交互对象，刷新 ExplorationHUD Toast，按 E 调用接口 |
| `UWacomExplorationHUD` | 只显示调用方传入的交互提示文本，不解析交互类型 |

交互对象进入范围时调用 `RegisterCandidateInteractable`，离开范围或销毁时调用 `UnregisterCandidateInteractable`。多个候选对象重叠时，PlayerController 使用接口返回的 `GetInteractLocation()` 按距离选最近对象；`CanInteract=false` 的对象不会显示 Toast，也不会响应 E。

### BattleTriggerActor

`ABattleTriggerActor` 职责：

- 场景中的敌人触发器（use-key 交互模型，Stage 7 改）
- 实现 `IWacomWorldInteractable`，提示文本为“按 E 战斗”
- SphereCollision **仅做距离判定**：Begin/EndOverlap 注册/反注册为候选交互对象
- 玩家按 IA_Interact（默认 E 键）→ PlayerController 选中最近对象并调用 `TryInteract`
- 持有 `UEnemyDefinition*` 配置
- `FName PersistentId`（必填，关卡级唯一）

### 为什么是 use-key 而非自动触发

旧模型（overlap 自动触发）有个致命漏洞：撤离回探索时玩家仍在 Sphere 内，永远不会有 EndOverlap → BeginOverlap 的循环，无法重入战斗（GDD §10.5 撤离重入）。use-key 模型用"在范围内"作为前置条件，按键作为触发点，重入天然支持。

### BeginPlay 逻辑

1. `PersistentId == NAME_None` → Warning，继续跑
2. RunSession 中 `DestroyedTriggerIds` 包含本 id → 立即 `Destroy()`（真胜利时被销毁）
3. 否则正常运行 + 注册 Begin/EndOverlap → PlayerController.Register/UnregisterCandidateInteractable

### EndPlay

向 PlayerController 反注册自己，避免悬空。

### ShopTriggerActor

`AWacomShopTriggerActor` 职责：

- 场景中的商店交互触发器，和 BattleTriggerActor 共用 `IWacomWorldInteractable` 管线
- `PersistentId` 作为 Run 商店节点 ID；RunSession 使用它保存库存和已购买状态
- `ShopDefinition` 可引用 `UShopDefinition` 静态商品资产；配置后优先使用资产商品列表
- `Offers` 作为旧关卡兼容兜底；未配置 `ShopDefinition` 时才使用手动商品列表
- 玩家按 E 后调用 `AWacomPlayerController::RequestOpenShop(PersistentId, Offers)`
- 不切换 `EGameFlowState`，商店只是 GameMenu 层界面；关闭商店时由 Run 层按购买情况结算节点

`ShopDefinition.ShopId` 是内容 ID，不替代 `PersistentId`。多个场景商店可以引用同一份商品定义，但仍通过各自 `PersistentId` 拥有独立库存。

---

## §6 UI 架构

### UWacomGameInstance + UWacomGameUIManagerSubsystem

- `UWacomGameInstance`：GameInstance 子类
- `UWacomGameUIManagerSubsystem`：GameInstance Subsystem，管理 UI 生命周期
- PrimaryLayout 生命周期：由 Subsystem 持有，切关卡时 TearDown + 重建（跟随当前 PC）

### UWacomPrimaryGameLayout（四层 Stack）

| Layer | 用途 |
|---|---|
| Game | 战斗 HUD / 探索 HUD |
| GameMenu | 主菜单、暂停菜单、背包 |
| Modal | 确认框 |
| Overlay | Toast |

### AppToast（战斗外通用反馈）

`UWacomAppToastSubsystem` 是 Run / 探索 / 菜单侧关键反馈的统一出口，持有唯一 `UWacomAppToastWidget` 并直接 `AddToViewport` 到高 ZOrder：

- 数据：调用方传入 `FWacomAppToastView`（`MessageText / Tone / IconKey / LifetimeOverride`）
- 生命周期：Subsystem 保证当前 GameInstance 内只有一个 AppToast Widget；探索局开始时由 PlayerController 预热，首次 Toast 触发时也会懒加载兜底；无消息时 `Collapsed`，队列有消息时 `HitTestInvisible`，播完后只隐藏不销毁
- 输入：Toast Widget 为非焦点、HitTestInvisible，不抢菜单或探索输入
- CommonUI 边界：AppToast 不进 CommonUI Stack，不成为 leaf-most active widget，不改变 `FUIInputConfig`；避免背包/商店关闭后探索 IMC 被 Overlay Toast 卡住
- 当前接入：商店购买成功/失败、背包删牌成功
- 边界：不合并战斗 `EventToast`，不复用 `ExplorationHUD` 的交互提示 Toast；它们分别服务不同节奏的反馈

三类 Toast 分工：

| 类型 | 出口 | 用途 |
|---|---|---|
| 交互提示 | `UWacomExplorationHUD::SetInteractToastVisible` | “按 E 战斗/商店”等范围内提示 |
| 战斗事件 | `EventToast + UWacomBattleEventPresentationBuilder` | 战斗 Snapshot/Event 后的战斗日志与即时事件 |
| AppToast | `UWacomAppToastSubsystem` | 战斗外获得卡牌、金币变化、删牌、节点奖励、拾取等关键反馈 |

### Widget 基类体系

| 基类 | 职责 |
|---|---|
| `UWacomActivatableWidget` | 项目根基类（动画钩子、通用生命周期）|
| `UWacomBattleWidgetBase` | 战斗血统（Session + Snapshot 刷新）|
| `UWacomMenuWidgetBase` | 菜单血统（焦点管理、UIInputConfig(Menu)、Back 委托）|

### UWacomExplorationHUD

- Game 层锚点
- 探索状态下的 Run HUD：显示时段、剩余节点、天数、手指、经验、压力分项和交互 Toast
- 数据来源：`UWacomRunViewModelProvider` → `UWacomRunViewModel`
- 刷新方式：订阅 `OnRunViewModelRefreshedNative`，并在 `NativeOnActivated` 中补订阅 + 补刷新
- 战斗中行为：BattleHUD push 到同一个 Game 层后成为 active widget，ExplorationHUD 被压到栈下；ExitBattle pop BattleHUD 后 ExplorationHUD 重新激活

### MVVM 数据流（M1+M2）

Run 域 widget 用 **ViewModel + Provider 订阅模型**，避免 widget 直接订阅业务层导致的生命周期错位 bug（CommonUI Stack Activate/Deactivate vs widget Construct/Destruct 不一致）。

```
RunSession 写 API（AddPressure / OnBattleFinished / DeleteCardForGold ...）
  ↓ 末尾 NotifyRunStateChanged() 内部 Broadcast
URunSession::OnRunStateChangedNative（粗粒度多播）
  ↓ 唯一订阅者
UWacomRunViewModelProvider（GameInstance Subsystem）
  ↓ 读 RunState 字段
  ↓ 调 ViewModel.SetXxx()（UE_MVVM_SET_PROPERTY_VALUE 内置 dedupe + FieldNotify）
  ↓ 末尾 OnRunViewModelRefreshedNative.Broadcast()（粗粒度，给非 ViewBinding 的订阅方）
UWacomRunViewModel（UMVVMViewModelBase）
  ↓ 21 个 FieldNotify 字段（Phase / NodeCount / Pressure 8 条 / Capacity / Gold / ...）
Widget（C++ 直接订阅 OnRunViewModelRefreshedNative，未来 WBP 可走 ViewBinding）
  ↓ 收到事件 → 读 ViewModel → SetText
```

**当前状态（M1+M2）**：Widget 端用 C++ 父类硬编码布局 + 订阅 Provider 粗粒度事件 + 手动 SetText。FieldNotify 字段已就位但暂未被 WBP ViewBinding 消费。

**美术阶段切 WBP 时**：在 WBP Designer 里把 ViewModel 加到 widget（创建模式 = Global Viewmodel Collection，Identifier = `WacomRunViewModel`），用 View Bindings 编辑器把字段绑到 TextBlock/ProgressBar 等。WBP 启用后 C++ 父类的 SetText 路径作 fallback 保留，逐步删除。

**关键设计**：

- ViewModel 是**纯数据**，不持有 Session 指针、不订阅事件。便于单测。
- Provider 是 **GameInstance Subsystem**，跨关卡持久。声明 `Collection.InitializeDependency(UMVVMGameSubsystem)` 保证销毁顺序。
- Widget 订阅 **Provider 的粗粒度 multicast**，不订阅 RunSession——隔离业务层。
- Widget 在 NativeOnActivated 也调 `TrySubscribeAndRefresh`：CommonUI Reactivate 时补刷一次，防漏更新。

参考 DevLog：`Docs/DevLog/UI架构MVVM迁移M1M2.md`

### 战斗 UI（保持原有 Snapshot 推送模型）

战斗 UI 用 **Snapshot + Controller 推送**而非 ViewModel 订阅模型。理由：

- BattleHUD 战斗开始 Push、结束 Pop，无 Reactivate 风险
- 9 个子 widget 在同一棵树里，BattleHUD 作为 Controller 递归 RefreshFromSnapshot 自然
- Snapshot 是值类型快照，子 widget 各读不同字段，结构稳定
- Hand / EnemyParts 动态列表 ViewModel 不擅长，保留 ChildBattleWidgets 白名单递归

战斗 UI 的命令出口唯一在 BattleHUD（`Session->SubmitCommand`），子 widget 只发委托。

战斗事件表现：

- `UWacomBattleEventPresentationBuilder` 负责把部分 `FBattleEvent` 转成 `FBattleEventPresentationView`；它只做 UI 展示文本、tone、icon key，不参与规则判断。
- `EventToast` 当前只消费 `PresentationView.MessageText` 并显示 Toast 队列；`VisualTone / IconKey` 已预留给后续颜色、图标、音效和战斗日志。
- `BattleEventLogPanel` 是 `BattleHUD` 内部半屏日志抽屉，复用同一份 `FBattleEventPresentationView` 显示本场战斗最近事件；它不是独立 CommonUI 页面，不通过 Layer Stack push/pop。C++ fallback 会尝试加载 `WBP_BattleEventLogPanel / WBP_BattleEventLogEntry`，缺失时使用 C++ 默认布局。
- `HandLimitDiscarded` 表示某张卡因普通手牌上限进入弃牌区，UI/日志可以用它播放"因手牌上限弃牌"表现。
- `HandLimitDiscarded.CardInstanceId` 是被弃掉的卡；`ActorInstanceId` 只在 `EffectDraw` 时表示触发抽牌的源卡；`HandLimitDiscardSource` 区分 `TurnStart / EffectDraw / PassiveOnCompanionCount`。
- `CardGained` 表示战斗中获得一张新卡。第一版来源是击倒事件 Aid / Destroy 的部位奖励卡；`CardDefinition` 给 UI/日志显示卡名，`CardInstanceId` 是本场战斗 runtime 实例，战后是否归入 Run 以 `FBattleResultPacket.GainedCards` 为准。
- `HandZoneChanged` 仍是刷新提示，不再作为具体弃牌语义来源。

击倒事件 UI：

- `BattleHUD` 收到 `KnockdownChoiceRequested` 事件后，调用 `UBattleSession::BuildPendingKnockdownChoiceView()` 获取当前击倒事件可用性。
- `UWacomKnockdownChoiceDialog` 只消费 `FKnockdownChoiceView`：按钮启用状态来自 `AidOption / WithdrawOption / DestroyOption`，不解析 `FBattleEvent.Count`。
- `FBattleEvent.Count` 中的旧位掩码仅保留日志兼容；正式 UI 合同以 BattleSession ViewData 为准。
- Dialog 点击后仍回到 `BattleHUD->OnKnockdownChoiceSelected()` 提交命令，Dialog 不直接修改战斗状态。

---

## §7 菜单系统

### MainMenuScreen

- New Game：初始化 RunSession → OpenLevel(L_Exploration)
- Continue：LoadFromSlot → OpenLevel(L_Exploration)
- Quit：退出游戏

### PauseMenuScreen

- Resume：关闭暂停菜单
- Save：调用 RunSession::SaveToSlot("Main")
- Quit to Menu：OpenLevel(L_MainMenu)

### ConfirmDialog

- 静态工厂 `UWacomConfirmDialog::Show(Title, Message, OnConfirm, OnCancel)`
- 推入 Modal 层
- 用于"确认退出"、"确认覆盖存档"等

### 按钮委托约定

- 菜单按钮不直接 OpenLevel：委托给 GameMode 控制切关卡
- Widget 不直接调用 SubmitCommand：通过委托通知 HUD，HUD 统一提交

### BackpackScreen（GDD §11）

- 入口：探索期 B 键（IA_OpenBackpack 资产手动建后接入）→ Push 到 GameMenu 层
- 临时 console command：`Wacom.OpenBackpack` / `Wacom.CloseBackpack`
- 战斗 IMC 不绑定 IA_OpenBackpack（战斗内 B 不可打开背包；OnOpenBackpackPressed 内部还有 GameMode 状态防御）
- ESC 关闭：复用 OnOpenMenuPressed 的"GameMenu 顶层 widget 直接 Deactivate"逻辑

UI 结构（三大区）：

| 区 | 容器 | 说明 |
|---|---|---|
| 顶部行 | HorizontalBox | 标题 / 金币 / 关闭按钮 |
| 删牌区 | `UWacomDeleteZoneDropTarget` | 拖入卡牌后弹 ConfirmDialog，确认后调用 `DeleteCardForGold` |
| 备战区 | `UWacomZoneDropTarget + WrapBox` | BattleDeck 卡，标题显示 N/Capacity；同时显示已入战 SpecialZone 投影卡 |
| 背包区 / 通量存放区 | `UWacomZoneDropTarget + WrapBox` | 显示物理位于 Backpack 的通量内容；A 类容器和普通卡都作为内容卡显示 |
| 背包区 / 特殊存放区 | 主卡区 + 动态 `UWacomZoneDropTarget + WrapBox` | 每张 B 主卡一个区块；主卡区只显示该 B 主卡，内容区显示受其容量效果影响的卡 |
| 背包区 / 负重区 | `UWacomZoneDropTarget + WrapBox` | 渲染 Run 层 Snapshot 中的 `BurdenCards` |

`UWacomBackpackScreen` 暴露以下 WBP 绑定槽位：

- `DeleteZoneHost`
- `BattleDeckZoneHost`
- `FluxContentDropTargetHost`
- `FluxContentCardsBox`
- `SpecialZonesHost`
- `BurdenZoneHost`

C++ fallback 会创建默认三大区布局；如果 WBP 绑定这些 Host，C++ 只向 Host 填充运行时 DropTarget / WrapBox，不再要求美术布局复刻 C++ 默认结构。

通量区只保留当前正式接口：`FluxContentDropTargetHost` 填充可投放内容卡的 DropTarget。`FluxMainCardsHost / FluxMainCardsBox` 仅作为旧 WBP 兼容字段保留，默认 C++ fallback 不再创建或填充通量主卡区。

WBP 制作时按 `Docs/UI_Backpack_WBP_Binding.md` 的清单绑定控件；主文档只保留结构和职责说明。

主卡投影规则：
- 出战卡的物理 instance 位于 `BattleDeck`。
- A 类容器卡不再在通量区生成投影；它出战时只显示在备战区，但仍贡献通量容量。
- B 类主卡出战时，其特殊存放区仍保留；特殊区内容不因主卡物理进入 `BattleDeck` 而消失。

子控件：`UWacomDeckCardWidget`

- 主体区域：左键拖拽，生成 `UWacomCardDragOperation`（InstanceId / FromZone / FromZoneOwnerInstanceId / Definition）
- 拖拽视觉：创建当前 `UWacomDeckCardWidget` 类的新实例作为 `DefaultDragVisual`；拖拽源卡牌透明度降为 50%，拖拽完成或取消后恢复
- 删牌入口：卡牌本体不提供删除按钮；拖到删牌区后由 `UWacomDeleteZoneDropTarget` 弹确认框，确认后调 `DeleteCardForGold`
- SpecialZone 内卡：右键切换 `bBattleEnabledInSpecialZone`
- `BattleEnabledBadge`：SpecialZone 内已选择入战的卡显示“已选”
- `ProjectedFromBadge`：BattleDeck 视觉投影卡显示“来自 [B 主卡名]”

子控件：`UWacomSpecialZoneWidget`

- 负责单个 B 类特殊存放区区块，`BackpackScreen` 只负责遍历 Snapshot 并创建它。
- 输入数据为 `FRunSpecialStorageView`，显示标题、B 主卡、已入战标记和内容卡列表。
- 标题文本与已出战可见性走 `UWacomBackpackScreenPresenter`，不再依赖 `BackpackScreen` 的展示 helper。
- 内容区内部创建 `UWacomZoneDropTarget`，目标为 `EZoneKind::SpecialZone + OwnerInstanceId`。
- 内容卡右键入战 toggle 先由该组件转发，再由 `BackpackScreen` 调 `RunSession::SetSpecialZoneCardBattleEnabled()`。

### CardView

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据的统一构建入口：

- 从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData` 和 `FWacomCardViewEffectBadge`
- 负责中文词条、费用、价值、身材/容量、效果徽章、被动 fallback 文本等展示推导
- 只服务 UI 表现，不参与战斗或 Run 规则结算
- 背包卡牌、战斗手牌、拖拽预览已复用该 Builder；奖励/商店卡牌后续也应接入，避免各界面重复解析 `CardDefinition`

`UWacomCardView` 是通用卡牌显示基类，只负责渲染 `FWacomCardViewData`：

- 默认小卡面显示 Cost、价值、身材/容量、卡名、类型/词条、效果数值徽章、卡图和禁用遮罩
- `Description` 在小卡面中只显示摘要；完整描述、任务、变化、被动说明由详情面板承接
- 效果数值徽章由 `UWacomCardEffectBadgeWidget` 承接，`CardView` 只按 `EffectBadges[]` 动态创建并填入 `EffectStatsHost`
- 不提交 `BattleSession` 命令
- 不调用 `RunSession::MoveInstance` / `DeleteCardForGold`
- 兼容保留 `BuildFromCardDefinition` / `BuildDetailFromCardDefinition` 静态函数，但新代码应直接调用 `UWacomCardPresentationBuilder`

`WBP_CardView` 可继承 `UWacomCardView`，用同名 `BindWidgetOptional` 控件替换 C++ fallback 布局。

战斗手牌 `UCardWidget` 是战斗交互外壳：

- 推荐绑定 `CardView : UWacomCardView` 复用统一卡面；`ZoneText` 可作为战斗外壳上的额外分区标签显示左手/双手/右手
- `ApplyCardSnapshot` 通过 `UWacomCardPresentationBuilder` 生成卡面数据，并用 `FHandCardSnapshot.RuntimeCost` 覆盖显示费用
- `bIsPlayable=false` 时写入 `FWacomCardViewData.bDisabled`，同时禁用 `RootButton`
- 点击、hover 上浮/缩放、目标选择高亮、提交出牌命令仍由 `UCardWidget / UHandPanel / BattleHUD` 负责，`UWacomCardView` 不知道战斗交互
- `UCardWidget` 的 hover 反馈使用 Render Transform 移动 `HoverVisualRoot` 视觉层，不移动根命中区域，也不改变 `UHandPanel` 中的布局占位；`HoverLift / HoverScale` 可在 WBP Details 中调整
- 点击需要敌方部位目标的手牌时，`BattleHUD` 进入 `TargetSelect` 并记录 `PendingTargetingCardId`；再次点击同一张牌会调用 `CancelTargetSelect()` 回到 `Idle`
- 手牌选中反馈由 `UHandPanel` 根据 `BattleHUD::IsInTargetSelect()` 和 `PendingTargetingCardId` 刷新；敌方部位可选反馈由 `BattleHUD::BuildTargetSelectionView()` 输出只读表现桥，当前 `EnemyInfoBar` 按 `PartInstanceId` 消费它，未来 HD-2D/PaperZD 敌方部位 Actor 也应消费同一份视图
- 战斗手牌 hover 详情由 `BattleHUD` 管理：`UCardWidget` 上报 hover，`UHandPanel` 转发，`BattleHUD` 创建 `UWacomCardDetailPanel` 并用 `UWacomCardPresentationBuilder` 填充详情数据；面板默认显示在悬停卡牌左侧，左侧空间不足时显示在右侧
- `BattleHUD` 会记录当前详情来源卡；快速从 A 卡 hover 到 B 卡时，A 卡随后 unhover 不会误关 B 卡详情
- 进入目标选择、提交命令、刷新 Snapshot、切换 Session 或战斗结束时都会隐藏手牌详情；目标选择阶段只保留选中卡高亮和敌方部位 targetable 反馈
- `FBattleTargetSelectionView` 是 UI/App 层只读 ViewData，不是战斗规则真相；第一版只区分是否正在选目标和部位是否已破坏，最终合法性仍由 `BattleSession::SubmitCommand` / `PlayCardResolver` 校验
- `UHandPanel` 默认尝试加载 `/Game/Wacom/UI/Battle/WBP_CardWidget`；找不到时回退到 C++ 默认 `UCardWidget`
- `BattleHUD` 默认尝试加载 `/Game/Wacom/UI/Battle/WBP_HandPanel`；找不到时回退到 C++ 默认 `UHandPanel`
- `WBP_CardWidget / WBP_HandPanel` 制作时按 `Docs/UI_Battle_WBP_Binding.md` 绑定；缺少 `RootButton` 时手牌不会崩溃但无法点击
- `UHandPanel` 内部先把 `FHandQueueSnapshot` 转成 `FHandCardVisualEntry[]`，默认使用统一水平手牌带 renderer；WBP 只需要绑定 `UnifiedHandSlot`
- 统一手牌带支持 `CardSpacing / HandContentPadding / bCenterCardsWhenNotOverflow / CardVerticalAlignment` 等编辑器参数；卡牌尺寸仍由 `WBP_CardWidget` 自己控制
- C++ fallback `BattleHUD` 的手牌区域大小由 `HandPanelSize / HandPanelBottomOffset` 控制；正式 BattleHUD WBP 中应直接通过 `HandPanel` 的父级 slot 控制显示区域

`UWacomBackpackZoneSectionWidget` 是背包区块的局部 WBP 承接点：

- 用于先替换备战区、通量内容区、特殊区列表、负重区等单个外壳，不要求同时制作完整 `WBP_BackpackScreen`
- C++ fallback 会按约定路径尝试加载 `WBP_BackpackBattleDeckZone`、`WBP_BackpackFluxContentZone` 等局部 WBP
- 每个局部 WBP 只绑定 `TitleText / ContentHost`，运行时 DropTarget、WrapBox 和卡牌仍由 `UWacomBackpackScreen` 填充
- 如果某个局部 WBP 缺少 `ContentHost`，只让该区块回退到 C++ 默认外壳，不影响其他区块继续显示
- 局部 WBP 不直接调用 `RunSession`

`UWacomBackpackScreenPresenter` 是背包界面的纯展示/纯计算层：

- 负责备战区、背包区、通量内容、特殊区、负重区、金币等标题文本
- 负责 BattleDeck 投影来源 badge 文本、特殊区已出战可见性、悬浮详情面板定位
- 负责把悬停卡牌转成详情面板数据；底层仍复用 `UWacomCardPresentationBuilder`
- 不持有 Widget，不订阅事件，不调用 `RunSession` 命令
- `UWacomBackpackScreen` 保留编排职责：创建控件、绑定事件、管理详情面板生命周期、提交 Move/Delete/Toggle 命令

`UWacomCardDetailPanel` 是可复用详情面板，只负责渲染 `FWacomCardDetailViewData`：

- 当前数据从 `CardDefinition.DisplayName / Description / Passives` 推导
- `Description` 只承接主动效果/效果词条；被动说明从 `FCardPassive.DisplayText` 进入“被动” section，为空时使用规则字段生成 fallback 文本
- 任务和变化暂为空数组，等待卡牌 DataAsset 字段正式扩展后接入
- 面板自身只绑定 `SectionsBox`，不固定绑定 `NameText / DescriptionText / TasksBox / ChangesBox / PassivesBox`
- `UWacomCardDetailSectionWidget` 承接单个详情区块；描述、任务、变化、被动都会按需转成 section，空区块不创建
- 背包界面第一版由 `UWacomBackpackScreen` 在卡牌悬停时把它显示在卡牌旁边；移出、开始拖拽、列表重建或关闭背包时隐藏
- 面板为 `HitTestInvisible`，不抢鼠标，不影响左键拖拽和 SpecialZone 右键入战切换
- 不自动处理战斗手牌详情、选中态或固定详情栏生命周期
- `WBP_CardDetailPanel` 的绑定清单见 `Docs/UI_Backpack_WBP_Binding.md`

DropTarget 规则：
- 普通 zone drop 调 `RunSession->MoveInstance`。
- DeleteZone drop 先弹 `UWacomConfirmDialog`，确认后调 `RunSession->DeleteCardForGold`。
- 普通 zone drop 会先读 `RunSession->ValidateMoveInstance()`，成功后通过 `UWacomAppToastSubsystem` 显示“移动卡牌：{CardName} → {ZoneName}”，失败时显示玩家可读原因，例如通量区已满、备战区已满、特殊存放区已满、主卡不能进入特殊存放区。
- DeleteZone 会先读 `RunSession->ValidateDeleteCardForGold()`；固有卡、最后一张 BagProvider、未持有卡等情况直接显示失败 Toast，不弹确认框。确认后若规则状态变化导致删除失败，也会显示失败原因。
- DeleteZone 删除成功后通过 `UWacomAppToastSubsystem` 显示“销毁卡牌：{CardName}，获得 {Gold} 金币”；取消确认不显示成功提示。
- `NativeOnDragOver` 只做视觉预判，并优先消费 RunSession validation；最终规则仍以 RunSession 写命令返回值为准。
- DropTarget 暴露 `EWacomDropTargetState`：`Normal / HoverValid / HoverInvalid / DropAccepted / DropRejected`。
- WBP 可实现 `BP_OnDropTargetStateChanged` 做高亮、禁用提示和失败反馈。

刷新模型：
- 操作命令 → RunSession 写状态 → `OnRunStateChangedNative` → Provider 刷 ViewModel → `OnRunViewModelRefreshedNative` → `BackpackScreen::RebuildAll()`。
- RebuildAll 已拆为 `RebuildTopStats / RebuildBattleDeckZone / RebuildBackpackZone / RebuildSpecialZones / RebuildBurdenZone`。
- `RebuildBackpackZone` 只编排通量内容区；`RebuildFluxMainCards` 仅保留兼容空实现；`RebuildSpecialZones` 只创建 `UWacomSpecialZoneWidget`，单个区块内部按 `OwnerCard + ContentCards` 渲染。
- UI 不做局部 patch，成功操作后通过 `RunSession::BuildBackpackStorageSnapshot()` 全量重建列表；顶部背包/备战容量计数由 Provider 从同一 Snapshot 写入 `UWacomRunViewModel`，压力、时间等非列表标量仍由 Provider 读取 Run 状态。

---

## §8 输入协调

### CommonUI UIActionRouter leaf-most 机制

CommonUI 的 UIActionRouter 会把输入路由到"最前面的可激活 Widget"。战斗 UI 激活时，探索输入自然被屏蔽。

### Game 层切换语义

Game 层同一时间只应有一个主要 HUD 处于 active 状态：

```
探索 BeginPlay → Push ExplorationHUD 到 Game 层
EnterBattle → Push BattleHUD 到 Game 层，ExplorationHUD 进入非 active 状态
ExitBattle → Pop BattleHUD，ExplorationHUD 重新 active，并在 NativeOnActivated 补刷新
```

`AWacomPlayerController::RefreshInteractToast` 只在 `EGameFlowState::Exploration` 时显示交互 Toast。战斗中即使候选交互对象仍在列表中，也不会显示交互提示。Toast 文案来自当前最近可交互对象的 `GetInteractPromptText()`。

### ShopScreen

`UWacomShopScreen` 是第一版最小可用商店界面，Push 到 `UI_Layer_GameMenu`：

- 数据源：`URunSession::BuildCurrentShopSnapshot()`
- 商品表现：`UWacomShopPresentationBuilder` 把 `FRunShopOffer + 当前金币` 转成 `FWacomShopOfferPresentationView`
- 卡牌展示：商品 ViewData 内部复用 `UWacomCardPresentationBuilder::BuildCardViewData()`
- 商品购买：点击 Offer 行按钮调用 `URunSession::PurchaseShopOffer(OfferId)`，成功后刷新列表和金币
- 购买反馈：成功后通过 `UWacomAppToastSubsystem` 显示“获得卡牌：{CardName}”；失败时按 Offer ViewData 的 `DisabledReason` 显示“金币不足 / 该商品已购买 / 商品不可购买”
- Offer 行只渲染 ViewData 并广播购买请求，不直接解析 `CardDefinition` 或判断金币状态
- 关闭结算：`NativeOnDeactivated` 中调用一次 `URunSession::EndShopVisit()`
- 默认 C++ fallback 可运行；后续可创建 `/Game/Wacom/UI/Shop/WBP_ShopScreen` 继承本类替换视觉

### IMC 资产

| IMC | 内容 |
|---|---|
| `IMC_Exploration` | WASD 移动 + 鼠标视角 + ESC 打开暂停菜单 + B 打开背包（IA 资产建好后） |
| `IMC_Battle` | 1-7 打牌 + W 等待 + E 结束回合 + R 重启 + P 刷新 HUD（不绑 IA_OpenBackpack） |

### IA 资产

| IA | 用途 |
|---|---|
| `IA_Move` | WASD 移动 |
| `IA_Look` | 鼠标视角 |
| `IA_OpenMenu` | ESC 打开菜单 |
| `IA_PlayCard1~7` | 打出手牌 1-7 |
| `IA_Wait` | 等待 |
| `IA_EndTurn` | 结束回合 |
| `IA_Restart` | 重启战斗 |
| `IA_RefreshHUD` | 刷新 HUD |
| `IA_OpenBackpack` | 打开背包（探索 IMC 绑定，资产由用户手动创建后启用） |

### 切关卡时的 IMC 重新 Push

切关卡时 PlayerController 的 InputComponent 会被重建。GameMode 在 BeginPlay 后根据当前 `EGameFlowState` 重新 Push 对应 IMC。

---

## §9 战斗流程

### EnterBattle 完整步骤

```
玩家走进 ABattleTriggerActor 的 Sphere 范围
→ ABattleTriggerActor::HandleBeginOverlap
→ AWacomPlayerController::RegisterCandidateInteractable(this)
→ ExplorationHUD 显示 Toast"按 E 战斗"

玩家按 E（IA_Interact 或 console `Wacom.Interact`）
→ AWacomPlayerController::OnInteractPressed
→ PickClosestInteractable → IWacomWorldInteractable::TryInteract
→ AWacomPlayerController::RequestEnterBattle(EnemyDef, TriggerActor)
→ Controller 调 GameMode::EnterBattle(EnemyDef)
→ GameMode:
    1. 设 State = Battle
    2. 禁用玩家移动（Character->SetExplorationInputEnabled(false)）
    3. Pop IMC_Exploration → Push IMC_Battle
    4. 创建/激活 BattleSession + 战斗 UI
    5. 记录触发的 TriggerActor 引用
```

### ExitBattle 完整步骤

```
BattleSession 结算完毕 → Phase = BattleEnd
→ 战斗 UI 检测到 BattleEnd → 通知 Controller::OnBattleFinished(Outcome)
→ Controller 调 GameMode::ExitBattle(Outcome)
→ GameMode:
    1. 设 State = Exploration
    2. 销毁战斗 UI
    3. Pop IMC_Battle → Push IMC_Exploration
    4. 恢复玩家移动（Character->SetExplorationInputEnabled(true)）
    5. 真胜利时 Destroy 触发战斗的 ABattleTriggerActor（撤离时不销毁，玩家可重入）
    6. 通知 RunSession::OnBattleFinished(Outcome)
    7. MarkTriggerDestroyed(PersistentId)
    8. SaveToSlot("Main") + SaveToSlot("Auto")
```

---

## §10 测试约定

### Standalone vs PIE

- `L_TestBattle`：纯战斗测试，不走存档，不走 GameMode 状态切换
- `L_Exploration`：完整流程测试（探索 → 战斗 → 存档 → 读档）
- PIE 中"重新启动游戏"会保留 GameInstance，测试读档时注意区分

### ESC 设置

- 探索时 ESC → 打开暂停菜单
- 战斗时 ESC → 当前不响应（后续可加战斗暂停）
- 菜单中 ESC → 关闭当前菜单（Back 委托）

### DefaultGameMode 区分

- `L_Exploration` / `L_MainMenu` 各有自己的 GameMode
- `L_TestBattle` 使用默认 GameMode 或 BattleTestActor 直接管理
- 存档逻辑只在 `AWacomGameMode` 中，不在父类
