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
| Run Map | `URunSession::BuildCurrentFloorMapSnapshot()` 经 App-private ViewData builder | `UWacomRunMapScreen` 只上报 Select / ConfirmTravel / Close；PlayerController-owned Flow 重验并调用唯一 Coordinator MapTravel seam |
| Backpack | `URunSession::BuildBackpackStorageSnapshot()` 与 Run ViewModel 标量 | `UWacomBackpackScreen` 接收 UI 意图；统一 `Workspace` 是卡牌、框选、携带和牌堆移动的唯一输入 owner，备战/特殊/负重以被动内嵌牌堆消费 Scene ViewData；销毁目标、preview、批量提交、Toast 和 Confirm 都经 Screen 私有 command flow / presentation helper 进入 RunSession；正式 WBP 与纯 C++ fallback 共用同一个 Workspace，不再构建旧 Zone Host 或 UMG DragDrop owner；卡牌详情生命周期和定位由 App-private detail controller 承接 |
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
| GameMenu 路由 | `FWacomExplorationScreenRouter` | 统一地图、背包、暂停菜单、商店、RunEvent 的打开、关闭、异步 Push 和访问 rollback | 不提交具体 Run 规则效果 |
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
| `UWacomRunMapScreen` | GameMenu | 显示当前 Floor 已知节点/边与同层传送候选；只维护瞬时选择并广播意图，完整生命周期和提交由 `FWacomRunMapScreenFlow` 承接 |
| `UWacomBackpackScreen` | GameMenu | 展示背包、备战区、负重区和 SpecialZone；拖拽 hover preview、drop 提交或按钮意图经 Screen flow 进入 RunSession；Screen 不复制负重区规则，目标合法性由 RunSession 决定 |
| `UWacomShopScreen` | GameMenu | 展示当前商店 snapshot、金币、商品状态；购买和关闭访问经 Screen flow 提交 |
| `UWacomRunEventScreen` | GameMenu | 展示当前事件节点、选项、支付需求和后果预览；选项提交和卡牌支付经 Screen flow 提交 |
| `UWacomRunMenuWidgetBase` | Run GameMenu base | Backpack / Shop / RunEvent 的 Run 专用菜单血统；承载 Run first-person menu lease / drop 合同 |
| `UWacomMenuWidgetBase` | Common GameMenu base | 处理 CommonUI activation 和 Back 请求；仅保留 deprecated Run first-person menu lease / drop Blueprint 兼容桥 |

Run / Backpack / Shop / RunEvent 的规则真相仍在 [WacomRun.md](./WacomRun.md)。WBP 制作槽位见各 Binding 文档。`UWacomBackpackScreen` 只保留 CommonUI 生命周期、WBP/fallback 绑定、Snapshot 刷新、金币文本和玩家意图入口；正式 WBP 与纯 C++ fallback 都只走 `Snapshot -> FWacomBackpackWorkspaceSceneBuilder -> FWacomBackpackWorkspaceVisualRegistry -> UWacomBackpackWorkspaceWidget`。移动 / 删牌 / SpecialZone 入战 toggle 的 Toast 文案由 App-private command presentation helper 承接；卡牌详情由 `UWacomCardPresentationBuilder` 生成通用数据，再由 `FWacomBackpackCardDetailController` 管理 source guard、显示隐藏和 viewport-safe positioning；存放区 snapshot revision / signature dirty gate 由 `FWacomBackpackStorageRefreshGate` 承接。`FWacomBackpackWorkspaceRuntime` 是 Workspace 的单一非反射实现所有权根，内部 Registry 以实际 Canvas 子控件为真相并统一复用 Static、Carry、CarryActive 与 Settlement 的视觉实例，Motion Coordinator 统一管理局部姿态、活动卡 DepthMotion 与收落完成。旧区域 Widget、旧列表 reconciler 和并行 fallback 展示链已删除，不得再建立第二套卡牌所有权。DeckCard 的入战 affordance、投影角标和只读语义来自 `FRunStorageCardView` / Workspace scene，Widget 不按自身父级重新推断规则。

### Backpack Workspace 输入与事务流

