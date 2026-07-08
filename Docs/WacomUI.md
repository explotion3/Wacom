---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-07-08
tags:
  - wacom/ui
  - wacom/wbp
---

# WacomUI 文档

> [!info] 本文职责
> 本文是 Wacom UI 表现层总入口。它记录 UI 数据流、ownership、测试访问原则和专题导航；CommonUI shell、BattleHUD、first-person hand 等细节放到专题文档。

> [!warning] 阅读边界
> 本文不定义战斗、Run、卡牌或资产规则。规则真相见 [WacomBattle.md](./WacomBattle.md)、[WacomRun.md](./WacomRun.md)、[WacomData.md](./WacomData.md)；内容生成 / 校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)，GameplayTag 见 [WacomGameplayTags.md](./WacomGameplayTags.md)。App 编排和输入流程见 [WacomApp.md](./WacomApp.md)，世界交互和 target handle 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

<a id="wacomui-ui-ownership-map"></a>
## §1 UI 总原则

UI 不直接修改战斗或 Run 状态。Widget 读取 Snapshot、ViewData 或 ViewModel，然后把玩家意图提交给 Screen、HUD、PlayerController 或领域 Session 的明确入口。

| 领域 | 数据来源 | 命令出口 |
|---|---|---|
| ExplorationHUD | `UWacomRunViewModelProvider -> UWacomRunViewModel` | 只读显示探索状态和交互提示 |
| Backpack | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图；DropTarget 只维护 hover/drop 视觉状态，hover preview、drop 提交、Toast 文案和 Confirm 都经私有 command flow / presentation helper 调用 RunSession、Toast 和 Confirm；负重区只渲染 Run snapshot 中的卡牌，不创建 Burden DropTarget；卡牌详情面板生命周期和定位由 App-private detail controller 承接 |
| Shop | `URunSession::BuildCurrentShopSnapshot()` | `UWacomShopScreen` 接收 UI 意图，私有 flow 编排购买、关闭访问和 Toast |
| RunEvent | `URunSession::BuildCurrentRunEventSnapshot()` | `UWacomRunEventScreen` 接收 UI 意图，私有 flow 编排选项提交、支付、关闭和 Toast |
| Battle | `FBattleSnapshot`、`FBattleEvent`、Battle ViewData | `UBattleHUD` 是唯一战斗 UI 命令出口 |

Widget 可以有 C++ fallback 布局，但 C++ 的职责是协议、生命周期和兜底显示；正式视觉由 WBP 承接。复杂流程应收口到 `WacomApp/Private` 的 flow / coordinator helper，Screen 保持 View 所有权和玩家意图入口。

## §2 专题入口

