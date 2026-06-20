---
type: presentation-contract
scope: wacom-ui
status: active
updated: 2026-06-20
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
| Backpack | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图，私有 command flow 调用 RunSession、Toast 和 Confirm |
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
| `UWacomBackpackScreen` | GameMenu | 展示背包、备战区、负重区和 SpecialZone；拖拽或按钮意图经 Screen flow 进入 RunSession |
| `UWacomShopScreen` | GameMenu | 展示当前商店 snapshot、金币、商品状态；购买和关闭访问经 Screen flow 提交 |
| `UWacomRunEventScreen` | GameMenu | 展示当前事件节点、选项、支付需求和后果预览；选项提交和卡牌支付经 Screen flow 提交 |
| `UWacomMenuWidgetBase` | GameMenu base | 处理 CommonUI activation、Back 请求和可选 first-person card menu lease |

Run / Backpack / Shop / RunEvent 的规则真相仍在 [WacomRun.md](./WacomRun.md)。WBP 制作槽位见各 Binding 文档。

## §6 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口。它从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData`、详情 token line 和效果徽章 view，只服务 UI 表现，不参与 Battle 或 Run 结算。

Battle 卡面可以额外传入 `FWacomCardPresentationRuntimeContext`（C++ only）来覆盖当前展示事实。该 context 由 `WacomApp/Private/UI/Battle` 从 `FHandCardSnapshot` 构造；拖拽、敌人 hover 或 TargetSelect hover 有候选目标时，可以再叠加 `WacomBattle` 返回的 `FBattleCardTargetPreview` facts。`FBattleCardTargetPreview` 只表示规则事实；App 侧统一通过 `WacomBattleCardPresentation::BuildTargetPreviewPresentation()` 生成 `FWacomBattleCardTargetPreviewPresentation`，其中包含 hand layer entries、source detail 和可选 target hand card detail。Bridge / coordinator 只应用这份 presentation，不在各自路径里重新拼卡面和详情。Builder 只消费这些 UI-only facts，不读取 `UBattleSession`，也不重算战斗规则。

Runtime context 当前覆盖：

- 本场 `RuntimeCost`、可用状态、卡面 Cost、disabled overlay，以及 `Magnitude.Source.RuntimeCost` / 旧 `bMagnitudeFromRuntimeCost` 徽章。
- 按 `EffectIndex` 的目标预览 magnitude override / skip；被 preview 判定不会生效的效果不显示误导性徽章。
- 详情面板 `TokenLines`：`Description` kind 保留原 `Description` 文案并用 token WBP 渲染；描述文本可使用显式占位符 `{Effect.N}` 引用主动效果，例如 `造成 {Effect.0} 伤害。` 会显示为类似 `造成 [伤] 6 伤害。`，目标预览时占位符内数字可显示 `6 -> 8`。`Effect` kind 只在没有手写 `Description` 时作为自动 fallback 主动效果行；`Passive` kind 承载被动触发 / 被动效果 token。面板会把 `Description + Effect` 渲染进“描述”Section，把 `Passive` 渲染进“被动”Section，不再生成“规则”Section。Description 不从中文自然语言反推规则，普通旧文本如 `造成 6 伤害。` 会按纯文本显示。Token 使用稳定 `StableId`，后续 WBP / RichText / 动效可以按 token 复用和高亮；约定路径下存在 `WBP_CardDetailTokenFlow / Line / Token` 时会自动进入 UMG 制作链路，不存在时 C++ fallback 只把 icon 渲染成短文本占位。
- 详情面板 `ChangeLines`：legacy/debug 过渡字段，不再作为正式 `变化` 区块渲染。费用变化、目标手牌 cost preview 等卡面属性变化应反映到对应 `UWacomCardView` 卡面数值，不生成 `[费] before -> after` 详情 token。
- 详情面板 `PassiveLines`：只作为被动无法 token 化时的 fallback；只要 `TokenLines` 中存在被动 token，面板不再额外显示旧 `被动` 区块。

没有 runtime context 的背包、商店和 Run 卡面继续使用静态定义展示；旧 `BuildCardViewData(Card)` / `BuildCardDetailViewData(Card)` 路径会生成基础 token lines，但不产生 target preview 数值。

当前复用方：

- 背包卡牌、拖拽预览和卡牌详情。
- 战斗 first-person hand、Presentation Stack 小卡和 Combat Log detail。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌详情由 `UWacomCardDetailPanel` 显示 `FWacomCardDetailViewData`；`UWacomCardDetailTokenFlowWidget -> UWacomCardDetailTokenLineWidget -> UWacomCardDetailTokenWidget` 是 `TokenLines` 的正式 UMG 制作入口，并会优先使用 `/Game/Wacom/UI/Card/` 下同名 WBP 资产。BattleHUD 内部 card detail controller 只服务 first-person viewport 详情。

## §7 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约，不写规则真相。

| 文档 | 职责 |
|---|---|
| [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md) | BattleHUD、手牌、CombatLogFeed、PresentationStack、场景敌人状态 Badge 和 first-person card view 的 WBP 绑定协议 |
| [UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | RunEventScreen、ChoiceButton、PaymentDropTarget 的父类、路径、绑定槽位和 PIE 检查 |

需要知道“为什么这样做”时，回到本文和对应专题文档；需要知道“绑定哪个槽位”时，读 Binding 文档。