正式结构是一个填满 GameMenu 的 `UWacomBackpackWorkspaceWidget`。通量区就是自由工作台；备战区、特殊区和非空负重区由 Scene Builder 一次投影为工作台内嵌牌堆，同时只展开一个。旧右侧区域栏与缩略 Preview 运行时链路、类和 WBP 已删除。视觉实例按 `InstanceId + OwnerInstanceId + PhysicalZone + Role` reconcile；Interaction Model 只接收按物理 `InstanceId` 去重后的可移动实体，BattleDeck projection、SpecialZone 主卡预览和负重卡只参与浏览，不能覆盖同 ID 实体的选择来源。折叠与展开都持有全部真实卡面。

```text
Run Snapshot / revision
  -> BackpackScreen coordinator
  -> Workspace Scene Builder（单份卡牌/牌堆/布局/命中合同）
  -> Visual Registry（跨层复用）-> passive Workspace / DeleteConfirm
  -> selection / carry intent
  -> BackpackCommandFlow
  -> Run atomic batch command
  -> one notification -> reconcile
```

Screen 持有输入租约和纯 interaction model；Workspace 用互斥的 `Idle / CardPress / Marquee / Carry / PileMove / Suspended` 统一处理单来源框选、拖动阈值、滚轮、按键、标题拖堆和鼠标捕获；DeckCard 只发指针意图，不创建独立 `UDragDropOperation`。牌堆位置、ZOrder、单一展开项和通量自由布局保存在 GameInstance Workspace State Store：同一 Run 重开保留，新 Run 清空，不进入 SaveGame。普通牌堆只允许标题拖动，释放按 16px 网格/边缘吸附并保证标题不重叠；开始拖堆会快照牌框位置与 ZOrder，取消同时恢复牌框和卡牌，成功才提交 Store。通量整理避让牌堆、固定负重区和销毁区。折叠使用固定 `296×420` 卡牌、零旋转和 10–24px 水平露出；展开复用同一批 Widget，以 `FocusWindowStrip` 形成左右压缩堆和动态 `1–5` 张中央完整窗口，默认完整间隔 `24px`、外围期望露出 `56px`、最低露出 `16px`，不滚动、不裁切、不缩放卡面。焦点窗口以上次起点做最小滑动，焦点卡用当前视觉中心补偿留在鼠标下；展开牌堆卡设为 pointer passthrough，浏览详情、Fake3D 与左键拾取统一由 Workspace 按当前视觉姿态和 Canvas ZOrder 解析，不能提前使用动画终点或卡牌旧 Slate 几何命中。框选与 Ctrl 选择冻结屏幕所见中心、角度和 ZOrder，直到选择生命周期结束。稳定几何或等价 Snapshot 重复提交相同目标时保留进行中的 transition，禁止取消后整组瞬移。收起时每张卡直接过渡到各自的最终折叠槽位，不允许先汇聚到标题中心。Simplified 立即完成且不产生 Hover/焦点/携带空间上抬、角度补偿或视觉弹簧；离开后冻结最后窗口直到牌堆收起。携带在合法折叠目标停留约 0.35s 后自动展开，失败保持完整携带，不产生部分提交。

背包表现制作值保存在 `DA_BackpackWorkspaceStyle`，`WBP_BackpackScreen.WorkspaceStyle` 只持有普通资产引用；禁止把 Style 作为 `Instanced/EditInlineNew` UObject 嵌入 Widget Blueprint 根详情。该 Style 资产是受版本控制的人工制作真相，Builder 只能创建缺失资产，不得在日常重建时覆盖或重新保存已有制作值。展开牌堆与多卡携带共享 `FocusWindowMaximumCards / FocusWindowFullGapPixels / FocusWindowCompressedExposurePixels / FocusWindowMinimumExposurePixels`；编辑器入口为 `WBP_BackpackScreen → WorkspaceStyle → DA_BackpackWorkspaceStyle`。普通左键按下未选中的可移动卡也会在当帧完成选择与 `Carry` 迁移；首次松开只消费起手保护，不依赖后续鼠标位移才开始跟随。