| 专题 | 当前事实入口 | 关注点 |
|---|---|---|
| UI Foundation | [WacomUIFoundation.md](./WacomUIFoundation.md) | CommonUI shell、PrimaryLayout、UI Settings、Widget registry、Modal、MainMenu、Run MVVM、AppToast |
| Battle UI | [WacomBattleUI.md](./WacomBattleUI.md) | BattleHUD、命令出口、HUD state、targeting、presentation flow、Combat Log、当前 Battle UI surface |
| First-person card layer | [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) | 第一人称手牌 authoring / runtime contract、Battle/Run source、hover/drag/drop、WBP_FPCardView |
| World interaction | [WacomWorldInteraction.md](./WacomWorldInteraction.md) | Run world target、Battle scene target、Run menu zone target、target handle |
| WBP 绑定 | [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md)、[UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)、[UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | 父类、资产路径、BindWidget 槽位和 PIE 检查 |

## §3 Ownership Map

Wacom UI 采用“Shell 集中 + Screen coordinator 分域负责”的结构，不新增全局大 UI manager，也不让被动 Widget 直接写规则状态。

| 层级 | Owner | 职责 | 边界 |
|---|---|---|---|
| UI Shell | `UWacomGameUIManagerSubsystem + UWacomPrimaryGameLayout` | 创建 / 重建根布局，按 Game / GameMenu / Modal / Overlay 管理 CommonUI stack | 不解释战斗、Run 或菜单业务规则 |
| 顶层配置 | `UWacomUIDeveloperSettings` | 注册 PrimaryLayout、AppToast 和 `UI.Widget.*` 顶层 WBP 覆盖 | 不作为运行时状态来源 |
| App 流程 | `AWacomGameMode / AWacomMenuGameMode / AWacomPlayerController` | 进入 / 退出战斗，主菜单 travel，探索交互，目标输入路由 | 不把 Screen 内部布局写进流程类 |
| GameMenu 路由 | `FWacomExplorationScreenRouter` | 统一背包、暂停菜单、商店、RunEvent 的打开、关闭、异步 Push 和访问 rollback | 不提交具体 Run 规则效果 |
| 输入上下文 | `UWacomInputContextCoordinatorSubsystem` | 统一 CommonUI input config、鼠标显隐 / capture、Enhanced Input profile 和 click / mouse-over lease | 不读取具体 Widget 业务状态 |
| Screen coordinator | `UBattleHUD / UWacomBackpackScreen / UWacomShopScreen / UWacomRunEventScreen` | 持有 Screen 生命周期、WBP 绑定、ViewData 刷新和玩家意图入口 | 子 Widget 不直接写领域状态 |
| Passive Widget | 卡牌、列表项、Badge、Panel、Button 等 | 显示 ViewData 并通过委托上报玩家意图 | 不直接调用 `UBattleSession` 或 `URunSession` 写 API |

## §4 Public Widget API 与测试口

复杂 Widget 的 public 头只保留 WBP 绑定、运行时生命周期和真实玩家意图入口。不要为了自动化测试在 `WacomApp/Public` 暴露 callable 的 `ForTest`、`ForAutomationTest`、`OverrideForTest`，也不要新增 Blueprint 可见测试 `UFUNCTION`。

测试复杂 Widget 行为时按以下优先级处理：

1. 在 `WacomTests` 中写 tests-only probe subclass，通过真实激活、刷新和玩家意图入口观察结果。
2. 如果运行时代码本身需要可扩展点，提炼 protected production seam；它必须对生产行为有清晰语义。
3. 只有前两者不适合时，才使用 automation-only private friend / test-access；这类入口不能是 `UFUNCTION` 或 Blueprint API。

当前测试访问口径：

| 区域 | 访问方式 |
|---|---|
| BattleHUD | `FWacomBattleHUDTestHarness`、`UWacomBattleHUDDetailTest` 和 BattleHUD automation test view |
| First-person card layer | production 非反射 test view + `FWacomFirstPersonCardLayerTestAccess` |
| Backpack | `FWacomBackpackScreenTestAccess` |
| CardView / EffectBadge | automation test view + `FWacomCardViewTestAccess` |
| Shop / RunEvent | automation test view + `FWacomShopRunEventTestAccess` |
| PlayerController / world probe | `FWacomPlayerControllerRunInteractionTestAccess`、`FWacomBattleSceneTargetClickTestAccess`、Run world / menu drop / choice button probe wrapper |

## §5 Run UI 摘要

Run UI 只显示 RunSession 的当前事实和 presentation view，不直接改 RunState。

| Screen / Widget | 层级 | 当前职责 |
|---|---|---|
| `UWacomExplorationHUD` | Game | 显示探索时段、剩余节点、交互提示；读取 Run ViewModel |
| `UWacomBackpackScreen` | GameMenu | 展示背包、备战区、负重区和 SpecialZone；拖拽 hover preview、drop 提交或按钮意图经 Screen flow 进入 RunSession；Screen 不复制负重区规则，目标合法性由 RunSession 决定 |
| `UWacomShopScreen` | GameMenu | 展示当前商店 snapshot、金币、商品状态；购买和关闭访问经 Screen flow 提交 |
| `UWacomRunEventScreen` | GameMenu | 展示当前事件节点、选项、支付需求和后果预览；选项提交和卡牌支付经 Screen flow 提交 |
| `UWacomRunMenuWidgetBase` | Run GameMenu base | Backpack / Shop / RunEvent 的 Run 专用菜单血统；承载 Run first-person menu lease / drop 合同 |
| `UWacomMenuWidgetBase` | Common GameMenu base | 处理 CommonUI activation 和 Back 请求；仅保留 deprecated Run first-person menu lease / drop Blueprint 兼容桥 |

Run / Backpack / Shop / RunEvent 的规则真相仍在 [WacomRun.md](./WacomRun.md)。WBP 制作槽位见各 Binding 文档。`UWacomBackpackScreen` 保留 Screen 生命周期、WBP 绑定、列表刷新和玩家意图入口；顶部金币 / 备战区标题等 header 标量刷新由 App-private `FWacomBackpackHeaderPresenter` 承接，移动 / 删牌 / SpecialZone 入战 toggle 的 Toast 文案由 App-private command presentation helper 承接，卡牌详情面板的创建、source guard、显示隐藏和 viewport-safe positioning 由 `FWacomBackpackCardDetailController` 承接，存放区 snapshot revision / signature dirty gate 由 `FWacomBackpackStorageRefreshGate` 承接，普通卡牌列表的 identity reconcile、复用、排序和移除回调由 `FWacomBackpackDeckCardListReconciler` 承接，SpecialZone 区块的 identity reconcile、排序和移除回调由 `FWacomBackpackSpecialZoneListReconciler` 承接，避免在 Screen / SpecialZone Widget 内重复扩散列表算法。

`UWacomShopScreen` 保留 Screen 生命周期、WBP 绑定、cached shop snapshot、商品行创建和购买意图入口；shop snapshot revision / offer row signature dirty gate 由 `FWacomShopRefreshGate` 承接，商品行的 identity reconcile、排序和移除由 `FWacomShopOfferRowListReconciler` 承接，金币变化仍通过 `CurrentGold` 进入 signature 来刷新购买可用状态。

`UWacomRunEventScreen` 保留 Screen 生命周期、WBP 绑定、cached choices、支付 Zone 映射和玩家意图入口；cached choices 与 Zone 映射的查询、支付 Zone -> Choice 解析和 debug 摘要由 App-private `FWacomRunEventPresentationStateView / Edit` 承接。Cached choices 只作为展示、复用、支付 Zone 映射和调试输入，不作为普通选项提交的最终可用性判断；点击选项必须调用 `URunSession::ChooseRunEventOptionWithResult()` 并以 `FRunEventChoiceResult` 为准。选项行和支付 DropTarget 的 identity reconcile、排序、复用和移除由 `FWacomRunEventChoiceListReconciler` 承接，稳定键为 `ChoiceId`，避免刷新事件 snapshot 时重建动态 WBP 实例或丢失支付 Zone 表现状态。卡牌支付 drop 的 resolve / submit 由 `FWacomRunEventPaymentDropFlow` 承接；Screen 只提供当前 RunSession、presentation state view、Toast 和结束状态，并在 flow 返回后记录调试摘要。卡牌支付候选进入 first-person menu lease 的 request 构造由 App-private `FWacomRunEventPaymentLeaseBuilder` 承接，负责从支付选项聚合稳定、去重、有效的 `ExplicitCardInstanceIds`，Screen 只负责清理或提交菜单租约。PIE / 蓝图排查用的 debug view、单行 summary 和 drop result summary 由 `FWacomRunEventScreenDebugBuilder` 统一生成，避免 Screen 内散落诊断字符串拼装。

## §6 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口和 Blueprint 可用门面。它从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData` 和效果徽章 view，只服务 UI 表现，不参与 Battle 或 Run 结算。小卡卡面数据、紧凑描述、体格/价值展示和效果徽章由 App-private `WacomCardFaceViewDataBuilder` 负责；详情文档的 Section 组装由 App-private `WacomCardDetailDocumentBuilder` 负责，settings 中的 explanation lexicon 读取和路径缓存由 App-private `WacomCardExplanationLexiconProvider` 负责，block / section 组装由 App-private `WacomCardExplanationCompiler` 负责，词典模板选择和 fallback 模板由 App-private `WacomCardExplanationTemplateResolver` 负责，`{value:...}` / `{icon:...}` / `{status:...}` / `{keyword:...}` 等 typed slot 到 semantic run 的转换由 App-private `WacomCardExplanationTemplateRenderer` 负责，tag leaf 显示文本由 App-private `WacomCardExplanationText` 统一。Widget 只消费最终 `FWacomCardViewData` / `Sections`，不推断卡面字段、Effect / Passive 的分区规则。

Battle 卡面可以额外传入 `FWacomCardPresentationRuntimeContext`（C++ only）来覆盖当前展示事实。该 context 由 `WacomApp/Private/UI/Battle` 从 `FHandCardSnapshot` 构造；拖拽、敌人 hover 或 TargetSelect hover 有候选目标时，可以再叠加 `WacomBattle` 返回的 `FBattleCardTargetPreview` facts。`FBattleCardTargetPreview` 只表示规则事实；App 侧统一通过 `WacomBattleCardPresentation::BuildTargetPreviewPresentation()` 生成 `FWacomBattleCardTargetPreviewPresentation`，其中包含 hand layer entries、source detail 和可选 target hand card detail。Bridge / coordinator 只应用这份 presentation，不在各自路径里重新拼卡面和详情。Builder 只消费这些 UI-only facts，不读取 `UBattleSession`，也不重算战斗规则。

Runtime context 当前覆盖：

- 本场 `RuntimeCost`、可用状态、卡面 Cost、disabled overlay，以及 `Magnitude.Source.RuntimeCost` / 旧 `bMagnitudeFromRuntimeCost` 徽章。
- 按 `EffectIndex` 的目标预览 magnitude override / skip；被 preview 判定不会生效的效果不显示误导性徽章。
- 详情面板 `Sections`：Builder 从 `Card->Effects`、`Card->Passives`、`FEffectCondition`、`FMagnitudeModifier`、`FWacomCardPresentationRuntimeContext` 和 `UWacomCardExplanationLexicon` 生成语义 `Blocks / Runs`。`UCardDefinition::Description` 和 `FCardPassive::DisplayText` 不再作为详情面板输入；它们可以继续服务小卡或其它旧 UI。词典模板支持 `{value:Magnitude}`、`{value:TriggerThreshold}`、`{icon:EffectIcon}`、`{status:EffectStatus}`、`{keyword:Tag}` 等 typed slot；`FCardEffect.Condition`、`FCardPassive.Condition` 和 `FCardEffect.MagnitudeModifiers` 以静态说明追加到对应 block，不在 UI 层执行规则判断或重新计算最终数值。目标预览时数值 run 只显示 Battle preview 给出的最终数值，RichText 使用 `ValueBuffed / ValueNerfed` 标记强化或削弱，skip 效果会被标记为不会生效。

旧 `FWacomCardDetailViewData.Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow contract 已删除。详情面板不再暴露平行纯文本正文或 `GetDescriptionText()`；需要显示的正文必须进入正式 `Sections` 语义文档。`FWacomCardViewData.Description` 仍是小卡卡面的紧凑描述，不属于详情面板 contract。

没有 runtime context 的背包、商店和 Run 卡面继续使用静态定义展示；旧 `BuildCardViewData(Card)` / `BuildCardDetailViewData(Card)` 路径会生成基础 explanation document，但不产生 target preview 数值。

当前复用方：

- 背包卡牌、拖拽预览和卡牌详情。
- 战斗 first-person hand、Presentation Stack 小卡和 Combat Log detail。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌详情由 `UWacomCardDetailPanel` 显示 `FWacomCardDetailViewData.Sections`；`WBP_CardDetailSection` 的正式制作合同是 `TitleText: CommonTextBlock` 与 `BodyText: UWacomCardDetailRichTextBlock`，正文由 `WacomCardDetailRichTextRenderer` 从语义 `Blocks / Runs` 转成 RichText markup。BattleHUD 内部 card detail controller 只服务 first-person viewport 详情；背包详情由 `FWacomBackpackCardDetailController` 承接背包界面内的 panel lifecycle、source guard 和定位。

## §7 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约，不写规则真相。

| 文档 | 职责 |
|---|---|
| [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md) | BattleHUD、手牌、CombatLogFeed、PresentationStack、场景敌人状态 Badge 和 first-person card view 的 WBP 绑定协议 |
| [UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | RunEventScreen、ChoiceButton、PaymentDropTarget 的父类、路径、绑定槽位和 PIE 检查 |

需要知道“为什么这样做”时，回到本文和对应专题文档；需要知道“绑定哪个槽位”时，读 Binding 文档。