正式 Screen 直接 Fill 全局 `1920×1080` DPI 设计空间，不再拥有固定 `1600×900` 子画布或第二套全屏缩放；Header Auto、Workspace Fill、右下 DeleteTarget 覆盖层组成唯一布局合同。背包卡面固定 `CardFaceScaleBox=1.0` 与 `CardRenderSize=296×420`，不消费 Battle/Run first-person `PresentationScale`，DeckCard 内部直接承载 Battle 已制作的 `WBP_FPCardView`。App-private `FWacomBackpackWorkspaceMotionCoordinator` 只把 hover/carry 映射到 Fake3D、视差、接触阴影和实时重绘开关；任何时刻最多一张背包卡持续动态重绘，其余卡只在内容或状态变化时请求补绘。Workspace 分为 `PileFrameLayer / StaticCardLayer / SettlementLayer / MarqueeLayer / CarryRoot`；Registry 每次 Scene reconcile 都线性扫描实际 Canvas 并重建 ViewKey、物理 InstanceId 与 ZoneKey 索引，瞬态索引不构成 UObject 所有权。`CarryRoot` 内含静止的 `CarryCache(CarryLayer)` 和独立的 `CarryActiveLayer`：缓存分支保存非当前携带卡，实时分支只保存当前最前卡。携带开始时只重挂载一次，普通鼠标移动只更新外层 `CarryRoot` 的受限视觉锚点；精确逻辑指针立即进入目标判定，视觉锚点仅做最大落后受限的轻量收敛，不移动缓存节点、不重算牌列、不全量刷新静态卡，也不触发 Snapshot/Scene reconcile。成功跨区释放时，Workspace 先建立 pending visual handoff；即使目标 ViewKey 的 `PhysicalZone` 改变或目标 Snapshot 晚一帧，卡牌仍留在携带视觉层，Registry 按 `InstanceId` 迁移同一受保护实体 Widget，再由目标 Scene 直接交接到目标静态布局。禁止销毁 A 实例、创建 B 实例、在等待窗口恢复 A，或生成 A→B 过渡。部分释放只交接已提交项。携带最前卡的中性深度表现与指针位置无关，鼠标热路径禁止重复写 Retainer phase、材质参数或 `RequestRender`。Carry、PileMove、悬停展开、焦点退出、局部姿态、Settlement 和牌堆基础布局过渡共用一个按需帧 ActiveTimer；牌堆收拢以真实过渡完成为结束事件，不使用等时长回调。几何稳定采样和下一帧 Retainer 补绘继续使用独立一次性任务。首次显示时 Workspace 等待 Slate 几何连续稳定，再以同一 Snapshot 重排；稳定前隐藏统一 Canvas，最终布局与交互表现落定后在下一次 Slate 更新补绘静态卡面。GameMenu CommonUI 过渡期间，Screen 根据 PrimaryLayout 层过渡事件暂停卡面 Retainer 缓存并直绘，结束后恢复和补绘，避免把淡入 Alpha 烘入静态缓存。背包采用分层 Back：Escape 依次取消确认或瞬态交互、收起展开牌堆，再交给 CommonUI 关闭；B 始终直接关闭并统一清理。确认框暂停不可变 carry snapshot，取消或提交失败原样恢复；成功才退出相应携带项。

`UWacomShopScreen` 保留 Screen 生命周期、WBP 绑定、cached shop snapshot、商品行创建和购买意图入口；shop snapshot revision / offer row signature dirty gate 由 `FWacomShopRefreshGate` 承接，商品行的 identity reconcile、排序和移除由 `FWacomShopOfferRowListReconciler` 承接，金币变化仍通过 `CurrentGold` 进入 signature 来刷新购买可用状态。

`UWacomRunEventScreen` 保留 Screen 生命周期、WBP 绑定、cached choices、支付 Zone 映射和玩家意图入口；cached choices 与 Zone 映射的查询、支付 Zone -> Choice 解析和 debug 摘要由 App-private `FWacomRunEventPresentationStateView / Edit` 承接。Cached choices 只作为展示、复用、支付 Zone 映射和调试输入，不作为普通选项提交的最终可用性判断；点击选项必须调用 `URunSession::ChooseRunEventOptionWithResult()` 并以 `FRunEventChoiceResult` 为准。选项行和支付 DropTarget 的 identity reconcile、排序、复用和移除由 `FWacomRunEventChoiceListReconciler` 承接，稳定键为 `ChoiceId`，避免刷新事件 snapshot 时重建动态 WBP 实例或丢失支付 Zone 表现状态。卡牌支付 drop 的 resolve / submit 由 `FWacomRunEventPaymentDropFlow` 承接；Screen 只提供当前 RunSession、presentation state view、Toast 和结束状态，并在 flow 返回后记录调试摘要。卡牌支付候选进入 first-person menu lease 的 request 构造由 App-private `FWacomRunEventPaymentLeaseBuilder` 承接，负责从支付选项聚合稳定、去重、有效的 `ExplicitCardInstanceIds`，Screen 只负责清理或提交菜单租约。PIE / 蓝图排查用的 debug view、单行 summary 和 drop result summary 由 `FWacomRunEventScreenDebugBuilder` 统一生成，避免 Screen 内散落诊断字符串拼装。

## §6 卡牌展示与 Builder

`UWacomCardPresentationBuilder` 是卡牌 UI 展示数据统一入口和 Blueprint 可用门面。它从 `UCardDefinition` 生成 `FWacomCardViewData`、`FWacomCardDetailViewData` 和效果徽章 view，只服务 UI 表现，不参与 Battle 或 Run 结算。小卡卡面数据、紧凑描述、体格/价值展示和效果徽章由 App-private `WacomCardFaceViewDataBuilder` 负责；详情文档的 Section 组装由 App-private `WacomCardDetailDocumentBuilder` 负责，settings 中的 explanation lexicon 读取和路径缓存由 App-private `WacomCardExplanationLexiconProvider` 负责，settings 中的 detail theme 读取和路径缓存由 App-private `WacomCardDetailThemeProvider` 负责，block / section 组装由 App-private `WacomCardExplanationCompiler` 负责，词典模板选择和 fallback 模板由 App-private `WacomCardExplanationTemplateResolver` 负责，`{value:...}` / `{icon:...}` / `{status:...}` / `{keyword:...}` 等 typed slot 到 semantic run 的转换由 App-private `WacomCardExplanationTemplateRenderer` 负责，tag / named text 的显示文案由 App-private `WacomCardExplanationText` 统一。Widget 只消费最终 `FWacomCardViewData` / `Sections`，不推断卡面字段、Effect / Passive 的分区规则。

Battle 卡面可以额外传入 `FWacomCardPresentationRuntimeContext`（C++ only）来覆盖当前展示事实。该 context 由 `WacomApp/Private/UI/Battle` 从 `FHandCardSnapshot` 构造；拖拽、敌人 hover 或 TargetSelect hover 有候选目标时，可以再叠加 `WacomBattle` 返回的 `FBattleCardTargetPreview` facts。`FBattleCardTargetPreview` 只表示规则事实；App 侧统一通过 `WacomBattleCardPresentation::BuildTargetPreviewPresentation()` 生成 `FWacomBattleCardTargetPreviewPresentation`，其中包含 hand layer entries、source detail 和可选 target hand card detail。Bridge / coordinator 只应用这份 presentation，不在各自路径里重新拼卡面和详情。Builder 只消费这些 UI-only facts，不读取 `UBattleSession`，也不重算战斗规则。

Runtime context 当前覆盖：

- 本场 `RuntimeCost`、可用状态、卡面 Cost、disabled overlay，以及 `Magnitude.Source.RuntimeCost` / 旧 `bMagnitudeFromRuntimeCost` 徽章。
- 目标预览事实仍按 `EffectIndex` 与规则效果对齐，但紧凑卡面按 `Damage / Poison / Heal / Shield` 等语义种类聚合为 `Badge.<Kind>`。条件分量被 skip 时只是不计入该语义组的预测总值；同组存在基础贡献时继续正常显示基础值，整个语义组都不会生效时保留 Badge 并低亮，不再绘制覆盖卡面的像素叉。
- 详情面板 `Sections`：Builder 从 `Card->Effects`、`Card->Passives`、`FEffectCondition`、`FMagnitudeModifier`、`FWacomCardPresentationRuntimeContext` 和 `UWacomCardExplanationLexicon` 生成语义 `Blocks / Runs`。`FCardPassive::DisplayText` 不再作为详情面板输入；`UCardDefinition::Description` 只在完全没有结构化详情 section 时作为受限回退，按普通正文显示且不解析 `{Effect.0}` 等旧占位。词典模板支持 `{value:Magnitude}`、`{value:TriggerThreshold}`、`{icon:EffectIcon}`、`{status:EffectStatus}`、`{keyword:Tag}` 等 typed slot；默认 Damage / Heal / Shield 模板显式使用 `{icon:EffectIcon}`，Draw 模板默认不显示图标并写为“抽 {value:Magnitude} 张牌。”，Discard / ExhaustSelf / GainKeyword / RemoveStatus / ModifyInitiative 等正式效果也有可读模板，状态效果模板通过 `{status:EffectStatus}` 显示状态图标和状态名。`PassiveOutcomeTemplates` 用于 `OnCompanionCount` 这类规则真实存在但不走 `Passive.Effects` 的结果句；`MagnitudeSourceTemplates` 用于 `Magnitude.Source.RuntimeCost / TargetStatusStacks / HandCount` 等数值来源短语，例如“相当于当前费用 2”。`FCardEffect.Condition`、`FCardPassive.Condition` 和 `FCardEffect.MagnitudeModifiers` 以静态说明追加到对应 block，不在 UI 层执行规则判断或重新计算最终数值。目标预览时若 Battle preview 给出的最终数值不同于基础显示值，数值 run 只显示最终值并使用 `ValueBuffed / ValueNerfed` 标记强化或削弱；未被 preview 改写时可保留来源短语。skip 效果会由 compiler 插入 `Muted` 前缀 run。

旧 `FWacomCardDetailViewData.Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow contract 已删除。详情面板不再暴露平行纯文本正文或 `GetDescriptionText()`；需要显示的正文必须进入正式 `Sections` 语义文档。`FWacomCardViewData.Description` 仍是小卡卡面的紧凑描述；expanded detail 只在空结构化详情时复用它作为兜底可读文本。

没有 runtime context 的背包、商店和 Run 卡面继续使用静态定义展示；旧 `BuildCardViewData(Card)` / `BuildCardDetailViewData(Card)` 路径会生成基础 explanation document，但不产生 target preview 数值。

当前复用方：

- 背包卡牌、拖拽预览和卡牌详情。
- 战斗 first-person hand、Presentation Stack 小卡和 Combat Log detail。
- 商店商品 ViewData。

`UWacomCardView` 只显示 `FWacomCardViewData`，不提交战斗、背包或 Run 命令。卡牌详情由 `UWacomCardDetailPanel` 显示 `FWacomCardDetailViewData.Sections`；`WBP_CardDetailSection` 的正式制作合同是 `TitleText: CommonTextBlock` 与 `BodyText: UWacomCardDetailRichTextBlock`，正文由 `WacomCardDetailRichTextRenderer` 从语义 `Blocks / Runs` 转成 RichText markup。`BodyText` 会自动注册详情 inline icon decorator；`Status` run 显示 `CardDetailTheme.StatusBrushes` 图标并保留状态文字，显式 `{icon:EffectIcon}` 显示 `CardDetailTheme.IconBrushes` 图标。BattleHUD 内部 card detail controller 只服务 first-person viewport 详情；背包详情由 `FWacomBackpackCardDetailController` 承接背包界面内的 panel lifecycle、source guard 和定位。

## §7 WBP 绑定文档分工

Binding 文档只记录 WBP 制作合约，不写规则真相。

| 文档 | 职责 |
|---|---|
| [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 背包、局部 Zone、SpecialZone、DeckCard、CardView、CardDetail、EffectBadge 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md) | BattleHUD、手牌、CombatLogFeed、PresentationStack、场景敌人状态 Badge 和 first-person card view 的 WBP 绑定协议 |
| [UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | RunEventScreen、ChoiceButton、PaymentDropTarget 的父类、路径、绑定槽位和 PIE 检查 |
| [UI_RunMap_WBP_Binding.md](./UI_RunMap_WBP_Binding.md) | Run Map Screen、动态节点、1920×1080 画布和像素风状态合同 |

需要知道“为什么这样做”时，回到本文和对应专题文档；需要知道“绑定哪个槽位”时，读 Binding 文档。
